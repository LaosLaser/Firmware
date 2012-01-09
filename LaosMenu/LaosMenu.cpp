/*
 * LaosMenu.cpp
 * Menu structure and user interface. Uses LaosDisplay
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
 */
#include "LaosMenu.h"

//extern int x0,y0,z0;

static const char *menus[] = {
    "STARTUP",     //0
    "MAIN",        //1 
    "MOVE",        //2
    "FOCUS",       //3
    "HOME",        //4
    "ORIGIN",      //5
    "START JOB",   //6
    "DELETE JOB",  //7 
    "IP",          //8
    "POWER / SPEED",//9
    "IO", //10
};

static const char *screens[] = {
    //0: main, navigate to  MOVE, FOCUS, HOME, ORIGIN, START JOB, IP, DELETE JOB, POWER
#define STARTUP (0)
    "$$$$$$$$$$$$$$$$" 
    "$$$$$$$$$$$$$$$$",

#define MAIN (STARTUP+1)
    "$$$$$$$$$$$$$$$$" 
    "<--- 0 ---> [ok]",
  
#define MOVE (MAIN+1)
    "X: +6543210 MOVE" 
    "Y: +6543210 [ok]",
    
#define FOCUS (MOVE+1)
    "Z: +543210 FOCUS" 
    "            [ok]",

#define HOME (FOCUS+1)
    "     HOME?      "
    "[cancel]    [ok]",  

#define ORIGIN (HOME+1)
    "  SET ORIGIN?   "
    "[cancel]    [ok]",   

#define RUN (ORIGIN+1)
    "RUN: $$$$$$$$$$$" 
    "[cancel]<10><ok>",   

#define DELETE (RUN+1)
    "DEL: $$$$$$$$$$$" 
    "[cancel]<10>[ok]",   

#define IP (DELETE+1)
    "210.210.210.210 " 
    "$$$$$$$$<0> [ok]",    

#define POWER (IP+1)
    "$$$$$$$: 6543210" 
    "[cancel] <0>[ok]",    

#define IO (POWER+1)
    "$$$$$$$$$$$=0 IO" 
    "[cancel]    [ok]",    

// Intermediate screens
#define DELETE_OK (IO+1)
    "   DELETE 10?   " 
    "<cancel>    [ok]",   

#define HOMING (DELETE_OK+1) 
    "HOMING...6543210" 
    "[cancel]        ",   
    
#define BUSY (HOMING+1)
    "BUSY: $$$$$$$$$$" 
    "[cancel]    [ok]",

#define PAUSE (BUSY+1)
    "PAUSE: $$$$$$$$$" 
    "[cancel]    [ok]",

};

static  const char *ipfields[] = { "IP", "NETMASK", "GATEWAY", "DNS" };
static  const char *powerfields[] = { "Pmin %", "Pmax %", "Voff", "Von" };
static  const char *iofields[] = { "o1:PURGE", "o2:EXHAUST", "o3:PUMP", "i1:COVER", "i2:PUMPOK", "i3:LASEROK", "i4:PURGEOK" };
  
  
/**
*** Make new menu object
**/
LaosMenu::LaosMenu(LaosDisplay *display) 
{
  waitup=timeout=iofield=ipfield=0;
  sarg = NULL;
  x=y=z=0;
  screen=prevscreen=speed=0;
  lastscreen = new LastScreen;
  menu=1;
  dsp = display;
  if ( dsp == NULL ) dsp = new LaosDisplay();
  dsp->cls();
  SetScreen(NULL);
}



/**
*** Destroy menu object
**/
LaosMenu::~LaosMenu() 
{

}

  
/**
*** Goto specific screen
**/
void LaosMenu::SetScreen(int screen) 
{
    sarg = NULL;
    this->screen = screen;
    Handle();
    Handle();
    Handle();
}
  
/**
*** Goto specific screen
**/
void LaosMenu::SetScreen(char *msg) 
{
  if ( msg == NULL ) 
  {
    sarg = NULL;
    screen = MAIN;
  }
  else if ( msg[0] == 0 ) 
  {
    screen = MAIN;
  }
  else 
  { 
    sarg = msg;
    screen = STARTUP;
  }
  prevscreen = -1; // force update
  Handle();
  Handle();
  Handle();
}


