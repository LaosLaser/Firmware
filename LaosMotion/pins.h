#include <mbed.h>

// Externs: stepper IO
extern DigitalOut enable;
extern DigitalOut xdir;
extern DigitalOut xstep;
extern DigitalOut ydir;
extern DigitalOut ystep;
extern DigitalOut zdir;
extern DigitalOut zstep;
extern DigitalOut estep; 
extern DigitalOut edir; 
extern PwmOut pwm;

// leds
extern DigitalOut led1,led2,led3,led4;

// Inputs
extern DigitalIn xhome;
extern DigitalIn yhome;
extern DigitalIn zmin;
extern DigitalIn zmax;
 

// laser
extern PwmOut pwm;                // O1: PWM (Yellow)
extern DigitalOut laser_enable;   // O2: enable laser
extern DigitalOut o3;              // 03: NC
extern DigitalOut laser;           // O4: LaserON (White)


// Analog in/out (cover sensor) + NC
extern DigitalIn cover;