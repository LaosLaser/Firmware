#include "mbed.h"

#ifndef _PINS_H_
#define _PINS_H_

// other definitions
extern "C" void mbed_reset();

// status leds
extern DigitalOut led1,led2,led3,led4;

// Status and communication
extern DigitalOut eth_link;  // green
extern DigitalOut eth_speed; // yellow

// Stepper IO
extern DigitalOut enable;
extern DigitalOut xdir;
extern DigitalOut xstep;
extern DigitalOut ydir;
extern DigitalOut ystep;
extern DigitalOut zdir;
extern DigitalOut zstep;

// Inputs
extern DigitalIn xhome;
extern DigitalIn yhome;
extern DigitalIn zmin;
extern DigitalIn zmax;

// Laser IO
extern PwmOut pwm;              // O1: PWM (Yellow)
extern DigitalOut laser_enable; // O2: enable laser
extern DigitalOut exhaust;      // 03: NC
extern DigitalOut *laser;       // O4: LaserON (White), do not statically 
                                //     allocate: because this will cause the 
                                //     laser to switch on at boot
#define LASER_PIN p5            // note: we define the laser pin here and do i
                                // not allocate the laser DigitalOut()

// Analog in/out (cover sensor) + NC
extern DigitalIn cover;

// the state of the laser OUTPUT
#define LASEROFF 1
#define LASERON 0

#endif
