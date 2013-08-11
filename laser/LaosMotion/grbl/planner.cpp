/*
  planner.c - buffers movement commands and manages the acceleration profile plan
  Part of Grbl

  Copyright (c) 2009-2011 Simen Svale Skogsrud
  Copyright (c) 2011 Sungeun K. Jeon
  
  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

/* The ring buffer implementation gleaned from the wiring_serial library by David A. Mellis. */

#include <inttypes.h>
#include <stdbool.h>
#include <math.h>       
#include <stdlib.h>
#include <string.h>


#include "global.h"
#include "planner.h"
#include "stepper.h"
#include "config.h"

// The GRBL configuration (scaling etc)
config_t config;

#define lround(x) ( (long)floor(x+0.5) )

// The number of linear motions that can be in the plan at any give time
#define BLOCK_BUFFER_SIZE 16
tTarget startpoint;

static block_t block_buffer[BLOCK_BUFFER_SIZE];  // A ring buffer for motion instructions
static volatile uint8_t block_buffer_head;       // Index of the next block to be pushed
static volatile uint8_t block_buffer_tail;       // Index of the block to process now

static int32_t position[NUM_AXES];             // The current position of the tool in absolute steps
static float previous_unit_vec[NUM_AXES];     // Unit vector of previous path line segment
static float previous_nominal_speed;   // Nominal speed of previous path line segment

static uint8_t acceleration_manager_enabled;   // Acceleration management active?


// initial entry point of the planner
// Clear values and set defaults
void plan_init() {
  block_buffer_head = 0;
  block_buffer_tail = 0;
  plan_set_acceleration_manager_enabled(true);
  clear_vector(position);
  clear_vector_double(previous_unit_vec);
  previous_nominal_speed = 0.0;
  
  memset (&startpoint, 0, sizeof(startpoint));
  
 // default config:
  config.steps_per_mm_x = fabs((float)cfg->xscale/1000.0); // convert xscale from [steps/meter] to [steps/mm]
  config.steps_per_mm_y = fabs((float)cfg->yscale/1000.0); 
  config.steps_per_mm_z = fabs((float)cfg->zscale/1000.0); 
  config.steps_per_mm_e = fabs((float)cfg->escale/1000.0); 
  config.maximum_feedrate_x = 60 * cfg->xspeed; // convert speed from [mm/sec] to [mm/min]
  config.maximum_feedrate_y = 60 * cfg->yspeed;
  config.maximum_feedrate_z = 60 * cfg->zspeed;
  config.maximum_feedrate_e = 60 * cfg->espeed;
  config.acceleration = cfg->accel; // [mm/sec2]
  config.junction_deviation = cfg->tolerance/1000.0; //  convert tolerance from [micron] to [mm]
  
  config.junction_deviation = 0.05;
 //  config.steps_per_mm_x =  config.steps_per_mm_y =  config.steps_per_mm_z =  config.steps_per_mm_e = 200;
  // config.acceleration = 200;
  //config.maximum_feedrate_x =  config.maximum_feedrate_y =  config.maximum_feedrate_z =  config.maximum_feedrate_e = 60000;
  printf("steps_per_mm_x %f...\n", (float)config.steps_per_mm_x);
  printf("steps_per_mm_y %f...\n", (float)config.steps_per_mm_y);
  printf("steps_per_mm_z %f...\n", (float)config.steps_per_mm_z);
  printf("steps_per_mm_e %f...\n", (float)config.steps_per_mm_e);
  printf("accel %f...\n", (float)config.acceleration);
  printf("Motion: double=%d, float=%d, block=%d\n", sizeof(double), sizeof(float), sizeof(block_t));

}


// Returns the index of the next block in the ring buffer
// NOTE: Removed modulo (%) operator, which uses an expensive divide and multiplication.
static int8_t next_block_index(int8_t block_index) {
  block_index++;
  if (block_index == BLOCK_BUFFER_SIZE) { block_index = 0; }
  return(block_index);
}


