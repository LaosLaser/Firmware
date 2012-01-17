/**
 * LaosMenu.h
 * Simple menu system
 *
 * Copyright (c) 2011 Peter Brier & Jaap Vermaas
 *
 *   This file is part of the LaOS project (see: http://wiki.laoslaser.org/)
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
 @code 
 --code--
 @endcode
 */
#ifndef _LAOSMENU_H_
#define _LAOSMENU_H_

#include "global.h"
#include "LaosDisplay.h"
#include "LastScreen.h"
#include "laosfilesystem.h"


    /** Menu system
      * Create server based on config file. 
      *
      * Example:
      * @code 
      * LaosMenu menu();
      * menu.Handle();
      * @endcode
      */
class LaosMenu {
public:
    /** Make new LaosMenu object. 
      */
  LaosMenu(LaosDisplay *display);
  ~LaosMenu();

/** Handle the menu system
  * Reads inputs, displays screen
  */ 
  void Handle();
  void SetScreen(int screen);
  void SetScreen(char *s);
  void SetFileName(char * name);
  void SetPosition(int xi, int yi, int zi);
  
private:
  // LaosDisplay *display;
  int args[5];
  unsigned char waitup, timeout;
  char *sarg;
  int speed;
  char jobname[MAXFILESIZE];
  
  // menu states
  int screen, prevscreen;
  LastScreen *lastscreen;
  unsigned char menu, ipfield, iofield;
  unsigned char powerfield, power[4];
  LaosDisplay *dsp;
  int x,y,z;
  int xoff, yoff, zoff;
  FILE *runfile;
  
};

 

#endif
