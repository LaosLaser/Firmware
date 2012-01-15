/**
 * LaosMotion.cpp
 * Motion Controll functions for Laos system
 *
 * Copyright (c) 2011 Peter Brier
 *
 *   This file is part of the LaOS project (see: http://laoslaser.org)
 *
 *   LaOS is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   LaOS is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with LaOS.  If not, see <http://www.gnu.org/licenses/>.
 *
 * 
 *
 */
#include "global.h"
#include "LaosMotion.h"
#include  "planner.h"
#include  "stepper.h"
#include  "pins.h"

// #define DO_MOTION_TEST 1

// status leds
extern DigitalOut led1,led2,led3,led4;

// Inputs;
DigitalIn xhome(p8);
DigitalIn yhome(p17);
DigitalIn zmin(p15);
DigitalIn zmax(p16);

// motors
DigitalOut enable(p7);
DigitalOut xdir(p23);
DigitalOut xstep(p24);
DigitalOut ydir(p25);
DigitalOut ystep(p26);
DigitalOut zdir(p27);
DigitalOut zstep(p28);
DigitalOut estep(p29); // NOK: CAN, (TODO)
DigitalOut edir(p30);  // NOK: CAN, (TODO)


// laser
PwmOut pwm(p22);                // O1: PWM (Yellow)
DigitalOut laser_enable(p21);   // O2: enable laser
DigitalOut o3(p6);              // 03: NC
DigitalOut laser(p5);           // O4: (p5) LaserON (White)

// Analog in/out (cover sensor) + NC
DigitalIn cover(p19);

  
// globals
int step=0, command=0;
int mark_speed = 100; // 100 [mm/sec] 

// next planner action to enqueue
tActionRequest  action; 

// axis invert flags

// Command interpreter
int param=0, val=0;



/**
*** LaosMotion() Constructor
*** Make new motion object
**/
LaosMotion::LaosMotion()
{
#if DO_MOTION_TEST
  tActionRequest act[2];
  int i=0;
  Timer t;
#endif
  pwm.period(1.0 / cfg->pwmfreq);
  pwm = cfg->pwmmin/100.0;

  mark_speed = cfg->speed;
  //start.mode(PullUp);
  xhome.mode(PullUp);
  yhome.mode(PullUp);
  isHome = false;
  plan_init();
  st_init();  
  reset();
  mark_speed = cfg->speed;
  action.param = 0;
  action.target.x = action.target.y = action.target.z = action.target.e =0;
  action.target.feed_rate = 60*mark_speed;

#if DO_MOTION_TEST
  t.start();
  act[0].ActionType = act[1].ActionType =  AT_MOVE;
  act[0].target.feed_rate = 60 * 100;
  act[1].target.feed_rate = 60 * 200;
  act[0].target.x = act[0].target.y = act[0].target.z = act[0].target.e = 0;
  act[1].target.x = act[1].target.y = act[1].target.z = act[1].target.e = 100;
  act[1].target.y = 200;
  while(1)
  {
    while( plan_queue_full() ) led3 = !led3;
    led1 = 1;
    i++;
    if ( i )
      printf("%d PING...\n", t.read_ms());
    else
      printf("%d PONG...\n", t.read_ms());
    if ( i > 1 || i<0) i = 0;
    plan_buffer_line (&act[i]);
    led1 = 0;
  }
#endif
  
}


/**
***Destructor
**/
LaosMotion::~LaosMotion()
{

}


/**
*** reset()
*** reset the state
**/
void LaosMotion::reset()
{
  step = command = xstep = xdir = ystep = ydir = zstep = zdir = 0;
  laser = LASEROFF;
  enable = cfg->enable;
  cover.mode(PullUp);
}



/**
*** ready()
*** ready to receive new commands 
**/
int LaosMotion::ready()
{
  return !plan_queue_full();
}

/** 
*** MoveTo()
**/
void LaosMotion::moveTo(int x, int y, int z)
{
   action.target.x = x/1000.0;
   action.target.y = y/1000.0;
   action.target.z = z/1000.0;
   action.ActionType = AT_MOVE;
   action.target.feed_rate =  60.0 * cfg->speed;
   plan_buffer_line(&action);
}



/**
*** write()
*** Write command and parameters to motion controller 
**/
void LaosMotion::write(int i)
{
  static int x,y,z,power=10000;
  //if (  plan_queue_empty() ) 
  //printf("Empty\n");
  if ( step == 0 )
  {
    command = i;
    step++;
  }
  else
  { 
     switch( command )
     {
          case 0: // move x,y (laser off) 
          case 1: // line x,y (laser on)
            switch ( step )
            {
              case 1:
                action.target.x = i/1000.0; 
                break;
              case 2: 
                action.target.y = i/1000.0;;
                step=0;
                action.param = power;
                action.ActionType =  (command ? AT_LASER : AT_MOVE);
                action.target.feed_rate =  60.0 * (command ? mark_speed : cfg->speed );
                plan_buffer_line(&action);
				// printf("cmd: %d %f %f\n\r", command, action.target.x, action.target.y);
                break;
            } 
            break;
          case 2: // move z 
            switch(step)
            {
              case 1: 
                z = i; 
                step = 0; 
                break;
            }
         case 4: // set x,y,z (absolute)
           switch ( step )
            {
              case 1: 
                x = i; 
                break;
              case 2: 
                y = i; 
                break;
              case 3: 
                z = i;
                setPosition(x,y,z); 
                step=0; 
                break;
            }
         case 5: // nop
           step = 0;
           break;
         case 7: // set index,value
            switch ( step )
            {
              case 1: 
                param = i; 
                break;
              case 2: val = i; step = 0; 
              switch( param )
              {
                case 100: 
                  if ( val < 1 ) val = 1;
                  if ( val > 9999 ) val = 10000; 
                  mark_speed = val * cfg->speed / 10000; 
                  break;  
                case 101: 
                  power = val;
                  printf("power: %d\n", power); 
                  step=0; 
                  break;                  
              }
              break;
            }
           break; 
         default: // I do not understand: stop motion
            step = 0;
          break;
    }
    if ( step ) step++;
  } 
}


/**
*** Return true if start button is pressed
**/
bool LaosMotion::isStart()
{
  return cover;
}


/**
*** Hard set the absolute position
*** Warning: only call when the motion is not busy!
**/
void LaosMotion::setPosition(int x, int y, int z)
{
  plan_set_current_position_xyz(x/1000.0,y/1000.0,z/1000.0);
}

/**
*** get the absolute position
**/
void LaosMotion::getPosition(int *x, int *y, int *z)
{

}


/**
*** Home the axis, stop when button is pressed
**/
void LaosMotion::home(int x, int y)
{
  int i=0;
  printf("Homing %d,%d with speed %d\n", x, y, cfg->homespeed);
  xdir = cfg->xhomedir;
  ydir = cfg->yhomedir;
  zdir = cfg->zhomedir;
  led1 = 0;
  isHome = false;
  while ( 1 )
  {
    xstep = ystep = 0;
    wait(cfg->homespeed/1E6);  
    xstep = xhome ^ cfg->xpol;
    ystep = yhome ^ cfg->ypol;
    wait(cfg->homespeed/1E6);
    
    led2 = !xhome;
    led3 = !yhome;
    led4 = ((i++) & 0x10000);
    if ( !(xhome ^ cfg->xpol) && !(yhome ^ cfg->ypol) ) 
    {
      // setPosition(x,y,0);
      setPosition(x,y,0);
      isHome = true;
      return;
    }
  }
 
}




