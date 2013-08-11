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
#include  "planner.h"
#include  "stepper.h"
#include  "pins.h"

// #define DO_MOTION_TEST 1

// status leds
extern DigitalOut led1,led2,led3,led4;

// bool endstopreached=false;

static Ticker timer; // the periodic timer used to step

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
DigitalOut *laser = NULL;           // O4: (p5) LaserON (White)

// Analog in/out (cover sensor) + NC
DigitalIn cover(p19);


// globals
int step=0, command=0;
int mark_speed = 100; // 100 [mm/sec]
int counter;
int steps;
int interrupt_busy=0;

// next planner action to enqueue
tActionRequest  action;

// position offsets
static int ofsx=0, ofsy=0, ofsz=0;

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
  #ifdef READ_FILE_DEBUG
    printf("LaosMotion::reset()\n");
  #endif
  xstep = xdir = ystep = ydir = zstep = zdir = step = command = 0;
  ofsx = ofsy = ofsz = 0;
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
*** clearBuffer()
*** clears the grbl buffer
**/
void LaosMotion::clearBuffer()
{
  plan_clear_buffer();
  clear_current_block();
}

/**
*** MoveTo()
**/
void LaosMotion::moveTo(int x, int y, int z)
{
   action.target.x = ofsx + x/1000.0;
   action.target.y = ofsy + y/1000.0;
   action.target.z = ofsz + z/1000.0;
   action.ActionType = AT_MOVE;
   action.target.feed_rate =  60.0 * cfg->speed;
   plan_buffer_line(&action);
  // printf("To buffer: %d, %d\n", x, y);
}

/**
*** MoveTo() width specific speed (%)
**/
void LaosMotion::moveTo(int x, int y, int z, int speed)
{
   action.target.x = ofsx + x/1000.0;
   action.target.y = ofsy + y/1000.0;
   action.target.z = ofsz + z/1000.0;
   action.ActionType = AT_MOVE;
   action.target.feed_rate =  (speed * 60.0 * cfg->speed) / 100;
   plan_buffer_line(&action);
   //printf("To buffer: %d, %d\n", x, y);
}

/**
*** write()
*** Write command and parameters to motion controller
*** Parse all the integers found in the simplecode file per integer
**/
void LaosMotion::write(int i)
{
  static int x=0,y=0,z=0,power=10000;
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
                action.target.x = i/1000.0;
                break;
              case 2:
                action.target.y = i/1000.0;;
                step=0;
                action.target.z = 0;
                action.param = power;
                action.ActionType =  (command ? AT_LASER : AT_MOVE);
                if ( bitmap_enable && action.ActionType == AT_LASER)
                {
                  action.ActionType = AT_BITMAP;
                  bitmap_enable = 0;
                }
                action.target.feed_rate =  60.0 * (command ? mark_speed : cfg->speed );
                plan_buffer_line(&action);
                break;
            }
            break;
          case 2: // move z
            switch(step)
            {
              case 1:
                step = 0;
                z = action.target.z = i/1000.0;;
                action.param = power;
                action.ActionType =  AT_MOVE;
                action.target.feed_rate =  60.0 * cfg->speed;
                plan_buffer_line(&action);
                break;
            }
            break;
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
*** Hard set the absolute position
*** Warning: only call when the motion is not busy!
**/
void LaosMotion::setPosition(int x, int y, int z)
{
  plan_set_current_position_xyz(x/1000.0,y/1000.0,z/1000.0);
  ofsx = ofsy = ofsz = 0;
}

/**
*** get the absolute position
**/
void LaosMotion::getPosition(int *x, int *y, int *z)
{
  float xx,yy,zz;
  plan_get_current_position_xyz(&xx, &yy, &zz);
  *x = xx * 1000;
  *y = yy * 1000;
  *z = zz * 1000;
}



/**
*** set the origin to this absolute position
*** set to (0,0,0) to reset the orgin back to its original position.
*** Note: Make sure you only call this at stand-still (motion queue is empty), otherwise strange things may happen
**/
void LaosMotion::setOrigin(int x, int y, int z)
{
  ofsx = x;
  ofsy = y;
  ofsz = z;
}

/**
*** Timers for manual movement
**/

void timerMoveY()
{
  if(!interrupt_busy){
    interrupt_busy=1;
    ystep = !ystep;
//    if(endstopReachedTest()) endstopreached=true;
    if(ystep){
      steps++;
    }
    counter++;
    interrupt_busy=0;
  }
}

void timerMoveX()
{
  if(!interrupt_busy){
    interrupt_busy=1;
    xstep = !xstep;
//    if(endstopReachedTest()) endstopreached=true;
    if(xstep){
      steps++;
    }
    counter++;
    interrupt_busy=0;
  }
}

/**
*** Manual movement code
**/