// Returns the index of the previous block in the ring buffer
static int8_t prev_block_index(int8_t block_index) {
  if (block_index == 0) { block_index = BLOCK_BUFFER_SIZE; }
  block_index--;
  return(block_index);
}


// Calculates the distance (not time) it takes to accelerate from initial_rate to target_rate using the 
// given acceleration:
static float estimate_acceleration_distance(float initial_rate, float target_rate, float acceleration) {
  return( (target_rate*target_rate-initial_rate*initial_rate)/(2*acceleration) );
}


/*                        + <- some maximum rate we don't care about
                         /|\
                        / | \                    
                       /  |  + <- final_rate     
                      /   |  |                   
     initial_rate -> +----+--+                   
                          ^  ^                   
                          |  |                   
      intersection_distance  distance                                                                           */
// This function gives you the point at which you must start braking (at the rate of -acceleration) if 
// you started at speed initial_rate and accelerated until this point and want to end at the final_rate after
// a total travel of distance. This can be used to compute the intersection point between acceleration and
// deceleration in the cases where the trapezoid has no plateau (i.e. never reaches maximum speed)
static float intersection_distance(float initial_rate, float final_rate, float acceleration, float distance) {
  return( (2*acceleration*distance-initial_rate*initial_rate+final_rate*final_rate)/(4*acceleration) );
}

            
// Calculates the maximum allowable speed at this point when you must be able to reach target_velocity
// using the acceleration within the allotted distance.
// NOTE: sqrt() reimplimented here from prior version due to improved planner logic. Increases speed
// in time critical computations, i.e. arcs or rapid short lines from curves. Guaranteed to not exceed
// BLOCK_BUFFER_SIZE calls per planner cycle.
static float max_allowable_speed(float acceleration, float target_velocity, float distance) {
  return( sqrt(target_velocity*target_velocity-2*acceleration*60*60*distance) );
}


// The kernel called by planner_recalculate() when scanning the plan from last to first entry.
static void planner_reverse_pass_kernel(block_t *previous, block_t *current, block_t *next) {
  if (!current) { return; }  // Cannot operate on nothing.
  
  if (next) { 
    // If entry speed is already at the maximum entry speed, no need to recheck. Block is cruising.
    // If not, block in state of acceleration or deceleration. Reset entry speed to maximum and 
    // check for maximum allowable speed reductions to ensure maximum possible planned speed.
    if (current->entry_speed != current->max_entry_speed) {
    
      // If nominal length true, max junction speed is guaranteed to be reached. Only compute
      // for max allowable speed if block is decelerating and nominal length is false.
      if ((!current->nominal_length_flag) && (current->max_entry_speed > next->entry_speed)) {
        current->entry_speed = min( current->max_entry_speed,
          max_allowable_speed(-config.acceleration,next->entry_speed,current->millimeters));
      } else {
        current->entry_speed = current->max_entry_speed;
      } 
      current->recalculate_flag = true;    
    
    }
  } // Skip last block. Already initialized and set for recalculation.
}


// planner_recalculate() needs to go over the current plan twice. Once in reverse and once forward. This 
// implements the reverse pass.
static void planner_reverse_pass() {
  auto int8_t block_index = block_buffer_head;
  block_t *block[3] = {NULL, NULL, NULL};
  while(block_index != block_buffer_tail) {    
    block_index = prev_block_index( block_index );
    block[2]= block[1];
    block[1]= block[0];
    block[0] = &block_buffer[block_index];
    planner_reverse_pass_kernel(block[0], block[1], block[2]);
  }
  // Skip buffer tail/first block to prevent over-writing the initial entry speed.
}


