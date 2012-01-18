/*
 * main.cpp
 * Laos Controller, main function
 *
 * Copyright (c) 2011 Peter Brier & Jaap Vermaas
 *
 *   This file is part of the LaOS project (see: http://wiki.laoslaser.org)
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
 * This program consists of a few parts:
 *
 * ConfigFile   Read configuration files
 * EthConfig    Initialize an ethernet stack, based on a configuration file (includes link status monitoring)
 * LaosDisplay  User interface functions (read keys, display text and menus on LCD)
 * LaosMenu     User interface stuctures (menu navigation)
 * LaosServer   TCP/IP server, accept connections read/write data
 * LaosMotion   Motion control functions (X,Y,Z motion, homing)
 * LaosIO       Input/Output functions
 * LaosFile     File based jobs (read/write/delete)
 *
 * Program functions:
 * 1) Read config file
 * 2) Enable TCP/IP stack (Fixed ip or DHCP)
 * 3) Instantiate tcp/ip port and accept connections
 * 4) Show menus, peform user actions
 * 5) Controll laser
 * 6) Controll motion
 * 7) Set and read IO, check status (e.g. interlocks)
 *
 */
#include "global.h"
#include "ConfigFile.h"
#include "EthConfig.h"
#include "TFTPServer.h"
#include "LaosMenu.h"
#include "LaosMotion.h"
#include "SDFileSystem.h"
#include "laosfilesystem.h"

// MBED blue status leds
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// Status and communication
DigitalOut eth_link(p29); // green
DigitalOut eth_speed(p30); // yellow
EthernetNetIf *eth; // Ethernet, tcp/ip

// Filesystems
LocalFileSystem local("local");   //File System
LaosFileSystem sd(p11, p12, p13, p14, "sd");

// Laos objects
LaosDisplay *dsp;
LaosMenu *mnu;
TFTPServer *srv;
LaosMotion *mot;
Timer systime;

// Config
GlobalConfig *cfg;

// Protos
void GetFile(void);
void main_nodisplay();
void main_menu();

// for debugging:
extern void plan_get_current_position_xyz(float *x, float *y, float *z);
extern PwmOut pwm;
extern "C" void mbed_reset();

/**
*** Main function
**/
int main() 
{
  systime.start();
  //float x, y, z;
  eth_speed = 1;
  
  dsp = new LaosDisplay();
  printf( VERSION_STRING "...\nBOOT...\n" ); 
  mnu = new LaosMenu(dsp);
  eth_speed=0;

  mnu->SetScreen(VERSION_STRING);
  printf("START...\n");
  cfg =  new GlobalConfig("/local/config.txt");
  mnu->SetScreen("CONFIG OK...."); 
  printf("CONFIG OK...\n");
  if (!cfg->nodisplay)
    dsp->testI2C();
  mnu->SetPosition(cfg->xhome, cfg->yhome, cfg->zhome);
  
  printf("MOTION...\n"); 
  mot = new LaosMotion();

  printf("TEST SD...\n"); 
  FILE *fp = sd.openfile("test.txt", "wb");
  if ( fp == NULL )
  {
    mnu->SetScreen("SD NOT READY!"); 
    wait(2.0);
    mbed_reset();
  }
  else
  {
    printf("SD: READY...\n");
    fclose(fp);
    removefile("test.txt");
  }
    
  eth = EthConfig();
  eth_speed=1;
      
  printf("SERVER...\n");
  srv = new TFTPServer("/sd");
  mnu->SetScreen("SERVER OK...."); 
  wait(0.5);
  mnu->SetScreen(9); // IP
  wait(1.0);
  
  printf("RUN...\n");
  
  // Wait for key, and then home
  
  if ( cfg->autohome )
  {
    printf("HOME...\n");
    wait(1);
  
    mnu->SetScreen("HOME....");
  
  // Start homing
    while ( !mot->isStart() );
    mot->home(cfg->xhome,cfg->yhome);
    // if ( !mot->isHome ) exit(1);
    printf("HOME DONE. (%d,%d)\n",cfg->xhome,cfg->yhome);
  }
  else
    printf("Homing skipped: %d\n", cfg->autohome);
  // mnu->x = x0;
  // mnu->y = y0;
  // mnu->z = z0;
  // mot->reset();

  mnu->SetScreen(NULL);  

  if (cfg->nodisplay) {
    printf("No display set\n\r");
    main_nodisplay();
  } else {
    printf("Entering display\n\r");
    main_menu();
  }
}

void main_nodisplay() {
  float x, y, z = 0;
  
  // main loop  
   while(1) 
  {  
    led1=led2=led3=led4=0;
    mnu->SetScreen("Wait for file ...");
    while (srv->State() == listen)
        Net::poll();
    GetFile();
    
    plan_get_current_position_xyz(&x, &y, &z);
     printf("%f %f\n", x,y); 
    mnu->SetScreen("Laser BUSY..."); 
    
    char name[21];
    srv->getFilename(name);
    printf("Now processing file: %s\n\r", name);
    FILE *in = sd.openfile(name, "r");
    while (!feof(in))
    { 
      while (!mot->ready() );
      mot->write(readint(in));
    }
    fclose(in);
    removefile(name);
    // done
    printf("DONE!...\n");
    mot->moveTo(cfg->xrest, cfg->yrest, cfg->zrest);
  }
}

void main_menu() {
  // main loop  
  while (1) {
        led1=led2=led3=led4=0;
        //plan_get_current_position_xyz(&x, &y, &z);
        
        mnu->SetScreen(1);
        while (1) {
            mnu->Handle();
            Net::poll();
            if (srv->State() != listen) {
                GetFile();
                char myname[32];
                srv->getFilename(myname);
                mnu->SetFileName(myname);
                mnu->SetScreen(2);
            }           
        }
    }
}

/**
*** Get file from network and save on SDcard
*** Ascii data is read from the network, and saved on the SD card in binary int32 format
**/
void GetFile(void) {
   Timer t;
   printf("Main::GetFile()\n\r" );
   mnu->SetScreen("Receive file...");
   t.start();
   while (srv->State() != listen) {
     Net::poll();
     switch ((int)t.read()) {
        case 1:
            mnu->SetScreen("Receive file");
            break;
        case 2:
            mnu->SetScreen("Receive file.");
            break;
        case 3:
            mnu->SetScreen("Receive file..");
            break;
        case 4:
            mnu->SetScreen("Receive file...");
            t.reset();
            break;
     }
   }
   mnu->SetScreen("Received file.");
} // GetFile

