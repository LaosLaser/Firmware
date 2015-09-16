/*
 * main.cpp
 *
 * Copyright (c) 2011-2015 Peter Brier & Jaap Vermaas
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
 * This program tests the inputs and outputs of the laoslaser board
 * LED1 = xhome
 * LED2 = yhome
 * LED3 = zmin
 * LED4 = zmax
 *
 */
#include "mbed.h"
#include "SDFileSystem.h"
#include "EthernetInterface.h"

// USB Serial
Serial pc(USBTX, USBRX); // tx, rx

// SD card
SDFileSystem sd(p11, p12, p13, p14, "sd");

// Ethernet
EthernetInterface eth;

// Analog in/out (cover sensor) + NC
DigitalIn cover(p19);

// I2C
I2C i2c(p9, p10);        // sda, scl
#define _I2C_ADDRESS 0x04
#define _I2C_HOME 0xFE
#define _I2C_CLS 0xFF
#define _I2C_BAUD 9600

// status leds
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// Inputs;
DigitalIn xhome(p8);
DigitalIn yhome(p17);
DigitalIn zmin(p15);
DigitalIn zmax(p16);

// motors
DigitalOut enable(p7);
DigitalOut xdir(p23);
DigitalOut xstep(p24);
DigitalOut ydir(p25);
DigitalOut ystep(p26);
DigitalOut zdir(p27);
DigitalOut zstep(p28);

// laser & exhaust outputs
DigitalOut laser_pwm(p22);
DigitalOut laser_enable(p21);
DigitalOut exhaust_enable(p6);
DigitalOut laser_on(p5);

// CAN bus
CAN can(p30, p29);

void home()
{
  printf("Homing with x,y,z dir=0\n");
  int i=0;
  xdir = 0;
  ydir = 0;
  zdir = 0;
  led1 = 0;
  while ( 1 )
  {
    xstep = ystep = 0;
    wait(0.0001);
    xstep = xhome;
    ystep = yhome;
    zstep = zmin;
    wait(0.0001);
    led1 = !xhome;    
    led2 = !yhome;
    led3 = !zmin;
    led4 = ((i++) & 0x10000);
    if ( (!xhome) && (!yhome) ) return;
  }
}

#define TEST(c,x) case c: io = &x; x = !x; printf("Test %c: " #x " is now %s \n\r", c, (x ? "ON" : "OFF")); break 