// The kernel called by planner_recalculate() when scanning the plan from first to last entry.
static void planner_forward_pass_kernel(block_t *previous, block_t *current, block_t *next) {
  if(!previous) { return; }  // Begin planning after buffer_tail
  
  // If the previous block is an acceleration block, but it is not long enough to complete the
  // full speed change within the block, we need to adjust the entry speed accordingly. Entry
  // speeds have already been reset, maximized, and reverse planned by reverse planner.
  // If nominal length is true, max junction speed is guaranteed to be reached. No need to recheck.  
  if (!previous->nominal_length_flag) {
    if (previous->entry_speed < current->entry_speed) {
      float entry_speed = min( current->entry_speed,
        max_allowable_speed(-config.acceleration,previous->entry_speed,previous->millimeters) );

      // Check for junction speed change
      if (current->entry_speed != entry_speed) {
        current->entry_speed = entry_speed;
        current->recalculate_flag = true;
      }
    }    
  }
}


// planner_recalculate() needs to go over the current plan twice. Once in reverse and once forward. This 
// implements the forward pass.
static void planner_forward_pass() {
  int8_t block_index = block_buffer_tail;
  block_t *block[3] = {NULL, NULL, NULL};
  
  while(block_index != block_buffer_head) {
    block[0] = block[1];
    block[1] = block[2];
    block[2] = &block_buffer[block_index];
    planner_forward_pass_kernel(block[0],block[1],block[2]);
    block_index = next_block_index( block_index );
  }
  planner_forward_pass_kernel(block[1], block[2], NULL);
}


/*                             STEPPER RATE DEFINITION                                              
                                     +--------+   <- nominal_rate
                                    /          \                                
    nominal_rate*entry_factor ->   +            \                               
                                   |             + <- nominal_rate*exit_factor  
                                   +-------------+                              
                                       time -->                                 
*/                                                                              
// Calculates trapezoid parameters so that the entry- and exit-speed is compensated by the provided factors.
// The factors represent a factor of braking and must be in the range 0.0-1.0.
// This converts the planner parameters to the data required by the stepper controller.
// NOTE: Final rates must be computed in terms of their respective blocks.
static void calculate_trapezoid_for_block(block_t *block, float entry_factor, float exit_factor) {
  
  block->initial_rate = ceil(block->nominal_rate*entry_factor); // (step/min)
  block->final_rate = ceil(block->nominal_rate*exit_factor); // (step/min)
  int32_t acceleration_per_minute = block->rate_delta*ACCELERATION_TICKS_PER_SECOND*60.0; // (step/min^2)
  int32_t accelerate_steps = 
    ceil(estimate_acceleration_distance(block->initial_rate, block->nominal_rate, acceleration_per_minute));
  int32_t decelerate_steps = 
    floor(estimate_acceleration_distance(block->nominal_rate, block->final_rate, -acceleration_per_minute));

  // Calculate the size of Plateau of Nominal Rate. 
  int32_t plateau_steps = block->step_event_count-accelerate_steps-decelerate_steps;
  
  // Is the Plateau of Nominal Rate smaller than nothing? That means no cruising, and we will
  // have to use intersection_distance() to calculate when to abort acceleration and start braking 
  // in order to reach the final_rate exactly at the end of this block.
  if (plateau_steps < 0) {  
    accelerate_steps = ceil(
      intersection_distance(block->initial_rate, block->final_rate, acceleration_per_minute, block->step_event_count));
    accelerate_steps = max(accelerate_steps,0); // Check limits due to numerical round-off
    accelerate_steps = min(accelerate_steps,block->step_event_count);
    plateau_steps = 0;
  }  
  
  block->accelerate_until = accelerate_steps;
  block->decelerate_after = accelerate_steps+plateau_steps;
}     