/**
*** Handle menu system
*** Read keys, an plan next action on the screen, output screen if something changed
**/
void LaosMenu::Handle()
{
  static int count=0;
  int c = dsp->read();
  if ( count++ > 10) count = 0;
  if ( c )
    timeout = 10;
  else
    if ( timeout ) timeout--;
  if ( screen != prevscreen ) 
    waitup = 1;
  if ( waitup && timeout) // if we have to wait for key-up, cancel the keypress
    c = 0;
  if ( waitup && !timeout )
    waitup=0;
  if ( !timeout )  // increase speed if we keep button pressed longer 
    speed = 1;
  else
  {
    speed += 1;
    if ( speed >= 500 ) speed = 100;    
  }

  if ( c || screen != prevscreen || count >9 ) 
  {
    
    prevscreen = screen;  
    
    switch ( screen ) 
    {
      case STARTUP:
        if ( sarg == NULL ) sarg = (char*) VERSION_STRING;
        break;
      case MAIN:
        switch ( c )
        {
          case K_RIGHT: menu+=1; waitup=1; break;
          case K_LEFT: menu-=1; waitup=1; break;
          case K_UP: lastscreen->set(MAIN); screen=MOVE; menu=MAIN; break;
          case K_DOWN: lastscreen->set(MAIN); screen=MOVE; menu=MAIN; break;
          case K_OK:  lastscreen->set(MAIN); screen=menu; menu=MAIN; break;
          case K_CANCEL: menu=MAIN; break;
          case K_FUP:  lastscreen->set(MAIN); screen=FOCUS; menu=MAIN; break;
          case K_FDOWN: lastscreen->set(MAIN); screen=FOCUS; menu=MAIN; break;
          case K_ORIGIN:  lastscreen->set(MAIN); screen=ORIGIN; waitup=MAIN; break;
        }
        menu =  menu % (sizeof(menus) / sizeof(menus[0]));
        sarg = (char*)menus[menu]; 
        args[0] = menu;
        break;
      
      case MOVE: // pos xy
        switch ( c )
        {
          case K_DOWN: y-=speed; if ((y+yoff)<0) y=0-yoff; break;
          case K_UP: y+=speed; if ((y+yoff)>cfg->ymax) y=cfg->ymax-yoff; break;
          case K_LEFT: x-=speed; if ((x+xoff)<0) x=0-xoff; break;
          case K_RIGHT: x+=speed; if ((x+xoff)>cfg->xmax) x=cfg->xmax-xoff; break;
          case K_OK: case K_CANCEL: screen=lastscreen->prev(); waitup=1; break;
          case K_FUP:  lastscreen->set(MOVE); screen=FOCUS; break;
          case K_FDOWN:  lastscreen->set(MOVE); screen=FOCUS; break;
          case K_ORIGIN:  lastscreen->set(MOVE); screen=ORIGIN; break;
        }
       args[0]=x; args[1]=y; 
        break;
        
      case FOCUS: // focus
        switch ( c )
        {
          case K_FUP: case K_UP: z+=speed; break;
          case K_FDOWN: case K_DOWN: z-=speed; break;
          // case K_LEFT: screen=MAIN; menu -= 1; break;
          // case K_RIGHT: screen=MAIN; menu += 1; break;
          case K_ORIGIN:  lastscreen->set(FOCUS); screen=ORIGIN; break;
          case 0: break;
          default: screen=lastscreen->prev(); waitup=1; break;
        } 
        args[0]=z; 
        break;

      case HOME:// home
        switch ( c )
        {
          case K_OK: screen=HOMING; break;
          case K_CANCEL: screen=lastscreen->prev(); waitup=1; break;
        }
        break;
  
      case ORIGIN: // origin
        switch ( c )
        {
          case K_OK:
                //mot->setPosition(0,0,0);
                x = y = z = 0;
                screen = lastscreen->prev();
                waitup = 1;
                break;
        }
        break;
      
      case 6: // START JOB select job to run
        switch ( c )
        {
            case K_OK: screen=lastscreen->prev(); waitup = 1; break; // INSERT: run current job
            case K_UP: job++; // next job
            case K_DOWN: job--; // prev job
            case K_CANCEL: screen=lastscreen->prev(); waitup = 1; break;
        }
        break;
          
      case IP: // IP
        switch ( c )
        {
          case K_RIGHT: ipfield++; waitup=1; break;
          case K_LEFT: ipfield--; waitup=1; break;
          case K_OK: screen=lastscreen->prev(); break;
          case K_CANCEL: screen=lastscreen->prev(); break;
        }
        ipfield %= 4;
        sarg = (char*)ipfields[ipfield];
        args[4] = ipfield;
        switch(ipfield)
        {
          case 0: memcpy(args, cfg->ip, 4*sizeof(int) ); break;
          default: memset(args,0,4*sizeof(int)); break;
        }
        break; 
        
       case IO: // IO
        switch ( c )
        {
          case K_RIGHT: iofield++; waitup=1; break;
          case K_LEFT: iofield--; waitup=1; break;
          case K_OK: screen=lastscreen->prev(); break;
          case K_CANCEL: screen=lastscreen->prev(); break;
        }
        iofield %= sizeof(iofields)/sizeof(char*);
        sarg = (char*)iofields[iofield];
        args[0] = ipfield;
        args[1] = ipfield;
        break;       
        
      case POWER: // POWER
        switch ( c )
        {
          case K_RIGHT: powerfield++; waitup=1; break;
          case K_LEFT: powerfield--; waitup=1; break;
          case K_UP: power[powerfield % 4] += speed;  break;
          case K_DOWN: power[powerfield % 4] -= speed;  break;
          case K_OK: screen=lastscreen->prev();  break;
          case K_CANCEL: screen=lastscreen->prev();  break;
        }
        powerfield %= 4;
        args[1] = powerfield;
        sarg = (char*)powerfields[powerfield];
        args[0] = power[powerfield];
        break;    


      case HOMING: // Homing screen
        //mot->home(cfg->xhome,cfg->yhome);
        x = cfg->xhome; y = cfg->yhome;
        screen=lastscreen->prev();
        break;     
        
      default: screen = MAIN; break;    
    }  
    dsp->ShowScreen(screens[screen], args, sarg);
  }
  
}
