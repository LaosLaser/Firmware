/*
 * Pin definitions
 *
 */
#include "mbed.h"

// status leds
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// Status and communication
DigitalOut eth_link(p29); // green
DigitalOut eth_speed(p30); // yellow

// Stepper IO
DigitalOut enable(p7);
DigitalOut xdir(p23);
DigitalOut xstep(p24);
DigitalOut ydir(p25);
DigitalOut ystep(p26);
DigitalOut zdir(p27);
DigitalOut zstep(p28);

// Inputs;
DigitalIn xhome(p8);
DigitalIn yhome(p17);
DigitalIn zmin(p15);
DigitalIn zmax(p16);

// laser IO
PwmOut pwm(p22);                // O1: PWM (Yellow)
DigitalOut laser_enable(p21);   // O2: enable laser
DigitalOut o3(p6);              // 03: NC
DigitalOut *laser = NULL;           // O4: (p5) LaserON (White)

// Analog in/out (cover sensor) + NC
DigitalIn cover(p19);