/*                            PLANNER SPEED DEFINITION                                              
                                     +--------+   <- current->nominal_speed
                                    /          \                                
         current->entry_speed ->   +            \                               
                                   |             + <- next->entry_speed
                                   +-------------+                              
                                       time -->                                 
*/                                                                              
// Recalculates the trapezoid speed profiles for flagged blocks in the plan according to the 
// entry_speed for each junction and the entry_speed of the next junction. Must be called by 
// planner_recalculate() after updating the blocks. Any recalulate flagged junction will
// compute the two adjacent trapezoids to the junction, since the junction speed corresponds 
// to exit speed and entry speed of one another.
static void planner_recalculate_trapezoids() {
  int8_t block_index = block_buffer_tail;
  block_t *current;
  block_t *next = NULL;
  
  while(block_index != block_buffer_head) {
    current = next;
    next = &block_buffer[block_index];
    if (current) {
      // Recalculate if current block entry or exit junction speed has changed.
      if (current->recalculate_flag || next->recalculate_flag) {
        // NOTE: Entry and exit factors always > 0 by all previous logic operations.     
        calculate_trapezoid_for_block(current, current->entry_speed/current->nominal_speed,
          next->entry_speed/current->nominal_speed);      
        current->recalculate_flag = false; // Reset current only to ensure next trapezoid is computed
      }
    }
    block_index = next_block_index( block_index );
  }
  // Last/newest block in buffer. Exit speed is set with MINIMUM_PLANNER_SPEED. Always recalculated.
  calculate_trapezoid_for_block(next, next->entry_speed/next->nominal_speed,
    MINIMUM_PLANNER_SPEED/next->nominal_speed);
  next->recalculate_flag = false;
}

// Recalculates the motion plan according to the following algorithm:
//
//   1. Go over every block in reverse order and calculate a junction speed reduction (i.e. block_t.entry_speed) 
//      so that:
//     a. The junction speed is equal to or less than the maximum junction speed limit
//     b. No speed reduction within one block requires faster deceleration than the one, true constant 
//        acceleration.
//   2. Go over every block in chronological order and dial down junction speed values if 
//     a. The speed increase within one block would require faster acceleration than the one, true 
//        constant acceleration.
//
// When these stages are complete all blocks have an entry speed that will allow all speed changes to 
// be performed using only the one, true constant acceleration, and where no junction speed is greater
// than the max limit. Finally it will:
//
//   3. Recalculate trapezoids for all blocks using the recently updated junction speeds. Block trapezoids
//      with no updated junction speeds will not be recalculated and assumed ok as is.
//
// All planner computations are performed with doubles (float on Arduinos) to minimize numerical round-
// off errors. Only when planned values are converted to stepper rate parameters, these are integers.

static void planner_recalculate() {     
  planner_reverse_pass();
  planner_forward_pass();
  planner_recalculate_trapezoids();
}

void plan_set_acceleration_manager_enabled(uint8_t enabled) {
  if ((!!acceleration_manager_enabled) != (!!enabled)) {
    st_synchronize();
    acceleration_manager_enabled = !!enabled;
  }
}

void plan_clear_buffer(){
  while(plan_get_current_block()!=NULL) plan_discard_current_block();
}

int plan_is_acceleration_manager_enabled() {
  return(acceleration_manager_enabled);
}

void plan_discard_current_block() {
  if (block_buffer_head != block_buffer_tail) {
    block_buffer_tail = next_block_index( block_buffer_tail );
  }
}

block_t *plan_get_current_block() {
  if (block_buffer_head == block_buffer_tail) { return(NULL); }
  return(&block_buffer[block_buffer_tail]);
}

