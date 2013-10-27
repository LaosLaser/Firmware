/*
  planner.h - buffers movement commands and manages the acceleration profile plan
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

#ifndef planner_h
#define planner_h
                 
#include <inttypes.h>

typedef enum {
  AT_MOVE,         // move with laser off
  AT_LASER,        // move with laser on
  AT_BITMAP,       // move with laser modulated by bitmap
  AT_MOVE_ENDSTOP, // move to endstops
  AT_WAIT,         // Dwell
} eActionType;
        
typedef  struct {
  float  x;
  float  y;
  float  z;
  float  e;
  float  feed_rate;
} tTarget;

// options for the action
#define OPT_LASER_ON  1 // switch laser on
#define OPT_WAIT      2 // wait at end of move
#define OPT_HOME_X    4 // stop on x switch
#define OPT_HOME_Y    8
#define OPT_HOME_Z   16
#define OPT_HOME_E   32
#define OPT_BITMAP   64 // bitmap mark a line


// This struct is used when buffering the setup for each linear movement "nominal" values are as specified in 
// the source g-code and may never actually be reached if acceleration management is active.
typedef struct 
{
  eActionType action_type;
  
  // Fields used by the bresenham algorithm for tracing the line
  uint32_t steps_x, steps_y, steps_z; // Step count along each axis
  uint32_t steps_e; 
  uint32_t direction_bits;            // The direction bit set for this block (refers to *_DIRECTION_BIT in config.h)
  int32_t  step_event_count;          // The number of step events required to complete this block
  uint32_t nominal_rate;              // The nominal step rate for this block in step_events/minute
  
  // Fields used by the motion planner to manage acceleration
  float nominal_speed;               // The nominal speed for this block in mm/min  
  float entry_speed;                 // Entry speed at previous-current junction in mm/min
  float max_entry_speed;             // Maximum allowable junction entry speed in mm/min
  float millimeters;                 // The total travel of this block in mm
  uint8_t recalculate_flag;           // Planner flag to recalculate trapezoids on entry junction
  uint8_t nominal_length_flag;        // Planner flag for nominal speed always reached

  // Settings for the trapezoid generator
  uint32_t initial_rate;              // The jerk-adjusted step rate at start of block  
  uint32_t final_rate;                // The minimal rate at exit
  int32_t rate_delta;                 // The steps/minute to add or subtract when changing speed (must be positive)
  uint32_t accelerate_until;          // The index of the step event on which to stop acceleration
  uint32_t decelerate_after;          // The index of the step event on which to start decelerating
  
  // extra
  uint8_t check_endstops; // for homing moves
  uint8_t options; // for further options (e.g. laser on/off, homing on axis, dwell, etc)  
  uint16_t power; // laser power setpoint
} block_t;

// This defines an action to enque, with its target position
typedef struct {
  eActionType ActionType;
  tTarget     target;  
  uint16_t    param; // argument for the action
} tActionRequest;



extern tTarget startpoint;
      
// Initialize the motion plan subsystem      
void plan_init();

// Add a new linear movement to the buffer. x, y and z is the signed, absolute target position in 
// millimeters. Feed rate specifies the speed of the motion. (in mm/min) 
void plan_buffer_line (tActionRequest *pAction);

void plan_buffer_action(tActionRequest *pAction);

// Called when the current block is no longer needed. Discards the block and makes the memory
// availible for new blocks.
void plan_discard_current_block();

// Gets the current block. Returns NULL if buffer empty
block_t *plan_get_current_block();

// Enables or disables acceleration-management for upcoming blocks
void plan_set_acceleration_manager_enabled(uint8_t enabled);

// Clear Buffers
void plan_clear_buffer();

// Is acceleration-management currently enabled?
int plan_is_acceleration_manager_enabled();

// Reset the position vector
void plan_set_current_position(tTarget *new_position); 

void plan_set_current_position_xyz(float x, float y, float z); 
void plan_get_current_position_xyz(float *x, float *y, float *z); 

void plan_set_feed_rate (tTarget *new_position);

uint8_t plan_queue_full (void);

uint8_t plan_queue_empty(void);

uint8_t plan_queue_items(void) ;

#endif
