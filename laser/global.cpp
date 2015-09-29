/*
 * global.cpp
 * Read global configuration from file
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
#include "global.h"

/**
*** Return a IpAddress, from a string in the format: ppp.qqq.rrr.sss
**/
void IpParse(char *a, int i[]) 
{
    int n = 0, j;
    char c[4];

    i[0] = i[1] = i[2] = i[3] =0;
    for (n=0; n<4; n++) {
        while (*a && (*a < '0' || *a > '9'))
            a++;
        j = 0;
        while (*a && *a >= '0' && *a <= '9') 
        {
            if ( j<3 )
                c[j++] = *a;
            a++;
        }
        c[j] = 0;
        i[n] = atoi(c);
    }
}
 
 
/**
*** Global config
*** Config settings into global Config struct
**/
GlobalConfig::GlobalConfig(const std::string& filename)
{
   char *file = new char[filename.size()+1];
   strcpy(file, filename.c_str());
   printf("\r\nOpen config file: '%s'\r\n", file);
   ConfigFile cfg(file);
    if ( !cfg.IsOpen() ) 
    {
      printf("File does not exists. Using defaults\r\n");
    }
    
    // IP settings
    cfg.Value("net.ip", ip, sizeof(ip), "192.168.0.111");
    cfg.Value("net.mask", nm, sizeof(nm), "255.255.255.0");
    cfg.Value("net.gateway", gw, sizeof(gw), "192.168.0.1");
    cfg.Value("net.dns", dns, sizeof(dns), "192.168.0.1");
    cfg.Value("net.port", &port, 69);
    cfg.Value("net.dhcp", &dhcp, 0);

    // features
    cfg.Value("sys.autohome", &autohome, 0);
    cfg.Value("sys.autozhome", &autozhome, 0);
    cfg.Value("sys.nodisplay", &nodisplay, 1);
	cfg.Value("sys.cover", &cover, 1);
    cfg.Value("sys.i2cbaud", &i2cbaud, 9600);
    cfg.Value("sys.cleandir", &cleandir, 1);
    cfg.Value("sys.disablecancelcheck", &disablecancelcheck, 0);
    
    // Laser
    cfg.Value("laser.enable", &lenable, 1); // laser enable polarity [0/1]
    cfg.Value("laser.on", &lon, 1);         // laser on polarity [0/1]
    cfg.Value("laser.pwm.min", &pwmmin, 0); // pwm at minimum power [0..100]
    cfg.Value("laser.pwm.max", &pwmmax, 0); // pwm at maximum power [0..100]
    cfg.Value("laser.pwm.freq", &pwmfreq, 20000); // pwm frequency [Hz]
    cfg.Value("sys.exhaustoffdelay", &exhaust_offdelay, 30); 
	// how long to continue air assist/extract after job completion (secs)
    
    // rest position (after homing)
    cfg.Value("x.rest", &xrest, 0);
    cfg.Value("y.rest", &yrest, 0);
    cfg.Value("z.rest", &zrest, 0);
    cfg.Value("e.rest", &erest, 0);
 // (homing) direction
    cfg.Value("x.homedir", &xhomedir, 0);
    cfg.Value("y.homedir", &yhomedir, 0);
    cfg.Value("z.homedir", &zhomedir, 0);
    cfg.Value("e.homedir", &ehomedir, 0);

    cfg.Value("x.invert", &xinv, 0);
    cfg.Value("y.invert", &yinv, 0);
    cfg.Value("z.invert", &zinv, 0);
    cfg.Value("e.invert", &einv, 0);


    // (homing)sensor polarity
    cfg.Value("x.pol", &xpol, 0);
    cfg.Value("y.pol", &ypol, 0);
    cfg.Value("z.pol", &zpol, 0);
    cfg.Value("e.pol", &epol, 0);
    
    // Scaling [steps/meter]
    cfg.Value("x.scale", &xscale, 200000);
    cfg.Value("y.scale", &yscale, 200000);
    cfg.Value("z.scale", &zscale, 200000);
    cfg.Value("e.scale", &escale, 200000);
   
    // max axis speed [mm/sec]
    cfg.Value("x.speed", &xspeed, 100);
    cfg.Value("y.speed", &yspeed, 100);
    cfg.Value("z.speed", &zspeed, 100);
    cfg.Value("e.speed", &espeed, 100);
   
    // max axis acceleration [mm/sec2]
    cfg.Value("x.accel", &xaccel, 2000);
    cfg.Value("y.accel", &yaccel, 2000);
    cfg.Value("z.accel", &zaccel, 2000);
    cfg.Value("e.accel", &eaccel, 2000);
   
    // max axis speed [mm/sec]
    // home positions [um]
    cfg.Value("x.home", &xhome, 0);
    cfg.Value("y.home", &yhome, 0);
    cfg.Value("z.home", &zhome, 100000);
    cfg.Value("e.home", &ehome, 0);
     
    // min and max [um]
    cfg.Value("x.max", &xmax, GlobalConfig::VERYLARGE);
    cfg.Value("y.max", &ymax, GlobalConfig::VERYLARGE);
    cfg.Value("z.max", &zmax, GlobalConfig::VERYLARGE);
    cfg.Value("e.max", &emax, GlobalConfig::VERYLARGE); 
    cfg.Value("x.min", &xmin, GlobalConfig::MINUSVERYLARGE); 
    cfg.Value("y.min", &ymin, GlobalConfig::MINUSVERYLARGE); 
    cfg.Value("z.min", &zmin, GlobalConfig::MINUSVERYLARGE);
    cfg.Value("e.min", &emin, GlobalConfig::MINUSVERYLARGE); 
        
    // motion settings: enable output state    
    cfg.Value("motion.homespeed", &homespeed, 10); // speed during homing [usec/step]
    cfg.Value("motion.zhomespeed", &zhomespeed, 10); // z-axis speed during homing [usec/step]
    cfg.Value("motion.speed", &speed, 100);   // max speed [mm/sec]
    cfg.Value("motion.accel", &accel, 100); // accelleration [mm/sec2]
    cfg.Value("motion.enable", &enable, 0); // enable output polarity [0/1]
    cfg.Value("motion.tolerance", &tolerance, 50); // cornering tolerance [1/1000 units]

 	cfg.Value("dir_us", &dir_us, 0);
	cfg.Value("pulse_us", &pulse_us, 0);
}

// get the configured bed height (y.max - y.min), or 0 if undefined
int GlobalConfig::BedHeight() const
{
    int result=0;
    if( (ymax != (int)GlobalConfig::VERYLARGE) && (ymin != (int)GlobalConfig::MINUSVERYLARGE) )
    {
        result = ymax - ymin;
        if(result < 0) result = 0;
    }
    return result;
}

