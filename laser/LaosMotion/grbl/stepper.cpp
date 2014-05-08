/*
  stepper.c - stepper motor driver: executes motion plans using stepper motors
  Part of Grbl
  Taken from R2C2 project and modified for Laos by Peter Brier
  stripped, included some Marlin changes from Erik van de Zalm

  Copyright (c) 2009-2011 Simen Svale Skogsrud
  Modifications Copyright (c) 2011 Sungeun K. Jeon
  Modifications Copyright (c) 2011 Peter Brier

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, orc
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warrlaanty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

/* The timer calculations of this module informed by the 'RepRap cartesian firmware' by Zack Smith
   and Philipp Tiefenbacher. */

/* The acceleration profiles are derived using the techniques descibed in the paper
  "Generate stepper-motor speed profiles in real time" David Austin 2004
   published at http://www.eetimes.com/design/embedded/4006438/Generate-stepper-motor-speed-profiles-in-real-time and others.
*/
#include <LaosMotion.h>
#include "pins.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "fixedpt.h"
#include "stepper.h"
#include "config.h"
#include "planner.h"


#define TICKS_PER_MICROSECOND (1) // Ticker uses 1usec units
// #define CYCLES_PER_ACCELERATION_TICK ((TICKS_PER_MICROSECOND*1000000)/ACCELERATION_TICKS_PER_SECOND)
#define STEP_TIMER_FREQ 1000000 // 1 MHz

// types: ramp state
typedef enum {RAMP_UP, RAMP_MAX, RAMP_DOWN} tRamp;

// Prototypes
static void st_interrupt ();
static void set_step_timer (uint32_t cycles);
static void st_go_idle();

// Globals
volatile unsigned char busy = 0;
volatile int32_t actpos_x, actpos_y, actpos_z, actpos_e; // actual position

// Locals
static block_t *current_block;  // A pointer to the block currently being traced
static Ticker timer; // the periodic timer used to step
static tFixedPt pwmofs; // the offset of the PWM value
static tFixedPt pwmscale; // the scaling of the PWM value
static volatile int running = 0;  // stepper irq is running

static uint32_t direction_inv;    // invert mask for direction bits
static uint32_t direction_bits;   // all axes direction (different ports)
static uint32_t step_bits;        // all axis step bits
static uint32_t step_inv;      // invert mask for the stepper bits
static int32_t counter_x,       // Counter variables for the bresenham line tracer
               counter_y,
               counter_z;
static int32_t counter_e, counter_l, pos_l; // extruder and laser
static uint32_t step_events_completed; // The number of step events executed in the current block

// Variables used by the trapezoid generation
//static uint32_t cycles_per_step_event;        // The number of machine cycles between each step event
static uint32_t trapezoid_tick_cycle_counter; // The cycles since last trapezoid_tick. Used to generate ticks at a steady
                                              // pace without allocating a separate timer
static uint32_t trapezoid_adjusted_rate;      // The current rate of step_events according to the trapezoid generator

static tFixedPt  c;          // current clock cycle count [1/speed]
static int32_t   c_min;      // minimal clock cycle count [at vnominal for this block]
static int32_t   n;
static int32_t   decel_n;
static tRamp     ramp;        // state of state machine for ramping up/down

extern unsigned char bitmap_bpp;
extern unsigned long bitmap[], bitmap_width, bitmap_size;


//         __________________________
//        /|                        |\     _________________         ^
//       / |                        | \   /|               |\        |
//      /  |                        |  \ / |               | \       s
//     /   |                        |   |  |               |  \      p
//    /    |                        |   |  |               |   \     e
//   +-----+------------------------+---+--+---------------+----+    e
//   |               BLOCK 1            |      BLOCK 2          |    d
//
//                           time ----->
//
//  The trapezoid is the shape the speed curve over time. It starts at block->initial_rate, accelerates by block->rate_delta
//  during the first block->accelerate_until step_events_completed, then keeps going at constant speed until
//  step_events_completed reaches block->decelerate_after after which it decelerates until the trapezoid generator is reset.
//  The slope of acceleration is always +/- block->rate_delta and is applied at a constant rate following the midpoint rule
//  by the trapezoid generator, which is called ACCELERATION_TICKS_PER_SECOND times per second.



