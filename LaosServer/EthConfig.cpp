/*
 * EthSetup.cpp
 * Setup ethernet interface, based on config file
 * Read IP, Gateway, DNS and port for server.
 * if IP is not defined, or dhcp is set, use dhcp for Ethernet
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
#include "EthConfig.h"

// Ethernet connection status inputs (from phy) and outputs (to leds)
#define ETH_LINK (1<<25)
#define ETH_SPEED (1<<26)
PortIn eth_conn(Port1, ETH_LINK | ETH_SPEED);   // p25: link, 0=connected, 1=NC, p26: speed, 0=100mbit, 1=10mbit


/**
*** Return Speed status, true==100mbps
**/
bool EthSpeed(void)
{
  int s = eth_conn.read();
  return !(s & ETH_SPEED) && !(s & ETH_LINK);
}

/**
*** Return Link status, true==connected
**/
bool EthLink(void)
{
  int s = eth_conn.read();
  return !(s & ETH_LINK);
}

#define IP(x) IpAddr(x[0], x[1], x[2], x[3])


/**
*** EthConfig
**/
EthernetNetIf * EthConfig()
 {
    EthernetNetIf *eth;
    if ( cfg->dhcp )
    {
        printf("DHCP...\n");
        eth = new EthernetNetIf();
    }
    else
    {
        printf("FIXED IP...\n");
        eth = new EthernetNetIf(IP(cfg->ip), IP(cfg->nm), IP(cfg->gw), IP(cfg->dns));
    }
    printf("Ethernet Setup...\n");
    if ( eth->setup() == ETH_TIMEOUT ) 
    {
        printf("Timeout!\n");
        delete eth;
        eth = new EthernetNetIf(IP(cfg->ip), IP(cfg->nm), IP(cfg->gw), IP(cfg->dns));
    }

    IpAddr myip = eth->getIp();
    cfg->ip[0] = myip[0];
    cfg->ip[1] = myip[1];
    cfg->ip[2] = myip[2];
    cfg->ip[3] = myip[3];
    
    printf("mbed IP Address is %d.%d.%d.%d\r\n", myip[0], myip[1], myip[2], myip[3]);
    return eth;
}