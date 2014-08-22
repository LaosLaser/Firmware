/**
 * LaosMotion.cpp
 * Motion Controll functions for Laos system
 *
 * Copyright (c) 2011 Peter Brier
 *
 *   This file is part of the LaOS project (see: http://laoslaser.org)
 *spe
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
#include  "stepper.h"
#include  "pins.h"

// #define DO_MOTION_TEST 1

// globals
unsigned int step=0;
int command=0;
int mark_speed = 100; // 100 [mm/sec]
int power = 10000 ;

// Command interpreter
int param=0, val=0;

// Bitmap buffer
#define BITMAP_PIXELS  (8192)
#define BITMAP_SIZE (BITMAP_PIXELS/32)
unsigned long bitmap[BITMAP_SIZE];
unsigned long bitmap_width=0; // nr of pixels
unsigned long bitmap_size=0; // nr of bytes
unsigned char bitmap_bpp=1, bitmap_enable=0;

/**
*** LaosMotion() Constructor
*** Make new motion object
**/
LaosMotion::LaosMotion()
{
  extern GlobalConfig *cfg;
#if DO_MOTION_TEST
  tActionRequest act[2];
  int i=0;
  Timer t;
#endif
  pwm.period(1.0 / cfg->pwmfreq);
  pwm = cfg->pwmmin/100.0;
  if ( laser == NULL ) laser = new DigitalOut(LASER_PIN);
  *laser = LASEROFF;

  mark_speed = cfg->speed;
  //start.mode(PullUp);
  xhome.mode(PullUp);
  yhome.mode(PullUp);
  isHome = false;
    setOriginAbsolute(0,0,0);
  plan_init();
  st_init();
  reset();
  mark_speed = cfg->speed;

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
  extern GlobalConfig *cfg;
  #ifdef READ_FILE_DEBUG
    printf("LaosMotion::reset()\n");
  #endif
  xstep = xdir = ystep = ydir = zstep = zdir = step = command = 0;
  m_PlannedXAbsolute = 0;
  m_PlannedYAbsolute = 0;
  m_PlannedZAbsolute = 0;
  *laser = LASEROFF;
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
*** queue()
*** return nr of items in the queue (0 is empty)
**/
int LaosMotion::queue()
{
  return plan_queue_items();
}




/**
*** MoveTo()
**/
void LaosMotion::moveToRelativeToOrigin(int x, int y, int z, int speed, int power)
{
  moveToAbsolute(x-ofsx, y-ofsy, z-ofsz, speed, power);
}

void LaosMotion::moveToAbsolute(int x, int y, int z, int speed, int power)
{
  extern GlobalConfig *cfg;
  int feedrate = (speed * 60.0 * cfg->speed) / 100;
  moveToAbsoluteWithAbsoluteFeedrate(x, y, z, feedrate, power, AT_MOVE);
}

void LaosMotion::moveToRelativeToOriginWithAbsoluteFeedrate(int x, int y, int z, int feedrate, int power, eActionType actiontype)
{
  moveToAbsoluteWithAbsoluteFeedrate(x-ofsx, y-ofsy, z-ofsz, feedrate, power, actiontype);
}

void LaosMotion::moveToAbsoluteWithAbsoluteFeedrate(int x, int y, int z, int feedrate, int power, eActionType actiontype)
{
  extern GlobalConfig *cfg;
  if(cfg->enforcelimits)
  {
    if(x < cfg->xmin) x=cfg->xmin;
    if(y < cfg->ymin) y=cfg->ymin;
    if(z < cfg->zmin) z=cfg->zmin;
    if(x > cfg->xmax) x=cfg->xmax;
    if(y > cfg->ymax) y=cfg->ymax;
    if(z > cfg->zmax) z=cfg->zmax;
  }
  tActionRequest action;
  action.target.x = x/1000.0;
  action.target.y = y/1000.0;
  action.target.z = z/1000.0;
  action.ActionType = actiontype;
  action.target.feed_rate =  feedrate;
  action.param = power;
  plan_buffer_line(&action);
  m_PlannedXAbsolute = x;
  m_PlannedYAbsolute = y;
  m_PlannedZAbsolute = z;
   //printf("To buffer: %d, %d, %d, %d\n", x, y,z,speed);
}

/**
*** write()
*** Write command and parameters to motion controller
*** Parse all the integers found in the simplecode file per integer
*** All moves and coordinates are treated as coordinates relative to the set origin
**/
void LaosMotion::write(int i)
{
  extern GlobalConfig *cfg;
  static int commandx=0,commandy=0,commandz=0;
  //if (  plan_queue_empty() )
  //printf("Empty\n");
  
  
  #ifdef READ_FILE_DEBUG_VERBOSE
  	printf(">%i (command: %i, step: %i)\n",i,command,step);
  #endif	
  
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
                commandx = i;
                break;
              case 2:
                commandy = i;
                step=0;
                int feedrate;
                eActionType actiontype;
                if(command == 1)
                {
                  if(bitmap_enable)
                  {
                    actiontype = AT_BITMAP;
                    bitmap_enable = 0;
                    feedrate = 60 * cfg->xspeed;
                  }
                  else
                  {
                    actiontype = AT_LASER;
                    feedrate = 60 * mark_speed;
                  }
                }
                else
                {
                  actiontype = AT_MOVE;
                  feedrate = 60 * cfg->speed;
                }
            		if ( actiontype == AT_BITMAP )
            		{
                  while ( queue() );// printf("-"); // wait for queue to empty
                  plan_set_accel(cfg->xaccel);
                }
                int dummy1, dummy2, plannedz;
                getPlannedPositionRelativeToOrigin(&dummy1, &dummy2, &plannedz); // get the planned z coordinatre
                moveToRelativeToOriginWithAbsoluteFeedrate(commandx, commandy, plannedz, feedrate, power, actiontype);
                if ( actiontype == AT_BITMAP )
                {
                  while ( queue() );// printf("-"); // wait for queue to empty
                  plan_set_accel(cfg->accel);
                }
                break;
            }
            break;
          case 2: // move z
            switch(step)
            {
              case 1:
                commandz = i;
                step = 0;
                int plannedx, plannedy, dummy1;
                getPlannedPositionRelativeToOrigin(&plannedx, &plannedy, &dummy1);
                int feedrate = 60.0 * cfg->speed;
                moveToRelativeToOriginWithAbsoluteFeedrate(plannedx, plannedy, commandz, feedrate, power, AT_MOVE);
                break;
            }
            break;
         case 4: // set x,y,z (absolute)
            switch ( step )
            {
              case 1:
                commandx = i;
                break;
              case 2:
                commandy = i;
                break;
              case 3:
                commandz = i;
                setPositionRelativeToOrigin(commandx, commandy, commandz);
                step=0;
                break;
            }
            break;
         case 5: // nop
           step = 0;
           break;
         case 7: // set index,value
            switch ( step )
            {
              case 1:
                param = i;
                break;
              case 2:
                val = i;
                step = 0;
                switch( param )
                {
                  case 100:
                    if ( val < 1 ) val = 1;
                    if ( val > 9999 ) val = 10000;
                    mark_speed = val * cfg->speed / 10000;
                    #ifdef READ_FILE_DEBUG
                      printf("> speed: %i\n",mark_speed);
                    #endif	
                    break;
                  case 101:
                    power = val;
                    #ifdef READ_FILE_DEBUG
                      printf("> power: %i\n",power);
                    #endif	
                    break;
                }
                break;
            }
            break;
         case 9: // Store bitmap mark data format: 9 <bpp> <width> <data-0> <data-1> ... <data-n>
            if ( step == 1 )
            {
              bitmap_bpp = i;
            }
            else if ( step == 2 )
            {
           //   if ( queue() ) printf("Queue not empty... wait...\n\r");
              while ( queue() );// printf("+"); // wait for queue to empty
              bitmap_width = i;
              bitmap_enable = 1;
              bitmap_size = (bitmap_bpp * bitmap_width) / 32;
              if  ( (bitmap_bpp * bitmap_width) % 32 )  // padd to next 32-bit
                bitmap_size++;
              // printf("\n\rBitmap: read %d dwords\n\r", bitmap_size);

            }
            else if ( step > 2 )// copy data
            {
              bitmap[ (step-3) % BITMAP_SIZE ] = i;
			  // printf("[%ld] = %ld\n", (step-3) % BITMAP_SIZE, i);
			  if ( step-2 == bitmap_size ) // last dword received
              {
                step = 0;
                // printf("Bitmap: received %d dwords\n\r", bitmap_size);
              }
            }
            break;
         default: // I do not understand: stop motion
            step = 0;
            break;
    }
    if ( step )
	  step++;
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
*** Hard set the position
*** Warning: only call when the motion is not busy!
*** current offset is taken into account
**/
void LaosMotion::setPositionRelativeToOrigin(int x, int y, int z)
{
  setPositionAbsolute(x-ofsx, y-ofsy, z-ofsz);
}