/**
*** main()
**/
int main() 
{
  // enable serial communication
  pc.baud(115200);

  // enable I2C display communcation
  i2c.frequency(_I2C_BAUD);

  // test networking 
  eth.init();
  eth.connect();
  printf("IP Address is: %s\n\r", eth.getIPAddress());

  FILE *test1=NULL, *test2=NULL;
  led1 = led2 = led3 = led4 = 1;
  Timer t;
  DigitalOut *io = NULL;
  CANMessage msg;

  xhome.mode(PullUp);
  yhome.mode(PullUp);
  zmin.mode(PullUp);
  zmax.mode(PullUp); 
  
  char buf[512];
  char *help = "\n\r\n\r\n\rLaOS Test program\n\r"
   __DATE__ " " __TIME__ "\n\r"
   "++++++++++++++++++\n\r"
   "Use these keys to test the functionality of the board:\n\r"
   "xX: X Step/Dir\t"
   "yY: Y Step/Dir\t"
   "zZ: Z Step/Dir\t"
   "tT: Ext Step/Dir (laser_pwm/laser_enable)\n\r"
   "e: Toggle Stepper enable\n\r"
   "c: Can bus test\n\r"
   "sS: SD-Card test (s=test file, S=speed test)\n\r"
   "i: I2C\n\r"
   "1/2/3/4: Toggle outputs\n\r"
   "f: apply 1Khz frequency to last IO line (1 second)\n\r"
   "h: X/Y homing (note: direction and home sensor polarity may be wrong\n\r";
  char iline[] = " LAOS-IOTEST V1.1";
  iline[0] = 0xFF;
  //i2c.write(_I2C_ADDRESS, iline, 17);
  printf(help);

  int i=0;
  int test=0;
  char key=0;
  while( 1 )
  {
    i++;
    led1 = xhome;
    led2 = yhome;
    led3 = zmin;
    led4 = zmax;
    if ( pc.readable() )
      test = pc.getc();
    else 
      test = 0;
    switch( test )
    {
      TEST('e',enable);
      TEST('X',xdir);
      TEST('x',xstep);
      TEST('Y',ydir);
      TEST('y',ystep);
      TEST('Z',zdir);
      TEST('z',zstep);
      TEST('t',laser_enable);
      TEST('T',laser_pwm);
      TEST('1',laser_pwm);
      TEST('2',laser_enable);
      TEST('3',exhaust_enable);
      TEST('4',laser_on);
      case 'h': 
        home(); 
        break;
      case 'f':
      case 'F':
        if (io != NULL ) 
        {
          printf("pulsing output for 1000 pulses sec at 500Hz or 10kHz...\n");
          for(i=0;i<1000;i++)
          {
            wait((test == 'f' ? 0.001 : 0.00005));
            *io = 1;
            wait((test == 'f' ? 0.001 : 0.00005));
            *io = 0;
          }
        
        }
        break;
      case 's':
        memset(buf,0,sizeof(buf));
        printf("Testing IO board SD card: Write...\n\r");
        test1 = fopen("/sd/text.txt", "wb");
        fwrite("bla bla\n",8, 1, test1); 
        fclose(test1);
        printf("Testing IO board SD card: Read...\n\r");
        test1 = fopen("/sd/text.txt", "rb");
        fread(&buf,8, 1, test1); 
        printf("Result: '%s'\n\r", buf);
        fclose(test1);
        break;
      case 'S':
        printf("SD card speed test (writing and reading 1MByte file\n");
        memset(buf,23,sizeof(buf));
        test2 = fopen("/sd/test.bin", "wb");
        t.reset();
        t.start();
        for(i=0;i<sizeof(buf);i++)
          buf[i] = (char)t.read_ms();
         t.stop(); t.reset(); t.start();
        for(i=0;i<200;i++)
          fwrite(buf,sizeof(buf), 1, test2); 
        t.stop();
        fclose(test2);
        printf("Write 1MByte: %d msec\n\r", (int)t.read_ms());
        test2 = fopen("/sd/test.bin", "rb");
        t.reset();
        t.start();
        while( fread(&buf, sizeof(buf), 1, test2) > 0 );
        t.stop();
        printf("Read 1MByte: %d msec\n\r", (int)t.read_ms());
      case 'i': 
        printf("I2C test:\n\r");
        char s[48];
        sprintf(s," 0123456789ABCDEF1234567890ABCDEF");
        s[0] = 0xFF;
        i2c.write(_I2C_ADDRESS, s, strlen(s));
        wait(3);
        i2c.read(_I2C_ADDRESS, s, 48);
        sprintf(iline," Key read: %c", s[0]);
        i2c.write(_I2C_ADDRESS, iline, strlen(iline));
        break;
      case 'c':
        printf("CAN bus test, sending and receiving characters, press any key to quit\n\r");
        can.frequency(500000);
        can.reset();
        t.start();
        can.reset();
        while( !pc.readable() )
        {
          if ( t > 1.0 ) 
          {
            t.reset();
            can.write(CANMessage(10, &key, 1));
            key++;
          }
          wait(0.1);
          if(can.read(msg)) 
            printf("Message received: id=%d, len=%d, data[0]=%d\n\n\r", (int)msg.id, (int)msg.len, (int)msg.data[0]);
   
         printf("value=%d, rderror=%d, tderror=%d\n\r", (int)key, can.rderror(), can.tderror());
       
        }         
        break;
      case 0:
      case '\n':
      case '\r': break;
      default: printf(help); break;
    }
    i2c.read(_I2C_ADDRESS,&key, 1);
    if (key != 0) {
        char s[48];
        printf("I2C key read: %c\n\r", key);
        sprintf(s," Key read: %c", key);
        s[0] = 0xFF;
        i2c.write(_I2C_ADDRESS, s, strlen(s));
        wait(0.2);
        if (key == '7') {
            wait(1);
            i2c.write(_I2C_ADDRESS, iline, 17);
        }
        i2c.read(_I2C_ADDRESS, s, 48);
        key=0;
    }
    if ( test ) 
    {
      printf("xhome=%d, yhome=%d, zmin=%d, zmax=%d, cover=%s\n\r", (int)xhome, (int)yhome, (int)zmin, (int)zmax, (cover ? "OPEN" : "CLOSED") );
    }
  }
}
