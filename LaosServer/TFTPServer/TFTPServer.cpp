/*
 * TFTPServer.cpp
 * Simple TFTP server
 *
 * Copyright (c) 2011 Jaap Vermaas
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
 */
#include "TFTPServer.h"

// create a new tftp server, with file directory dir and
// listening on port
TFTPServer::TFTPServer(char* dir) {
    ListenSock = new UDPSocket();
    ListenSock->setOnEvent(this, &TFTPServer::onListenUDPSocketEvent);
    state = listen;
    if (ListenSock->bind(Host(IpAddr(), TFTP_PORT)))
        state = error;
    //TFTPServerTimer.attach(this, &TFTPServer::cleanUp, 5000);
    sprintf(workdir, "%s", dir);
    filecnt = 0;
}

// destroy this instance of the tftp server
TFTPServer::~TFTPServer() {
    ListenSock->resetOnEvent();
    delete(ListenSock);
    delete(remote);
    state = deleted;
}

void TFTPServer::reset() {
    ListenSock->resetOnEvent();
    delete(ListenSock);
    delete(remote);
    //TFTPServerTimer.detach();
    ListenSock = new UDPSocket();
    ListenSock->setOnEvent(this, &TFTPServer::onListenUDPSocketEvent);
    state = listen;
    if (ListenSock->bind(Host(IpAddr(), TFTP_PORT)))
        state = error;
    //TFTPServerTimer.attach(this, &TFTPServer::cleanUp, 5000);
    sprintf(filename, "");
    filecnt = 0;
}

// get current tftp status
TFTPServerState TFTPServer::State() {
    return state;
}

// Temporarily disable incoming TFTP connections
void TFTPServer::suspend() {
    state = suspended;
}

// Resume after suspension
void TFTPServer::resume() {
    if (state == suspended)
        state = listen;
}

// During read and write, this gives you the filename
void TFTPServer::getFilename(char* name) {
    sprintf(name, "%s", filename);
}

// return number of received files
int TFTPServer::fileCnt() {
    return filecnt;
}

// create a new connection reading a file from server
void TFTPServer::ConnectRead(char* buff, Host* client) {
    extern LaosFileSystem sd;
    IpAddr clientIp = client->getIp();
    int clientPort = client->getPort();
    remote = new Host(clientIp, clientPort);
    Ack(0);
    blockcnt = 0;
    dupcnt = 0;

    sprintf(filename, "%s", &buff[2]);
    
    fp = sd.openfile(filename, "rb");
    if (fp == NULL) {
        state  = listen;
        delete(remote);
        Err("Could not read file", client);
    } else {
        // file ready for reading
        blockcnt = 0;
        state = reading;
        #ifdef TFTP_DEBUG
            char debugmsg[256];
            sprintf(debugmsg, "Listen: Requested file %s from TFTP connection %d.%d.%d.%d port %d",
                filename, clientIp[0], clientIp[1], clientIp[2], clientIp[3], clientPort);
            TFTP_DEBUG(debugmsg);
        #endif
        getBlock();
        sendBlock();
    }
}

// create a new connection writing a file to the server
void TFTPServer::ConnectWrite(char* buff, Host* client) {
    extern LaosFileSystem sd;
    IpAddr clientIp = client->getIp();
    int clientPort = client->getPort();
    remote = new Host(clientIp, clientPort);
    Ack(0);
    blockcnt = 0;
    dupcnt = 0;

    sprintf(filename, "%s", &buff[2]);
    sd.shorten(filename, MAXFILESIZE);
    fp = sd.openfile(filename, "wb");
    if (fp == NULL) {
        Err("Could not open file to write", client);
        state  = listen;
        delete(remote);
    } else {
        // file ready for writing
        blockcnt = 0;
        state = writing;
        #ifdef TFTP_DEBUG 
            char debugmsg[256];
            sprintf(debugmsg, "Listen: Incoming file %s on TFTP connection from %d.%d.%d.%d clientPort %d",
                filename, clientIp[0], clientIp[1], clientIp[2], clientIp[3], clientPort);
            TFTP_DEBUG(debugmsg);
        #endif
    }
}

