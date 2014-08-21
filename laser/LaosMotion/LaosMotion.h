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
#include "pins.h"
#include  "planner.h"

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
  void home(int xhome, int yhome, int zhome); // home the system, move to the sensors and set the specified position
  bool isStart(); // start button is enabled
  bool isHome; // system is homed
  void setPositionRelativeToOrigin(int x, int y, int z);
  void setPositionAbsolute(int x, int y, int z);
  void getCurrentPositionRelativeToOrigin(int *x, int *y, int *z);
  void getCurrentPositionAbsolute(int *x, int *y, int *z);
  void getPlannedPositionRelativeToOrigin(int *x, int *y, int *z);
  void getPlannedPositionAbsolute(int *x, int *y, int *z);
  void setOriginAbsolute(int x, int y, int z); // set the origin to this absolute position [micron]
  void MakeCurrentPositionOrigin(); // set the current position to be the origin
  void moveToRelativeToOrigin(int x, int y, int z, int speed=100, int power=100);
  void moveToAbsolute(int x, int y, int z, int speed=100, int power=100);
  void moveToRelativeToOriginWithAbsoluteFeedrate(int x, int y, int z, int feedrate, int power, eActionType actiontype);
  void moveToAbsoluteWithAbsoluteFeedrate(int x, int y, int z, int feedrate, int power, eActionType actiontype);
  int queue(); // queued items
private:
  int ofsx, ofsy, ofsz;
  int m_PlannedXAbsolute, m_PlannedYAbsolute, m_PlannedZAbsolute; // in absolute coordinates
  
};

#endif