// Initialize and start the stepper motor subsystem
void st_init(void)
{
  direction_inv =
   (cfg->xscale<0 ? (1<<X_DIRECTION_BIT) : 0) |
   (cfg->yscale<0 ? (1<<Y_DIRECTION_BIT) : 0) |
   (cfg->zscale<0 ? (1<<Z_DIRECTION_BIT) : 0) |
   (cfg->escale<0 ? (1<<E_DIRECTION_BIT) : 0);
  step_inv =
   (cfg->xinv ? (1<<X_STEP_BIT) : 0) |
   (cfg->yinv ? (1<<Y_STEP_BIT) : 0) |
   (cfg->zinv ? (1<<Z_STEP_BIT) : 0) |
   (cfg->einv ? (1<<E_STEP_BIT) : 0);
 
  printf("Direction: %lu\n", direction_inv);
  pwmofs = to_fixed(cfg->pwmmin) / 100; // offset (0 .. 1.0)
  if ( cfg->pwmmin == cfg->pwmmax )
    pwmscale = 0;
  else
    pwmscale = div_f(to_fixed(cfg->pwmmax - cfg->pwmmin), to_fixed(100) );
  printf("ofs: %lu, scale: %lu\n", pwmofs, pwmscale);
  actpos_x = actpos_y = actpos_z = actpos_e = 0;
  st_wake_up();
  trapezoid_tick_cycle_counter = 0;
  st_go_idle();  // Start in the idle state
}

// output the direction bits to the appropriate output pins
static inline void  set_direction_pins (void)
{
  xdir = ( (direction_bits & (1<<X_DIRECTION_BIT))? 0 : 1 );
  ydir = ( (direction_bits & (1<<Y_DIRECTION_BIT))? 0 : 1 );
  zdir = ( (direction_bits & (1<<Z_DIRECTION_BIT))? 0 : 1 );
  // edir = ( (direction_bits & (1<<E_DIRECTION_BIT))?0:1);
}

// output the step bits on the appropriate output pins
static inline void  set_step_pins (uint32_t bits)
{
  xstep = ( (bits & (1<<X_STEP_BIT))?1:0 );
  ystep = ( (bits & (1<<Y_STEP_BIT))?1:0 );
  zstep = ( (bits & (1<<Z_STEP_BIT))?1:0 );
 // estep = ( (bits & (1<<E_STEP_BIT))?1:0 );
}

// unstep all stepper pins (output low)
static inline void  clear_all_step_pins (void)
{

  xstep =( (step_inv & (1<<X_STEP_BIT)) ? 1 : 0 );
  ystep =( (step_inv & (1<<Y_STEP_BIT)) ? 1 : 0 );
  zstep =( (step_inv & (1<<Z_STEP_BIT)) ? 1 : 0 );
  // estep =( (step_inv & (1<<E_STEP_BIT)) ? 0 : 1 );
}


// check home sensor
int hit_home_stop_x(int axis)
{
  return 1;
}
// check home sensor
int hit_home_stop_y(int axis)
{
  return 1;
}
// check home sensor
int hit_home_stop_z(int axis)
{
  return 1;
}

// Start stepper again from idle state, starts the step timer at a default rate
void st_wake_up()
{
  if ( ! running )
  {
    running = 1;
    set_step_timer(2000);
  //  printf("wake_up()..\n");
  }
}

// When not stepping, go to idle mode. Steppers can be switched off, or set to reduced current
// (some delay might have to be implemented). Currently no motor switchoff is done.
static void st_go_idle()
{
  timer.detach();
  running = 0;
  clear_all_step_pins();
  *laser = LASEROFF;
  pwm = cfg->pwmmax / 100.0;  // set pwm to max;
//  printf("idle()..\n");
}

// return number of steps to perform:  n = (v^2) / (2*a)
// alpha is a adjustment factor for ?? (alsways 1! in this source)
static inline int32_t calc_n (float speed, float alpha, float accel)
{
  return speed * speed / (2.0 * alpha * accel);
}