// get DATA block from file on disk into memory
void TFTPServer::getBlock() {
    blockcnt++;
    char *p;
    p = &sendbuff[4];
    int len = fread(p, 1, 512, fp);
    sendbuff[0] = 0x00;
    sendbuff[1] = 0x03;
    sendbuff[2] = blockcnt >> 8;
    sendbuff[3] = blockcnt & 255;
    blocksize = len+4;
}

// send DATA block to the client
void TFTPServer::sendBlock() {
    ListenSock->sendto(sendbuff, blocksize, remote);
}


// compare host IP and Port with connected remote machine
int TFTPServer::cmpHost(Host* client) {
    IpAddr clientIp = client->getIp();
    IpAddr remoteIp = remote->getIp();
    int clientPort = client->getPort();
    int remotePort = remote->getPort();
    return ((clientIp == remoteIp) && (clientPort == remotePort));
}

// send ACK to remote
void TFTPServer::Ack(int val) {
    char ack[4];
    ack[0] = 0x00;
    ack[1] = 0x04;
    if ((val>603135) || (val<0)) val = 0;
    ack[2] = val >> 8;
    ack[3] = val & 255;
    ListenSock->sendto(ack, 4, remote);
}

// send ERR message to named client
void TFTPServer::Err(char* msg, Host* client) {
    char message[32];
    strncpy(message, msg, 32);
    char err[37];
    sprintf(err, "0000%s0", message);
    err[0] = 0x00;
    err[1] = 0x05;
    err[2]=0x00;
    err[3]=0x00;
    int len = strlen(err);
    err[len-1] = 0x00;
    ListenSock->sendto(err, len, client);
    #ifdef TFTP_DEBUG
        char debugmsg[256];
        sprintf(debugmsg, "Error: %s", message);
        TFTP_DEBUG(debugmsg);
    #endif
}

// check if connection mode of client is octet/binary
int TFTPServer::modeOctet(char* buff) {
    int x = 2;
    while (buff[x++] != 0); // get beginning of mode field
    int y = x;
    while (buff[y] != 0) {
        buff[y] = tolower(buff[y]);
        y++;
    } // make mode field lowercase
    return (strcmp(&buff[x++], "octet") == 0);
}

// timed routine to avoid hanging after interrupted transfers
void TFTPServer::cleanUp() {
    extern Timer systime;
    static int lastblock;
    int now = systime.read();
    switch (state) {
        case reading:
            if (lastblock+5 < now) {
                fclose(fp);
                state=listen;
                delete(remote);
            }
            lastblock = now;
            break;
        case writing:
            if (lastblock+5 < now) {
                fclose(fp);
                state = listen;
                remove(filename);
                delete(remote);
            }
            lastblock = now;
            break;
    } // state
    lastblock = now;
}

