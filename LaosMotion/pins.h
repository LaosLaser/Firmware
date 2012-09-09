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

#define LASER_PIN p5 // note: we define the laser pin here and do not allocate the laser DigitalOut()
extern DigitalOut *laser;           // O4: LaserON (White), do not statically allocte: because this will cause the laser to switch on at reboot


// Analog in/out (cover sensor) + NC
extern DigitalIn cover;