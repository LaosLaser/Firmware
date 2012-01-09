/**
 * LaosServer.cpp
 * Simple TCP/IP server
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
 * Sends and Receives integers via TCP/IP
 *
 @code 
 --code--
 
 @endcode
 */

#ifndef _LAOSSERVER_H_
#define _LAOSSERVER_H_

#include "global.h"
#include "EthernetNetIf.h"
#include "TCPSocket.h"
#include "laosfilesystem.h"

enum ServerState { error, listen, connected, busy };


    /** Simple TCP/IP Server
      * Create server based on config file. 
      *
      * Example:
      * @code 
      * LaosServer srv("config.txt");
      * int i = srv.read();
      * srv.write(i);
      * @endcode
      */
class LaosServer {
public:
    /** Make new LaosServer object. Open config file.
      * Note: the file handle is kept open during the lifetime of this object.
      * To close the file: destroy this ConfigFile object!
      * @param file Filename of the configuration file.
      */
    LaosServer(int port);

    ~LaosServer();

/** Read value. If socket is not open, this blocks
  * @return integer value, read from socket
  */ 
    int read();

/** Write Integer value. If socket is not open, or cannot write: block
  * @param i Integer 
  * @return "true" if the key is found "false" is key is not found (default value is returned)
  */ 
    void write(int i);
    void reset();
    ServerState state();
    int count();
    
private:
    FILE *fp;
    ServerState mystate;
    int port,total, mycount;
    TCPSocket ListeningSock;
    TCPSocket* pConnectedSock; 
    Host client;
    TCPSocketErr err;
    void onConnectedTCPSocketEvent(TCPSocketEvent e);
    void onListeningTCPSocketEvent(TCPSocketEvent e);
    
};

#endif
