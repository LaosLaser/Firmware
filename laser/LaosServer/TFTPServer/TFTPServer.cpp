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

TFTPServer::TFTPServer(int myport) {
    port = myport;
    connect_cnt = 0;
    printf("TFTPServer(): port=%d\n", myport);
    ListenSock = new UDPSocket();
    SendSock = new UDPSocket();
    state = listen;
    if (ListenSock->bind(port))
        state = tftperror;
    if (SendSock->bind(2048))
        state = tftperror;
    ListenSock->set_blocking(false, 1);
    SendSock->set_blocking(false, 1);
    filecnt = 0;
}

// destroy this instance of the tftp server
TFTPServer::~TFTPServer() {
    ListenSock->close();
    delete(ListenSock);
    strcpy(remote_ip,"");
    state = deleted;
}

void TFTPServer::reset() {
    ListenSock->close();
    delete(ListenSock);
    SendSock->close();
    delete(SendSock);
    strcpy(remote_ip,"");
    ListenSock = new UDPSocket();
    SendSock = new UDPSocket();
    state = listen;
    if (ListenSock->bind(port))
        state = tftperror;
    ListenSock->set_blocking(false, 1);
    SendSock->set_blocking(false, 1);
    strcpy(filename, "");
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
void TFTPServer::ConnectRead(char* buff) {
    extern LaosFileSystem sd;
    remote_ip = client.get_address();
    remote_port = client.get_port();
    Ack(0);
    blockcnt = 0;
    dupcnt = 0;
    connect_cnt++;
    sprintf(filename, "%s", &buff[2]);
    
    if (modeOctet(buff))
        fp = sd.openfile(filename, "rb");
    else
        fp = sd.openfile(filename, "r");
    if (fp == NULL) {
        state  = listen;
        Err("Could not read file");
    } else {
        // file ready for reading
        blockcnt = 0;
        state = reading;
        #ifdef TFTP_DEBUG
            char debugmsg[128];
            sprintf(debugmsg, "Listen: Requested file %s from TFTP connection %d.%d.%d.%d:s%d",
                filename, (int)remote_ip [0], (int)remote_ip [1], (int)remote_ip [2], (int)remote_ip [3],  (int)remote_port);
            TFTP_DEBUG(debugmsg);
        #endif
        getBlock();
        sendBlock();
    }
}

// create a new connection writing a file to the server
void TFTPServer::ConnectWrite(char* buff) {
    extern LaosFileSystem sd;
    // printf("ConnectWrite()\n");
    remote_ip = client.get_address();
    remote_port = client.get_port();
    Ack(0);
    blockcnt = 0;
    dupcnt = 0;
    connect_cnt++;

    sprintf(filename, "%s", &buff[2]);
    sd.shorten(filename, MAXFILESIZE);
    // printf("filename: %s\n", filename);
    
    if (modeOctet(buff))
        fp = sd.openfile(filename, "wb");
    else
        fp = sd.openfile(filename, "w");
    if (fp == NULL) {
        Err("Could not open file to write");
        state  = listen;
        strcpy(remote_ip,"");
    } else {
        // file ready for writing
        blockcnt = 0;
        state = writing;
        // printf("ready for writing");
        #ifdef TFTP_DEBUG 
            char debugmsg[256];
            sprintf(debugmsg, "Listen: Incoming file %s on TFTP connection from %d.%d.%d.%d clientPort %d",
                filename, remote_ip [0], remote_ip [1], remote_ip [2], remote_ip [3],  remote_port);
            TFTP_DEBUG(debugmsg);
        #endif
    }
    // printf("Done.\n");
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
    SendSock->sendTo(client, sendbuff, blocksize);
}


// compare host IP and Port with connected remote machine
int TFTPServer::cmpHost() {
    char ip[17];
    strcpy(ip, client.get_address());
    int port = client.get_port();
    return ((strcmp(ip, remote_ip) == 0) && (port == remote_port));
}


// send ACK to remote
void TFTPServer::Ack(int val) {
    char ack[4];
   // printf("Ack(%d)\n", val);
    ack[0] = 0x00;
    ack[1] = 0x04;
    if ((val>603135) || (val<0)) val = 0;
    ack[2] = val >> 8;
    ack[3] = val & 255;
   // printf("Ack() %d.%d.%d.%d:%d\n", (int)client.get_address()[0], (int)client.get_address()[1], (int)client.get_address()[2], (int)client.get_address()[3], (int)client.get_port() );
    SendSock->sendTo(client, ack, 4);
}

// send ERR message to named client
void TFTPServer::Err(const std::string& msg) {
    char message[32];
    char err[37];
    printf("Err(%s)\n", msg.c_str());
    strncpy(message, msg.c_str(), 32);
    sprintf(err, "0000%s0", message);
    err[0] = 0x00;
    err[1] = 0x05;
    err[2] = 0x00;
    err[3] = 0x00;
    int len = strlen(err);
    err[len-1] = 0x00;
    SendSock->sendTo(client, err, len);
    #ifdef TFTP_DEBUG
        char debugmsg[256];
        sprintf(debugmsg, "Error: %s", message);
        TFTP_DEBUG(debugmsg);
    #endif
    if ( fp != NULL ) {
		fclose(fp);
		fp=NULL;
	}
	state = listen; 	// terminate connectiom
	strcpy(remote_ip,"");
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

// event driven routines to handle incoming packets
// A bit naive: only support one client. The 1st packet will be received on TFTP port (69) then we will reply by sending packets on the WriteSocket(2048) and all
// subsequent packets will be received on that port also
void TFTPServer::poll() {
    if ((state == suspended) || (state == deleted) || (state == tftperror)) {
        return;
    }
    char buff[592];
    int len = ListenSock->receiveFrom(client, buff, sizeof(buff));
    if (len == 0) {
		len = SendSock->receiveFrom(client, buff, sizeof(buff));
    }
    if (len == 0) {
		return;
    }

    // printf("Got block with size %d, state=%d, buff[1]=%d\n\r", len, (int)state, (int)buff[1]);
           #ifdef TFTP_DEBUG 
            char debugmsg[256];	
      //      sprintf(debugmsg, "from %d.%d.%d.%d clientPort %d", (int)remote_ip [0], (int)remote_ip [1], (int)remote_ip [2], (int)remote_ip [3],  remote_port);
      //      TFTP_DEBUG(debugmsg);
        #endif
    switch (state) {
        case listen: {
            switch (buff[1]) {
                case 0x01: // RRQ
                    ConnectRead(buff);
                    break;
                case 0x02: // WRQ
                    ConnectWrite(buff);
                    break;
                case 0x03: // DATA before connection established
                    Err("No data expected");
                    break;
                case 0x04:  // ACK before connection established
                    Err("No ack expected");
                    break;
                case 0x05: // ERROR packet received
                    #ifdef TFTP_DEBUG
                        TFTP_DEBUG("TFTP Eror received\n\r");
                    #endif
                    break;
                default:    // unknown TFTP packet type
                    Err("Unknown TFTP packet type");
                    break;
            } // switch buff[1]
            break; // case listen
        }
        case reading: {
            if (cmpHost())
	            switch (buff[1]) {
	                case 0x01:
	                    // if this is the receiving host, send first packet again
	                    if (blockcnt==1) {
                            Ack(0);
	                        dupcnt++;
	                    }
	                    if (dupcnt>10) { // too many dups, stop sending
	                        Err("Too many dups");
	                    }
	                    break;
	                case 0x02:
	                    // this should never happen, ignore
	                    Err("WRQ received on open read sochet");
	                    break; // case 0x02
	                case 0x03:
	                    // we are the sending side, ignore
	                    Err("Received data package on sending socket");
	                    break;
	                case 0x04:
	                    // last packet received, send next if there is one
	                    dupcnt = 0;
	                    if (len == 516) {
	                        getBlock();
	                        sendBlock();
	                    } else { //EOF
	                        fclose(fp);
	                        state = listen;
                            strcpy(remote_ip,"");
	                    }
	                    break;
	                default:  // this includes 0x05 errors
	                    Err("Received 0x05 error message");
	                    break;
	            } // switch (buff[1])
            else 
                printf("Ignoring package from other host during RRQ");
            break; // reading
        }
        case writing: {
            if (cmpHost()) 
	            switch (buff[1]) {
	                case 0x02: {
	                    // if this is a returning host, send ack again
	                    Ack(0);
	                    #ifdef TFTP_DEBUG
	                        TFTP_DEBUG("Resending Ack on WRQ");
	                    #endif
	                    break; // case 0x02
                    }
	                case 0x03: {
	                    int block = (buff[2] << 8) + buff[3];
	                    if ((blockcnt+1) == block) {
	                        Ack(block);
	                        // new packet
	                        char *data = &buff[4];
	                        fwrite(data, 1,len-4, fp);
	                        blockcnt++;
	                        dupcnt = 0;
	                    } else { // mismatch in block nr
	                        if ((blockcnt+1) < block) { // too high
                                Err("Packet count mismatch");
	                            remove(filename);
	                         } else { // duplicate packet, send ACK again
	                            if (dupcnt > 10) {
	                                Err("Too many dups");
	                                remove(filename);
	                            } else {
	                                Ack(blockcnt);
                                    dupcnt++;
	                            }
	                        }
	                    }
                        if (len<516) {
                            Ack(blockcnt);
                            fclose(fp);
                            strcpy(remote_ip,"");
                            state = listen;
                            filecnt++;
                            printf("File receive finished\n");
                        }
	                    break; // case 0x03
                    }
	                default: {
	                     Err("No idea why you're sending me this!");
	                     break; // default
                    }
	            } // switch (buff[1])
            else {
                printf("Ignoring packege from other host during WRQ");
            }
            break; // writing
        }
        case tftperror: {
        }
        case suspended: {
        }
        case deleted: {
        }
    } // state
}