// Add a new Action movement to the buffer. x, y and z is the signed, absolute target position in 
// millimeters. Feed rate specifies the speed of the motion. 
void plan_buffer_line (tActionRequest *pAction)
{
  float x;
  float y;
  float z;
  float feed_rate;
  bool e_only = false;
  float speed_x, speed_y, speed_z, speed_e; // Nominal mm/minute for each axis  
    
  x = pAction->target.x;
  y = pAction->target.y;
  z = pAction->target.z;
  feed_rate = pAction->target.feed_rate;
  
  #ifdef READ_FILE_DEBUG
		printf("> ACTION type: %i target: (x:%f y:%f f:%f) power: %" SCNd16 "\n",pAction->ActionType,x,y,(float)feed_rate,pAction->param);
	#endif
	
  // hard clipping. Might implement correct clipping some day...
  // JAAP: temporary disabled clipping because it caused parts of the print 
  // to "disappear" outside the working area, even though they were still 
  // with x/y limits!  This needs fixing!
  //if ( 1000*x < cfg->xmin || 1000*x > cfg->xmax ) return;
  //if ( 1000*y < cfg->ymin || 1000*y > cfg->ymax ) return;
  //if ( 1000*z < cfg->zmin || 1000*z > cfg->zmax ) return;
  
  
  // printf("%f %f %f %f %f\n", x,y,z,(float)feed_rate); 
  // Calculate target position in absolute steps
  int32_t target[NUM_AXES];
  target[X_AXIS] = lround(x*(float)config.steps_per_mm_x);
  target[Y_AXIS] = lround(y*(float)config.steps_per_mm_y);
  target[Z_AXIS] = lround(z*(float)config.steps_per_mm_z);     
  target[E_AXIS] = lround(pAction->target.e*(float)config.steps_per_mm_e);     
  
  // Calculate the buffer head after we push this byte
  int next_buffer_head = next_block_index( block_buffer_head );    
  
  // If the buffer is full: good! That means we are well ahead of the robot. 
  // Rest here until there is room in the buffer.
  while(block_buffer_tail == next_buffer_head) { sleep_mode(); }
  
  // Prepare to set up new block
  block_t *block = &block_buffer[block_buffer_head];

  block->action_type = AT_MOVE;
  block->power = pAction->param;
  
  // Compute direction bits for this block
  block->direction_bits = 0;
  if (target[X_AXIS] < position[X_AXIS]) { block->direction_bits |= (1<<X_DIRECTION_BIT); }
  if (target[Y_AXIS] < position[Y_AXIS]) { block->direction_bits |= (1<<Y_DIRECTION_BIT); }
  if (target[Z_AXIS] < position[Z_AXIS]) { block->direction_bits |= (1<<Z_DIRECTION_BIT); }
  if (target[E_AXIS] < position[E_AXIS]) { block->direction_bits |= (1<<E_DIRECTION_BIT); }
  
  // Number of steps for each axis
  block->steps_x = labs(target[X_AXIS]-position[X_AXIS]);
  block->steps_y = labs(target[Y_AXIS]-position[Y_AXIS]);
  block->steps_z = labs(target[Z_AXIS]-position[Z_AXIS]);
  block->steps_e = labs(target[E_AXIS]-position[E_AXIS]);
  block->step_event_count = max(block->steps_x, max(block->steps_y, block->steps_z));
  block->step_event_count = max(block->step_event_count, block->steps_e);

  // Bail if this is a zero-length block
  if (block->step_event_count == 0) { return; };
  
  // Compute path vector in terms of absolute step target and current positions
  float delta_mm[NUM_AXES];
  delta_mm[X_AXIS] = (target[X_AXIS]-position[X_AXIS])/(float)config.steps_per_mm_x;
  delta_mm[Y_AXIS] = (target[Y_AXIS]-position[Y_AXIS])/(float)config.steps_per_mm_y;
  delta_mm[Z_AXIS] = (target[Z_AXIS]-position[Z_AXIS])/(float)config.steps_per_mm_z;
  delta_mm[E_AXIS] = (target[E_AXIS]-position[E_AXIS])/(float)config.steps_per_mm_e;
  block->millimeters = sqrt(square(delta_mm[X_AXIS]) + square(delta_mm[Y_AXIS]) + 
                            square(delta_mm[Z_AXIS]));
  if (block->millimeters == 0)
  {
    e_only = true;
    block->millimeters = fabs(delta_mm[E_AXIS]);
  }
  float inverse_millimeters = 1.0/block->millimeters;  // Inverse millimeters to remove multiple divides    
  
//
// Speed limit code from Marlin firmware
//
  float microseconds;
  //if(feedrate<minimumfeedrate)
  //  feedrate=minimumfeedrate;
  microseconds = lround((block->millimeters/feed_rate*60.0)*1000000.0);

  // Calculate speed in mm/minute for each axis
  float multiplier = 60.0*1000000.0/(float)microseconds;
  speed_x = delta_mm[X_AXIS] * multiplier;
  speed_y = delta_mm[Y_AXIS] * multiplier;
  speed_z = delta_mm[Z_AXIS] * multiplier;
  speed_e = delta_mm[E_AXIS] * multiplier;

  // Limit speed per axis
  float speed_factor = 1; //factor <=1 do decrease speed
  if(fabs(speed_x) > config.maximum_feedrate_x) 
  {
    speed_factor = (float)config.maximum_feedrate_x / fabs(speed_x);
  }
  if(fabs(speed_y) > config.maximum_feedrate_y)
  {
    float tmp_speed_factor = (float)config.maximum_feedrate_y / fabs(speed_y);
    if(speed_factor > tmp_speed_factor) speed_factor = tmp_speed_factor;
  }
  if(fabs(speed_z) > config.maximum_feedrate_z)
  {
    float tmp_speed_factor = (float)config.maximum_feedrate_z / fabs(speed_z);
    if(speed_factor > tmp_speed_factor) speed_factor = tmp_speed_factor;
  }
  if(fabs(speed_e) > config.maximum_feedrate_e)
  {
    float tmp_speed_factor = (float)config.maximum_feedrate_e / fabs(speed_e);
    if(speed_factor > tmp_speed_factor) speed_factor = tmp_speed_factor;
  }

  multiplier = multiplier * speed_factor;
  speed_x = delta_mm[X_AXIS] * multiplier;
  speed_y = delta_mm[Y_AXIS] * multiplier;
  speed_z = delta_mm[Z_AXIS] * multiplier;
  speed_e = delta_mm[E_AXIS] * multiplier;
  block->nominal_speed = block->millimeters * multiplier;    // mm per min
  block->nominal_rate = ceil(block->step_event_count * multiplier);   // steps per minute

  
  // Compute the acceleration rate for the trapezoid generator. Depending on the slope of the line
  // average travel per step event changes. For a line along one axis the travel per step event
  // is equal to the travel/step in the particular axis. For a 45 degree line the steppers of both
  // axes might step for every step event. Travel per step event is then sqrt(travel_x^2+travel_y^2).
  // To generate trapezoids with contant acceleration between blocks the rate_delta must be computed 
  // specifically for each line to compensate for this phenomenon:
  // Convert universal acceleration for direction-dependent stepper rate change parameter
  block->rate_delta = ceil( block->step_event_count*inverse_millimeters *  
        config.acceleration*60.0 / ACCELERATION_TICKS_PER_SECOND ); // (step/min/acceleration_tick)

  // Perform planner-enabled calculations
  if (acceleration_manager_enabled  ) 
  {  
    // Compute path unit vector                            
    float unit_vec[NUM_AXES];

    unit_vec[X_AXIS] = delta_mm[X_AXIS]*inverse_millimeters;
    unit_vec[Y_AXIS] = delta_mm[Y_AXIS]*inverse_millimeters;
    unit_vec[Z_AXIS] = delta_mm[Z_AXIS]*inverse_millimeters;  
  
    // Compute maximum allowable entry speed at junction by centripetal acceleration approximation.
    // Let a circle be tangent to both previous and current path line segments, where the junction 
    // deviation is defined as the distance from the junction to the closest edge of the circle, 
    // colinear with the circle center. The circular segment joining the two paths represents the 
    // path of centripetal acceleration. Solve for max velocity based on max acceleration about the
    // radius of the circle, defined indirectly by junction deviation. This may be also viewed as 
    // path width or max_jerk in the previous grbl version. This approach does not actually deviate 
    // from path, but used as a robust way to compute cornering speeds, as it takes into account the
    // nonlinearities of both the junction angle and junction velocity.
    float vmax_junction = MINIMUM_PLANNER_SPEED; // Set default max junction speed

    // Skip first block or when previous_nominal_speed is used as a flag for homing and offset cycles.
    if ((block_buffer_head != block_buffer_tail) && (previous_nominal_speed > 0.0)) {
      // Compute cosine of angle between previous and current path. (prev_unit_vec is negative)
      // NOTE: Max junction velocity is computed without sin() or acos() by trig half angle identity.
      float cos_theta = - previous_unit_vec[X_AXIS] * unit_vec[X_AXIS] 
                         - previous_unit_vec[Y_AXIS] * unit_vec[Y_AXIS] 
                         - previous_unit_vec[Z_AXIS] * unit_vec[Z_AXIS] ;
                           
      // Skip and use default max junction speed for 0 degree acute junction.
      if (cos_theta < 0.95) {
        vmax_junction = min(previous_nominal_speed,block->nominal_speed);
        // Skip and avoid divide by zero for straight junctions at 180 degrees. Limit to min() of nominal speeds.
        if (cos_theta > -0.95) {
          // Compute maximum junction velocity based on maximum acceleration and junction deviation
          float sin_theta_d2 = sqrt(0.5*(1.0-cos_theta)); // Trig half angle identity. Always positive.
          vmax_junction = min(vmax_junction,
            sqrt(config.acceleration*60*60 * config.junction_deviation * sin_theta_d2/(1.0-sin_theta_d2)) );
        }
      }
    }
    block->max_entry_speed = vmax_junction;
    
    // Initialize block entry speed. Compute based on deceleration to user-defined MINIMUM_PLANNER_SPEED.
    float v_allowable = max_allowable_speed(-config.acceleration,MINIMUM_PLANNER_SPEED,block->millimeters);
    block->entry_speed = min(vmax_junction, v_allowable);

    // Initialize planner efficiency flags
    // Set flag if block will always reach maximum junction speed regardless of entry/exit speeds.
    // If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
    // the current block and next block junction speeds are guaranteed to always be at their maximum
    // junction speeds in deceleration and acceleration, respectively. This is due to how the current
    // block nominal speed limits both the current and next maximum junction speeds. Hence, in both
    // the reverse and forward planners, the corresponding block junction speed will always be at the
    // the maximum junction speed and may always be ignored for any speed reduction checks.
    if (block->nominal_speed <= v_allowable) { block->nominal_length_flag = true; }
    else { block->nominal_length_flag = false; }
    block->recalculate_flag = true; // Always calculate trapezoid for new block
  
    // Update previous path unit_vector and nominal speed
    memcpy(previous_unit_vec, unit_vec, sizeof(unit_vec)); // previous_unit_vec[] = unit_vec[]
    previous_nominal_speed = block->nominal_speed;

  } else {
    // Acceleration planner disabled. Set minimum that is required.
    //  block->entry_speed = block->nominal_speed;
    block->initial_rate = block->nominal_rate;
    block->final_rate = block->nominal_rate;
    block->accelerate_until = 0;
    block->decelerate_after = block->step_event_count;
    block->rate_delta = 0;
  }
 
 // check action options 
  block->check_endstops = (pAction->ActionType == AT_MOVE_ENDSTOP);
  if (  pAction->ActionType == AT_LASER ) 
    block->options = OPT_LASER_ON;
  else if (  pAction->ActionType == AT_BITMAP ) 
    block->options = OPT_BITMAP;
  else
    block->options = 0;
  
  // now that the options are set: make this a MOVE action.
  pAction->ActionType = AT_MOVE;
  
  // Move buffer head
  block_buffer_head = next_buffer_head;     
  // Update position
  memcpy(position, target, sizeof(target)); // position[] = target[]

  startpoint = pAction->target;
  
  if (acceleration_manager_enabled) { planner_recalculate(); }  
  st_wake_up();
}