// Initializes the trapezoid generator from the current block. Called whenever a new
// block begins. Calculates the length ofc the block (in events), step rate, slopes and trigger positions (when to accel, decel, etc.)
static inline void trapezoid_generator_reset()
{
  tFixedPt  c0;
#define alpha (1.0)

//  float    alpha = 1.0;
  float    accel;
  int32_t   accel_until;
  int32_t   decel_after;

  accel = current_block->rate_delta*ACCELERATION_TICKS_PER_SECOND / 60.0;

  c0 = (float)STEP_TIMER_FREQ * sqrt (2.0*alpha/accel);
  n = calc_n (current_block->initial_rate/60.0, alpha, accel);
  if (n==0)
  {
    n = 1;
    c = c0*0.676;
  }
  else
  {
    c = c0 * (sqrt(n+1.0)-sqrt((float)n));
  }

  ramp = RAMP_UP;

  accel_until = calc_n (current_block->nominal_rate/60.0, alpha, accel);
  c_min = c0 * (sqrt(accel_until+1.0)-sqrt((float)accel_until));
  accel_until = accel_until - n;

  decel_n = - calc_n (current_block->nominal_rate/60.0, alpha, accel);
  decel_after = current_block->step_event_count + decel_n + calc_n (current_block->final_rate/60.0, alpha, accel);

  if (decel_after < accel_until)
  {
    decel_after = (decel_after + accel_until) / 2;
    decel_n  = decel_after - current_block->step_event_count - calc_n (current_block->final_rate/60.0, alpha, accel);
  }
  current_block->decelerate_after = decel_after;

  c = to_fixed(c);
  c_min = to_fixed (c_min);
#undef alpha
}


// get step rate (steps/min) from time cycles
//static inline uint32_t get_step_rate (uint64_t cycles)
//{
//  return (TICKS_PER_MICROSECOND*1000000*6) / cycles * 10;
//}

// Set the step timer. Note: this starts the ticker at an interval of "cycles"
static inline void set_step_timer (uint32_t cycles)
{
   volatile static double p;
   timer.attach_us(&st_interrupt,cycles);
   // p = to_double(pwmofs + mul_f( pwmscale, ((power>>6) * c_min) / ((10000>>6)*cycles) ) );
   // p = ( to_double(c_min) * current_block->power) / ( 10000.0 * (double)cycles);
  // p = (60E6/nominal_rate) / cycles; // nom_rate is steps/minute,
   //printf("%f,%f,%f\n\r", (float)(60E6/nominal_rate), (float)cycles, (float)p);
  // printf("%d: %f %f\n\r", (int)current_block->power, (float)p, (float)c_min/(float(c) ));
   p = (double)(cfg->pwmmin/100.0 + ((current_block->power/10000.0)*((cfg->pwmmax - cfg->pwmmin)/100.0)));
   pwm = p;
}

