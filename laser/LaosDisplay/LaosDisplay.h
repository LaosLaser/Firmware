/**
 * LaosDisplay.cpp
 * User interface class, for 2x16 display and keypad
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
 * Assume (USB) serial connected display (or terminal) in future, this could be CAN or I2C connected
 *
 */
#ifndef _LAOS_DISPLAY_H_
#define _LAOS_DISPLAY_H_

extern "C" void mbed_reset();
    /** LaosDisplay
      * Connect to LCD terminal or PC
      * Example:
      * @code 
      * @endcode
      */
class LaosDisplay {
public:
    /** Make new LaosDisplay object. 
      */
    LaosDisplay();
    void Simulate();
    void EnableI2C(int baud);
/** Display string at position (x,y)
  * @param x  x position in characters (zero based)
  * @param y  y position in characters (zero based)
  * @param s The string to display
  */ 
  void write(char *s);
  void ShowScreen(const char *l, int *arg, char *s);
  void testI2C();
   

/** Clear screen
  */ 
   void cls();

/** Read key value. (blocking)
  * @return (ASCII) character value, zero if no character is available
  */ 
  int read();

/** Read key value. (non blocking)
  * @return (ASCII) character value, zero if no character is available
  */ 
  int read_nb();
    
private:
  bool sim;
  int i2cBaud;
};

/* PC keyboard and I2C display
 * have the same keys
 */
#define K_UP '8'
#define K_DOWN '2'
#define K_LEFT '4'
#define K_RIGHT '6'
#define K_FUP '9'
#define K_FDOWN '3'
#define K_OK '5'
#define K_CANCEL '7'
#define K_ORIGIN '1'

#endif