// event driven routines to handle incoming packets
void TFTPServer::onListenUDPSocketEvent(UDPSocketEvent e) {
    cleanUp();
    extern LaosFileSystem sd;
    Host* client = new Host();
    char buff[516];
    if (e == UDPSOCKET_READABLE) {
        switch (state) {
            case listen:
                if (int len = ListenSock->recvfrom(buff, 516, client)) {
                    switch (buff[1]) {
                        case 0x01: // RRQ
                            if (modeOctet(buff))
                                ConnectRead(buff, client);
                            else
                                Err("Not in octet mode", client);
                            break;
                        case 0x02: // WRQ
                            if (modeOctet(buff))
                                ConnectWrite(buff, client);
                            else
                                Err("Not in octet mode", client);
                            break;
                        case 0x03: // DATA before connection established
                            Err("No data expected", client);
                            break;
                        case 0x04:  // ACK before connection established
                            Err("No ack expected", client);
                            break;
                        case 0x05: // ERROR packet received
                            #ifdef TFTP_DEBUG
                                TFTP_DEBUG("TFTP Eror received\n\r");
                            #endif
                            break;
                        default:    // unknown TFTP packet type
                            Err("Unknown TFTP packet type", client);
                            break;
                    } // switch buff[1]
                } // recvfrom
                break; // case listen
            case reading:
                while (int len = ListenSock->recvfrom(buff, 516, client) ) {
                    switch (buff[1]) {
                        case 0x01:
                            // if this is the receiving host, send first packet again
                            if ((cmpHost(client) && blockcnt==1)) {
                                sendBlock();
                                dupcnt++;
                            }
                            if (dupcnt>10) { // too many dups, stop sending
                                fclose(fp);
                                state=listen;
                                delete(remote);
                            }
                            break;
                        case 0x02:
                            // this should never happen, ignore
                            break; // case 0x02
                        case 0x03:
                            // we are the sending side, ignore
                            break;
                        case 0x04:
                            // last packet received, send next if there is one
                            dupcnt = 0;
                            if (blocksize == 516) {
                                getBlock();
                                sendBlock();
                            } else { //EOF
                                fclose(fp);
                                state = listen;
                                delete(remote);
                            }
                            break;
                        default:  // this includes 0x05 errors
                            fclose(fp);
                            state = listen;
                            delete(remote);
                            break;
                    } // switch (buff[1])
                } // while
                break; // reading
            case writing:
                while (int len = ListenSock->recvfrom(buff, 516, client) ) {
                    switch (buff[1]) {
                        case 0x02:
                            // if this is a returning host, send ack again
                            if (cmpHost(client)) {
                                Ack(0);
                                #ifdef TFTP_DEBUG
                                    TFTP_DEBUG("Resending Ack on WRQ");
                                #endif
                            }
                            break; // case 0x02
                        case 0x03:
                            if (cmpHost(client)) { // check if this is our partner
                                int block = (buff[2] << 8) + buff[3];
                                if ((blockcnt+1) == block) {
                                    Ack(block);
                                    // new packet
                                    char *data = &buff[4];
                                    fwrite(data, 1,len-4, fp);
                                    blockcnt++;
                                    dupcnt = 0;
                                } else {
                                    if ((blockcnt+1) < block) { // high block nr
                                        // we missed a packet, error
                                        #ifdef TFTP_DEBUG
                                            TFTP_DEBUG("Missed packet!");
                                        #endif
                                        fclose(fp);
                                        state = listen;
                                        remove(filename);
                                        delete(remote);
                                    } else { // duplicate packet, do nothing
                                        #ifdef TFTP_DEBUG
                                            char debugmsg[256];
                                            sprintf(debugmsg, "Dupblicate packet %d", blockcnt);
                                            TFTP_DEBUG(debugmsg);
                                        #endif
                                        if (dupcnt > 10) {
                                            Err("Too many dups", client);
                                            fclose(fp);
                                            remove(filename);
                                            state = listen;
                                        } else {
                                            Ack(blockcnt);
                                        }
                                        dupcnt++;
                                    }
                                }
                                //printf ("Read packet %d with blocksize = %d\n\r", blockcnt, len);
                                if (len<516) {
                                    #ifdef TFTP_DEBUG
                                        char debugmsg[256];
                                        sprintf(debugmsg, "Read last block %d", len);
                                        TFTP_DEBUG(debugmsg);
                                    #endif
                                    fclose(fp);
                                    state = listen;
                                    delete(remote);
                                    filecnt++;
                                }
                                break; // case 0x03
                            }  else // if cmpHost
                                Err("Wrong IP/Port: Not your connection!", client);
                            break; // case 0x03
                        default:
                            Err("No idea why you're sending me this!", client);
                            break; // default
                    } // switch (buff[1])
                } // while
                break; // writing
            case suspended:
                if (int len = ListenSock->recvfrom(buff, 516, client))
                    Err("Packet received on suspended socket, discarded", client);
                break;
        } // state
    } else {
        #ifdef TFTP_DEBUG
            TFTP_DEBUG("TFTP unknown UDP status");
        #endif
    }
    delete(client);
}