// push a wait (dwell) in the motion queue
void plan_buffer_wait (tActionRequest *pAction)
{
    
  // Calculate the buffer head after we push this block
  int next_buffer_head = next_block_index( block_buffer_head );    
  
  // If the buffer is full: good! That means we are well ahead of the robot. 
  // Rest here until there is room in the buffer.
  while(block_buffer_tail == next_buffer_head) { sleep_mode(); }
  
  // Prepare to set up new block
  block_t *block = &block_buffer[block_buffer_head];
  
  //TODO
  
  block->action_type = pAction->ActionType;
  // every 50ms
  block->millimeters = 10;
  block->nominal_speed = 600;
  block->nominal_rate = 20*60;
  
  block->step_event_count = 1000;
  
    // Acceleration planner disabled. Set minimum that is required.
    block->entry_speed = block->nominal_speed;
    
    block->initial_rate = block->nominal_rate;
    block->final_rate = block->nominal_rate;
    block->accelerate_until = 0;
    block->decelerate_after = block->step_event_count;
    block->rate_delta = 0;
    
  // Move buffer head
  block_buffer_head = next_buffer_head;     

  if (acceleration_manager_enabled) { planner_recalculate(); }  
  st_wake_up();    
}

// Enqueue an action. Either move, laser, endstop or wait. 
void plan_buffer_action(tActionRequest *pAction)
{
  switch (pAction->ActionType)
  {
  case AT_MOVE:
  case AT_LASER:
  case AT_BITMAP:
  case AT_MOVE_ENDSTOP:
    plan_buffer_line (pAction);
    break;    
  case AT_WAIT:
    plan_buffer_wait (pAction);
    break;
  }
}

