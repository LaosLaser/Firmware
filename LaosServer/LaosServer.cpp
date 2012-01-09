/*
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
 */
#include "LaosServer.h"

// Destroy the server
LaosServer::~LaosServer() 
{

}


void LaosServer::reset() 
{
  mystate = listen;
}

// Make new server
LaosServer::LaosServer(int port) 
{
// Set the callbacks for Listening
  ListeningSock.setOnEvent(this, &LaosServer::onListeningTCPSocketEvent); 
  fp = NULL;
  mystate = listen;
  mycount=0;
  // bind and listen on TCP
  err=ListeningSock.bind(Host(IpAddr(),port));
  printf("Binding..\r\n");
  if(err)
  {
   //Deal with that error...
    printf("Binding Error\n");
  }
   err=ListeningSock.listen(); // Starts listening
   printf("Listening...\r\n");
   if(err)
   {
    printf("Listening Error\r\n");
   }
   if ( err ) mystate = error;
   pConnectedSock = NULL;
}

ServerState LaosServer::state()
{
  return this->mystate;
}


int LaosServer::count()
{
  return this->mycount;
}

// Read value
int LaosServer::read()
{
  char buf;
  if ( pConnectedSock != NULL )
  {
     int len = pConnectedSock->recv(&buf, 1);
     if ( len )
       return buf;
   }
   else
     return -2;
  return -1;
}

// Write int value
void LaosServer::write(int i)
{

}



// Connected socket
void LaosServer::onConnectedTCPSocketEvent(TCPSocketEvent e)
{
    extern LaosFileSystem sd;
   switch(e)
    {
    case TCPSOCKET_CONNECTED:
        printf("TCP Socket Connected\r\n");
        this->mystate = connected;
        total = 0;
        break;
    case TCPSOCKET_WRITEABLE:
      //Can now write some data...
        printf("TCP Socket Writable\r\n");
        break;
    case TCPSOCKET_READABLE:
      //Can now read dome data...
       // printf("TCP Socket Readable\r\n");
       // Read in any available data into the buffer
       if ( fp == NULL )
       {
         char name[16];
         printf("Open file %d!\n", mycount+1);
         sprintf(name, "%d.txt", mycount+1);
         fp = sd.openfile(name, "wb");
         if ( fp == NULL ) printf("Could not open file!\n");
        }
        char buff[512];
        while ( int len = pConnectedSock->recv(buff, sizeof(buff) ) ) 
        {
          if ( fp != NULL ) 
            fwrite(buff, 1, len, fp);
          total += len;
          mystate = busy;  
        }
        //int len = pConnectedSock->recv(buff, 10);
       //printf("R%d\n", len);
       break;
    case TCPSOCKET_CONTIMEOUT:
        printf("TCP Socket Timeout\r\n");
        break;
    case TCPSOCKET_CONRST:
        printf("TCP Socket CONRST\r\n");
        break;
    case TCPSOCKET_CONABRT:
        printf("TCP Socket CONABRT\r\n");
        break;
    case TCPSOCKET_ERROR:
        printf("TCP Socket Error\r\n");
        break;
    case TCPSOCKET_DISCONNECTED:
    //Close socket...
        printf("TCP Socket Disconnected (%d bytes received)\r\n", total);        
        if ( fp != NULL )
        {
          fclose(fp);
          fp = NULL;
        }
        mycount++;
        mystate = connected;
        pConnectedSock->close();
        pConnectedSock = NULL;
        break;
    default:
        printf("DEFAULT\r\n"); 
      }
}

// Listening socket
void LaosServer::onListeningTCPSocketEvent(TCPSocketEvent e)
{
    switch(e)
    {
    case TCPSOCKET_ACCEPT:
        printf("Listening: TCP Socket Accepted\r\n");
        // Accepts connection from client and gets connected socket.   
        err=ListeningSock.accept(&client, &pConnectedSock);
        if (err) {
            printf("onListeningTcpSocketEvent : Could not accept connection.\r\n");
            return; //Error in accept, discard connection
        }
        // Setup the new socket events
        pConnectedSock->setOnEvent(this, &LaosServer::onConnectedTCPSocketEvent);
        // We can find out from where the connection is coming by looking at the
        // Host parameter of the accept() method
        IpAddr clientIp = client.getIp();
        printf("Listening: Incoming TCP connection from %d.%d.%d.%d\r\n", 
           clientIp[0], clientIp[1], clientIp[2], clientIp[3]);
        this->mystate = connected;
        total = 0;
       break;
    default:
        printf("Listening socket: DEFAULT\r\n"); 
     };
}

