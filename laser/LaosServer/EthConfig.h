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
#ifndef _ETHCONFIG_H_
#define _ETHCONFIG_H_
#include "global.h"
#include "EthernetInterface.h"

EthernetInterface * EthConfig();
bool EthSpeed(void);
bool EthLink(void);

#endif

