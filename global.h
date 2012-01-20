/*
 * global.h
 * Laos Controller, global header file, included in all other files
 *
 * Copyright (c) 2011 Peter Brier
 *
 *   This file is part of the LaOS project (see: http://wiki.protospace.nl/index.php/LaOS)
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
 */
#ifndef _GLOBAL_H_
#define _GLOBAL_H_
 
#include "mbed.h"
#include "LaosDisplay.h"
#include "LaosMotion.h"
 
// Global configuration struct
class GlobalConfig
{
public:
  int ip[4], gw[4], nm[4], dns[4], port, dhcp;  // network settings
  int enable; // enable state (1 or 0)
  int autohome; // automatically home the axis at startup
  int nodisplay; // there is no display 
  int cleandir; // remove files from SD at startup
  int i2cbaud; // i2cBaudrate
  int xmax, ymax, zmax, emax; // max values
  int xmin, ymin, zmin, emin; // min values
  int xpol, ypol, zpol, epol; // polarity for the home switches
  int xinv, yinv, zinv, einv; // invert signal polarity for step/dir
  int xhome, yhome, zhome, ehome; // home position
  int xrest, yrest, zrest, erest; // rest positon (moveto after job)
  int xhomedir, yhomedir, zhomedir, ehomedir;
  int homespeed; // speed used for homing [usec]
  int speed, xspeed, yspeed, zspeed, espeed; // Maximum linear speed and max speed per axis [mm/sec]
  int accel; // defaul accelletaion [mm/sec2]
  int tolerance; // corner tolerance [micrometer]
  int xscale; // steps per meter
  int yscale; // steps per meter
  int zscale; // steps per meter
  int escale; // steps per meter
  int lenable, lon, pwmmin, pwmmax, pwmfreq; // laser enable, laser on and pwm min/max [%] and frequency [Hz];
  GlobalConfig(char *filename);
};
extern GlobalConfig *cfg; 

#define VERSION_STRING "\033LAOS v0.3" __DATE__ " " __TIME__

#endif