// Reset the planner position vector and planner speed
void plan_get_current_position_xyz(float *x, float *y, float *z)
{
  *x = actpos_x / config.steps_per_mm_x;
  *y = actpos_y / config.steps_per_mm_y;
  *z = actpos_z / config.steps_per_mm_z;
}


// Reset the planner position vector and planner speed
void plan_set_current_position_xyz(float x, float y, float z)
{
  tTarget new_pos = startpoint;
  new_pos.x = x;
  new_pos.y = y;
  new_pos.z = z;
  plan_set_current_position (&new_pos);
}

// Set absolute position
void plan_set_current_position(tTarget *new_position) 
{
  startpoint = *new_position;
  position[X_AXIS] = lround(new_position->x*(float)config.steps_per_mm_x);
  position[Y_AXIS] = lround(new_position->y*(float)config.steps_per_mm_y);
  position[Z_AXIS] = lround(new_position->z*(float)config.steps_per_mm_z);    
  position[E_AXIS] = lround(new_position->e*(float)config.steps_per_mm_e);    
  previous_nominal_speed = 0.0; // Resets planner junction speeds. Assumes start from rest.
  clear_vector_double(previous_unit_vec);
  printf("Set Position: %d,%d,%d,%d", position[X_AXIS],  position[Y_AXIS],  position[Z_AXIS],  position[E_AXIS]);
  // Wait for all motion to stop and THEN set the actual stepper axis positions;
  // while( !mot->ready() );
  actpos_x =  position[X_AXIS];
  actpos_y =  position[Y_AXIS];
  actpos_z =  position[Z_AXIS];
  actpos_e =  position[E_AXIS];
}

// Force the feedrate
//void plan_set_feed_rate (tTarget *new_position) 
//{
// startpoint.feed_rate = new_position->feed_rate;
//}

// return true if queue is filled
uint8_t plan_queue_full (void)
{
  int next_buffer_head = next_block_index( block_buffer_head );    
  
  if (block_buffer_tail == next_buffer_head)
    return 1;
  else
    return 0;
}

// Return true if queue is empty
uint8_t plan_queue_empty(void) 
{
  if (block_buffer_head == block_buffer_tail)
    return 1;
  else
    return 0;
}

// Return nr of items in the queue
uint8_t plan_queue_items(void) 
{
 // BLOCK_BUFFER_SIZE;
  int len =  block_buffer_head - block_buffer_tail;
  if ( len < 0 ) len = -len;
  return len;
}


