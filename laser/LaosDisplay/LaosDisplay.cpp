/**
 * LaosDisplay.cpp
 * User interface class, for 
 *    => 2x16 display and keypad
 *    => serial connection
 *
 * Copyright (c) 2011 Peter Brier and Jaap Vermaas
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
 */
#include "LaosDisplay.h"
#include "MODI2C.h"
#include "mbed.h"

// Serial
#if !MRI_ENABLE
Serial serial(USBTX, USBRX);
#define _SERIAL_BAUD 115200
#endif

// I2C
MODI2C i2c(p9, p10);        // sda, scl
#define _I2C_ADDRESS 0x04
#define _I2C_HOME 0xFE
#define _I2C_CLS 0xFF
#define _I2C_BAUD 9600

// Make new config file object
LaosDisplay::LaosDisplay()
{
  int i2cBaud = _I2C_BAUD;
  i2c.frequency(i2cBaud );
#if !MRI_ENABLE
  int serialBaud = _SERIAL_BAUD;
  serial.baud(serialBaud);
#endif

  char key;
  // test I2C, if we cannot read, display is not attached, enable simulation
  // wait 1 second to make sure that I2C has time to power on!
  wait(3);
  sim = i2c.read(_I2C_ADDRESS ,&key, 1) != 0; 
  if (sim) {
    printf("LaosDisplay()\n");
    printf("Display() Simulation=ON, I2C Baudrate=%d\n", i2cBaud  );
  }
}

// Test I2C again when config file is read
void LaosDisplay::testI2C() {
    char key;
    sim = i2c.read(_I2C_ADDRESS ,&key, 1) != 0;
    if (sim) mbed_reset();
}

// Write string
void LaosDisplay::write(char *s) 
{  
  if ( sim ) 
  {
    while (*s)
#if !MRI_ENABLE
      serial.putc(*s++);
#else
      putc(*s++, stdout);
#endif
    return;
  } else {
    while (*s) {
      i2c.write(_I2C_ADDRESS, s++, 1);
      while(i2c.getQueue()) wait_us(1);
    }
  }
}

// Clear screen
void LaosDisplay::cls() 
{
   char s[2];
   s[0] = _I2C_HOME;
   s[1] = 0;
   write(s);
}

// Read Key
int LaosDisplay::read() 
{
  char key = 0;
  if (sim)
  {
#if !MRI_ENABLE
    if ( serial.readable() )
      key =  serial.getc();
    else
#endif
      key = 0;
  }
  else
  {
    i2c.read(_I2C_ADDRESS ,&key, 1);
  }
  if ((key < '1') || (key > '9'))
    key = 0;
  return key;   
}

// Read Key non-blocking (needs to be called often to retrieve key)
int LaosDisplay::read_nb() 
{
  char key = 0;
  static char bg_buf = 0;  // Static buffer for background retrieve
  static int status = 1;   // for first start
  if (sim)
  {
#if !MRI_ENABLE
    if ( serial.readable() )
      key =  serial.getc();
    else
#endif
      key = 0;
  }
  else
  {
    if(status) {
      // last I2C read has finished now
      key = bg_buf;
      i2c.read_nb(_I2C_ADDRESS ,&bg_buf, 1, false, &status);
    }
  }
  if ((key < '1') || (key > '9'))
    key = 0;
  return key;   
}

/**
*** Screens are defined with:
*** name, line[2], int* i[4], char *s;
***
*** in the lines, the following replacements are made:
*** $$$$$$ : "$" chars are replaced with the string argument for this screen (left alligned), if argument is NULL, spaces are inserted
*** +9876543210: "+" and numbers are replaced with decimal digits from the integer arguments (the "+" sign is replaced with '-' if the argument is negative
*** 
***
**/
inline char digit(int i, int n)
{
  int d=1;
  char c;
  if ( i<0 ) i *= -1;
  while(n--) d *= 10;
  c = '0' + ((i/d) % 10);
  return c;
}

void LaosDisplay::ShowScreen(const char *l, int *arg, char *s)
{
  char c, next=0,surpress=1;
  char str[128],*p; 
  p = str;
  *p++ = _I2C_HOME;
  while ( *l )
  {
    c=*l;
    switch ( c )
    {
    case '$': 
      if (s != NULL && *s)
        c = *s++;
      else 
        c = ' ';
      break;
      
    case '+':
      if (arg != NULL && *arg < 0)
        c = '-';
      else 
        c = '+';
      break;
        
    case '0': next=1; surpress=0; 
    case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9':
      char d = ' ';
      if (arg != NULL )
      {
        d = digit(*arg, *l-'0');
        if ( d == '0' && surpress  ) // supress leading zeros 
          d =  ' ';
        else
          surpress = 0;
        if ( next && arg != NULL ) // take next numeric argument
        {
          arg++;
          next=0;
          surpress=1;
        }
      }
      c = d;
      break;
    }
    *p++ = c;
    l++;
  }
  *p=0;
  write(str);
}

// EOF
