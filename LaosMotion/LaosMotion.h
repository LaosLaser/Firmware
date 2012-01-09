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
#ifndef _LAOSMOTION_H_
#define _LAOSMOTION_H_
#include "global.h"


// the state of the laser OUTPUT
#define LASEROFF 1
#define LASERON 0

    /** Motion Controll system
      *
      * Example:
      * @code 
      * @endcode
      */
class LaosMotion {
public:
    /** Make new LaosMotion object. 
      * Installs ticker
      */
  LaosMotion();
  ~LaosMotion();
  void write(int i); // write command word to motion controller
  int ready(); // returns true if we are ready to accept a new instruction
  void reset(); // reset the instruction decoder and motion controller
  void home(int xhome, int yhome); // home the system, move to the sensors and set the specified position
  bool isStart(); // start button is enabled
  bool isHome; // system is homed
  void setPosition(int x, int y, int z); // in micron
  void getPosition(int *x, int *y, int *z); // in micron
  void moveTo(int x, int y, int z); // in microns
private:
  
};

 

#endif