// "The Stepper Driver Interrupt" - This timer interrupt is the workhorse of Grbl. It is  executed at the rate set with
// set_step_timer. It pops blocks from the block_buffer and executes them by pulsing the stepper pins appropriately.
// It is supported by The Stepper Port Reset Interrupt which it uses to reset the stepper port after each pulse.
// The bresenham line tracer algorithm controls all three stepper outputs simultaneously with these two interrupts.
static  void st_interrupt (void)
{
  // TODO: Check if the busy-flag can be eliminated by just disabeling this interrupt while we are in it

  if(busy){ /*printf("busy!\n"); */ return; } // The busy-flag is used to avoid reentering this interrupt
  busy = 1;

  // Set the direction pins a cuple of nanoseconds before we step the steppers
  //STEPPING_PORT = (STEPPING_PORT & ~DIRECTION_MASK) | (out_bits & DIRECTION_MASK);
   // set_direction_pins (out_bits);

  // Then pulse the stepping pins
  //STEPPING_PORT = (STEPPING_PORT & ~STEP_MASK) | out_bits;
  // led2 = 1;
  set_step_pins (step_bits ^ step_inv);

  // If there is no current block, attempt to pop one from the buffer
  if (current_block == NULL)
  {
    // Anything in the buffer?
    current_block = plan_get_current_block();
    if (current_block != NULL) {
      trapezoid_generator_reset();
      counter_x = -(current_block->step_event_count >> 1);
      counter_y = counter_x;
      counter_z = counter_x;
      counter_e = counter_x;
      counter_l = counter_x;
      pos_l = 0; // reset laser bitmap counter
      step_events_completed = 0;
      direction_bits = current_block->direction_bits ^ direction_inv;
      set_direction_pins ();
      step_bits = 0;
    }
    else
    {
      st_go_idle();
    }
  }

  // process the current block
  if (current_block != NULL)
  {

   // this block is a bitmap engraving line, read laser on/off status from buffer
   if ( current_block->options & OPT_BITMAP )
   {
      *laser =  ! (bitmap[pos_l / 32] & (1 << (pos_l % 32)));
      counter_l += bitmap_width;
     //  printf("%d %d %d: %d %d %c\n\r", bitmap_width, pos_l, counter_l,  pos_l / 32, pos_l % 32, (*laser ?  '1' : '0' ));
      if (counter_l > 0)
      {
        counter_l -= current_block->step_event_count;
     //   putchar ( (*laser ?  '1' : '0' ) );
        pos_l++;
      }
   }
   else
   {
     *laser = ( current_block->options & OPT_LASER_ON ? LASERON : LASEROFF);
   }

    if (current_block->action_type == AT_MOVE)
    {
      // Execute step displacement profile by bresenham line algorithm
      step_bits = 0;
      counter_x += current_block->steps_x;
      if (counter_x > 0) {
        actpos_x +=  ( (current_block->direction_bits & (1<<X_DIRECTION_BIT))? -1 : 1 );
        step_bits |= (1<<X_STEP_BIT);
        counter_x -= current_block->step_event_count;
      }
      counter_y += current_block->steps_y;
      if (counter_y > 0) {
        actpos_y +=  ( (current_block->direction_bits & (1<<Y_DIRECTION_BIT))? -1 : 1 );
        step_bits |= (1<<Y_STEP_BIT);
        counter_y -= current_block->step_event_count;
      }
      counter_z += current_block->steps_z;
      if (counter_z > 0) {
        actpos_z +=  ( (current_block->direction_bits & (1<<Z_DIRECTION_BIT))? -1 : 1 );
          step_bits |= (1<<Z_STEP_BIT);
        counter_z -= current_block->step_event_count;
      }

      counter_e += current_block->steps_e;
      if (counter_e > 0) {
        actpos_e +=  ( (current_block->direction_bits & (1<<E_DIRECTION_BIT))? -1 : 1 );
        step_bits |= (1<<E_STEP_BIT);
        counter_e -= current_block->step_event_count;
      }

      //clear_step_pins (); // clear the pins, assume that we spend enough CPU cycles in the previous statements for the steppers to react (>1usec)
      step_events_completed++; // Iterate step events

      // This is a homing block, keep moving until all end-stops are triggered
      if (current_block->check_endstops)
      {
        if ( (current_block->steps_x && hit_home_stop_x (direction_bits & (1<<X_DIRECTION_BIT)) ) ||
             (current_block->steps_y && hit_home_stop_y (direction_bits & (1<<Y_DIRECTION_BIT)) ) ||
             (current_block->steps_z && hit_home_stop_z (direction_bits & (1<<Z_DIRECTION_BIT)) )
           )
        {
          step_events_completed = current_block->step_event_count;
          step_bits = 0;
        }
      }


      // While in block steps, update acceleration profile
      if (step_events_completed < current_block->step_event_count)
      {
        tFixedPt new_c;

        switch (ramp)
        {
          case RAMP_UP:
          {
            new_c = c - (c<<1) / (4*n+1);
            if (step_events_completed >= current_block->decelerate_after)
            {
              ramp = RAMP_DOWN;
              n = decel_n;
            }
            else if (new_c <= c_min)
            {
              new_c = c_min;
              ramp = RAMP_MAX;
            }

            if (to_int(new_c) != to_int(c))
            {
              set_step_timer (to_int(new_c));
            }
            c = new_c;
          }
          break;

          case RAMP_MAX:
            if (step_events_completed >= current_block->decelerate_after)
            {
              ramp = RAMP_DOWN;
              n = decel_n;
            }
          break;

          case RAMP_DOWN:
            new_c = c - (c<<1) / (4*n+1);
            if (to_int(new_c) != to_int(c))
            {
              set_step_timer (to_int(c));
            }
            c = new_c;
          break;
        }

        n++;
      } else {
        // If current block is finished, reset pointer
        current_block = NULL;
        plan_discard_current_block();
      }
    }
  }
  else
  {
    // Still no block? Set the stepper pins to low before sleeping.
    // printf("block == NULL\n");
    step_bits = 0;
  }

  clear_all_step_pins (); // clear the pins, assume that we spend enough CPU cycles in the previous statements for the steppers to react (>1usec)
  busy=0;

}


// Block until all buffered steps are executed
void st_synchronize()
{
  while(plan_get_current_block()) { sleep_mode(); }
}

