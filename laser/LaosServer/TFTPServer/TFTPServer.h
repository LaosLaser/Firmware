/**
 * TFTPServer.h
 * Simple TFTP server 
 *
 * Copyright (c) 2011 Jaap Vermaas
 *
 *   This file is part of the LaOS project (see: http://wiki.laoslaser.org
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
 * Minimal TFTP Server
 *      * Receive and send files via TFTP
 *      * Server handles only one transfer at a time
 *      * Supports only binary mode transfers, no (net)ascii
 *      * fixed block size: 512 bytes
 *
 * http://spectral.mscs.mu.edu/RFC/rfc1350.html
 *
 * Example:
 * @code 
 * TFTPServer *srv;
 * ...
 * srv = new TFTPServer();
 * ...
 * @endcode
 *
 */

#ifndef _TFTPSERVER_H_
#define _TFTPSERVER_H_

#include <stdio.h>
#include <ctype.h>
#include "mbed.h"
#include "laosfilesystem.h"
#include "EthernetInterface.h"
//#include "Watchdog.h"
//#include "UDPSocket.h"      // http://mbed.org/users/donatien/programs/EthernetNetIf

#define TFTP_PORT 69
//#define TFTP_DEBUG(x) printf("%s\n\r", x);

enum TFTPServerState { listen, reading, writing, tftperror, suspended, deleted }; 

class TFTPServer {

public:
    // create a new tftp server, with file directory dir and 
    // listening on port
    TFTPServer(char* dir, int myport = TFTP_PORT);
    // destroy this instance of the tftp server
    void reset();
    // reset socket
    ~TFTPServer();
    // get current tftp status
    TFTPServerState State();
    // Temporarily disable incoming TFTP connections
    void suspend();
    // Resume after suspension
    void resume();
    // Poll for data or new connection
    void poll();
    // During read and write, this gives you the filename
    void getFilename(char* name);
    // Return number of received files
    int fileCnt();

private:
    // create a new connection reading a file from server
    void ConnectRead(char* buff);
    // create a new connection writing a file to the server
    void ConnectWrite(char* buff);
    // get DATA block from file on disk into memory
    void getBlock();
    // send DATA block to the client
    void sendBlock();
    // handle special files
    // anything called *.bin is written to /local/FIRMWARE.BIN
    // anything called config.txt is written to /local/config.txt
    // even if workdir is not /local
    int cmpHost();
    // send ACK to remote
    void Ack(int val);
    // send ERR message to named client
    void Err(char* msg);
    // check if connection mode of client is octet/binary
    int modeOctet(char* buff);
    // timed routine to avoid hanging after interrupted transfers
    void cleanUp();
    // event driven routines to handle incoming packets
    // void onListenUDPSocketEvent(UDPSocketEvent e);
    int port; // The TFTP port
    UDPSocket* ListenSock;      // main listening socket (dflt: UDP port 69)
    char workdir[256];          // file working directory
    TFTPServerState state;      // current TFTP server state
    char* remote_ip;         // connected remote Host IP
    int remote_port;            // connected remote Host Port
    int blockcnt, dupcnt;       // block counter, and DUP counter
    FILE* fp;                   // current file to read or write
    char sendbuff[516];         // current DATA block;
    int blocksize;              // last DATA block size while sending
    char filename[256];         // current (or most recent) filename
    //Ticker TFTPServerTimer;     // timeout timer
    int filecnt;                // received file counter
};

#endif