/**
*** Hard set the position
*** Warning: only call when the motion is not busy!
**/
void LaosMotion::setPositionAbsolute(int x, int y, int z)
{
  m_PlannedXAbsolute = x;
  m_PlannedYAbsolute = y;
  m_PlannedZAbsolute = z;
  plan_set_current_position_xyz(x/1000.0,y/1000.0,z/1000.0);
}

/**
*** get the position
*** The result is offset by the current offset location
**/
void LaosMotion::getCurrentPositionRelativeToOrigin(int *x, int *y, int *z)
{
  getCurrentPositionAbsolute(x, y, z);
  *x += ofsx;
  *y += ofsy;
  *z += ofsz;
}

/**
*** get the actual position
**/
void LaosMotion::getCurrentPositionAbsolute(int *x, int *y, int *z)
{
  float xx,yy,zz;
  plan_get_current_position_xyz(&xx, &yy, &zz);
  *x = xx * 1000;
  *y = yy * 1000;
  *z = zz * 1000;
}

void LaosMotion::getPlannedPositionRelativeToOrigin(int *x, int *y, int *z)
{
  getPlannedPositionAbsolute(x, y, z);
  *x += ofsx;
  *y += ofsy;
  *z += ofsz;  
}

void LaosMotion::getPlannedPositionAbsolute(int *x, int *y, int *z)
{
  *x = m_PlannedXAbsolute;
  *y = m_PlannedYAbsolute;
  *z = m_PlannedZAbsolute;
}