void LaosMotion::manualMove()
{
  LaosDisplay *dsp;
  int c;
//  endstopreached=false;
  int countupto = 50;
  int speed;
  int x,y,z;
  int args[5];
  getPosition(&x,&y,&z);
  args[0]=x/1000.0;
  args[1]=y/1000.0;
  dsp->ShowScreen("X: +6543210 mm  " "Y: +6543210 mm  ", args, NULL);
  while(1){
//    if(cover==0 || endstopReached()) return;
    c=dsp->read();
    counter=0;
    switch(c){
      case K_CANCEL:
        return;
      case K_UP:
      case K_DOWN:
        speed=cfg->manualspeed*4;
        if(cfg->yscale>0){
          (c==K_UP) ? ydir = 1 : ydir = 0;
        }else{
          (c==K_UP) ? ydir = 0 : ydir = 1;
        }
        timer.attach_us(&timerMoveY,speed);
        while(1){
/*          if(endstopreached){
            timer.detach();
            return;
          }*/
          c = dsp->read();
          if((c!=K_UP && c!=K_DOWN) || cover==0){
            counter=0;
            while(speed<cfg->manualspeed*2){
              wait_ms(1); // this "fixes" a (timing?) bug.... doesn't work without this...
              if(counter>=countupto){
                speed=speed*1.05;
                counter=0;
                timer.attach_us(&timerMoveY,speed);
              }
            }
            timer.detach();
            ystep=0;
            if(ydir){
              y=y+(steps/(cfg->yscale/1000000.0));
            }else{
              y=y-(steps/(cfg->yscale/1000000.0));
            }
            args[0]=x/1000.0;
            args[1]=y/1000.0;
            dsp->ShowScreen("X: +6543210 mm  " "Y: +6543210 mm  ", args, NULL);
            steps=0;
            setPosition(x,y,z);
            break;
          }
          if(counter>=countupto){
            if(cfg->manualspeed<speed){
              speed=speed/1.1;
              if(speed<cfg->manualspeed) speed=cfg->manualspeed;
              timer.attach_us(&timerMoveY,speed);
            }
            args[0]=x/1000.0;
            if(ydir){
              args[1]=y/1000.0+(steps/(cfg->yscale/1000));
            }else{
              args[1]=y/1000.0-(steps/(cfg->yscale/1000));
            }
            dsp->ShowScreen("X: +6543210 mm  " "Y: +6543210 mm  ", args, NULL);
            counter=0;
          }
        }
        break;
      case K_LEFT:
      case K_RIGHT:
        speed=cfg->manualspeed*4;
        if(cfg->xscale>0){
          (c==K_RIGHT) ? xdir = 1 : xdir = 0;
        }else{
          (c==K_RIGHT) ? xdir = 0 : xdir = 1;
        }
        timer.attach_us(&timerMoveX,speed);
        while(1){
/*          if(endstopreached){
            timer.detach();
            return;
          }*/
          c = dsp->read();
          if((c!=K_LEFT && c!=K_RIGHT) || cover==0){
            counter=0;
            while(speed<cfg->manualspeed*2){
              wait_ms(1); // this "fixes" a (timing?) bug.... doesn't work without this...
              if(counter>=countupto){
                speed=speed*1.05;
                counter=0;
                timer.attach_us(&timerMoveX,speed);
              }
            }
            timer.detach();
            xstep=0;
            if(xdir){
              x=x+(steps/(cfg->xscale/1000000.0));
            }else{
              x=x-(steps/(cfg->xscale/1000000.0));
            }
            args[0]=x/1000.0;
            args[1]=y/1000.0;
            dsp->ShowScreen("X: +6543210 mm  " "Y: +6543210 mm  ", args, NULL);
            steps=0;
            setPosition(x,y,z);
            break;
          }
          if(counter>=countupto){
            if(cfg->manualspeed<speed){
              speed=speed/1.1;
              if(speed<cfg->manualspeed) speed=cfg->manualspeed;
              timer.attach_us(&timerMoveX,speed);
            }
            args[1]=y/1000.0;
            if(ydir){
              args[0]=x/1000.0+(steps/(cfg->xscale/1000));
            }else{
              args[0]=x/1000.0-(steps/(cfg->xscale/1000));
            }
            dsp->ShowScreen("X: +6543210 mm  " "Y: +6543210 mm  ", args, NULL);
            counter=0;
          }
        }
        break;
    }
  }
}



/**
*** Home the axis, stop when both home switches are pressed
**/
void LaosMotion::home(int x, int y, int z)
{
  LaosDisplay *dsp;
  int i=0;
  int canceled = 0;
  int c = 0;
  int counter = 0;
  int countupto = (cfg->xscale/1000)*5; // check cancel button state every 5mm (maybe)
  printf("Homing %d,%d, %d with speed %d\n", x, y, z, cfg->homespeed);
  xdir = cfg->xhomedir;
  ydir = cfg->yhomedir;
  zdir = cfg->zhomedir;
  led1 = 0;
  isHome = false;
  printf("Home Z...\n\r");
  if (cfg->autozhome) {
    while ((zmin ^ cfg->zpol) && (zmax ^ cfg->zpol) && !canceled) {
      if(counter==countupto){
        c = dsp->read();
        if(c==K_CANCEL || cover==0){
          isHome = false;
          return;
        }
        counter=0;
      }else{
        counter++;
      }
      zstep = 0;
        wait(cfg->homespeed/1E6);
        zstep = 1;
        wait(cfg->homespeed/1E6);
    }
  }
  printf("Home XY...\r\n");
  while ( 1 && !canceled )
  {
    if(counter==countupto){
      c = dsp->read();
      if(c==K_CANCEL || cover==0){
        isHome = false;
        return;
      }
      counter=0;
    }else{
      counter++;
    }
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
      setPosition(x,y,z);
      moveTo(x,y,z);
      isHome = true;
      printf("Home done.\r\n");
      return;
    }
  }
}