void LaosMotion::getLimitsRelative(int *minx, int *miny, int *minz, int *maxx, int *maxy, int *maxz)
{
  extern GlobalConfig *cfg;
  *minx=cfg->xmin + ofsx;
  *miny=cfg->ymin + ofsy;
  *minz=cfg->zmin + ofsz;
  *maxx=cfg->xmax + ofsx;
  *maxy=cfg->ymax + ofsy;
  *maxz=cfg->zmax + ofsz;
}

/**
*** set the origin to this absolute position
*** set to (0,0,0) to reset the orgin back to its original position.
*** Note: Make sure you only call this at stand-still (motion queue is empty), otherwise strange things may happen
**/
void LaosMotion::setOriginAbsolute(int x, int y, int z)
{
  ofsx = -x;
  ofsy = -y;
  ofsz = -z;
}

/* 
  Make current position the origin. 'origin' is defined here as the top left corner (0,0) in Visicut
  Since LAOS has the y coordinate 0 at the bottom of the bed, we need to offset by the bed height.
*/
void LaosMotion::MakeCurrentPositionOrigin()
{
  extern GlobalConfig *cfg;
  int x,y,z;
  getCurrentPositionAbsolute(&x, &y, &z);
  y -= cfg->bedheight;
  setOriginAbsolute(x, y, z);
}


/**
*** Home the axis, stop when both home switches are pressed
*** Resets the origin
**/
void LaosMotion::home(int x, int y, int z)
{
  extern GlobalConfig *cfg;
  int i=0;
  printf("Homing %d,%d, with speed %d\n", x, y, cfg->homespeed);
  xdir = cfg->xhomedir;
  ydir = cfg->yhomedir;
  zdir = cfg->zhomedir;
  led1 = 0;
  isHome = false;
  printf("Home Z...\n\r");
  if (cfg->autozhome) {
    printf("Homing %d with speed %d\n", z, cfg->zhomespeed);
    while ((zmin ^ cfg->zpol) && (zmax ^ cfg->zpol)) {
        zstep = 0;
        wait(cfg->zhomespeed/1E6);
        zstep = 1;
        wait(cfg->zhomespeed/1E6);
    }
  }
  printf("Home XY...\n\r");
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
      setOriginAbsolute(0, 0, 0); // reset origin
      setPositionAbsolute(x,y,z);
      moveToAbsolute(x,y,z);
      isHome = true;
      printf("Home done.\n\r");
      return;
    }
  }

}



