
/*
Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "HTTPServer.h"

//#define __DEBUG
#include "dbg/dbg.h"

HTTPServer::HTTPServer()
{
  m_pTCPSocket = new TCPSocket;
  m_pTCPSocket->setOnEvent(this, &HTTPServer::onTCPSocketEvent);
}

HTTPServer::~HTTPServer()
{
  delete m_pTCPSocket;
}

void HTTPServer::bind(int port /*= 80*/)
{
  Host h(IpAddr(127,0,0,1), port, "localhost");
  m_pTCPSocket->bind(h);     
  m_pTCPSocket->listen(); //Listen
}

#if 0 //Just for clarity
template<typename T>
void HTTPServer::addHandler(const char* path)
{
  m_lpHandlers[path] = &T::inst;
  
}
#endif
  
void HTTPServer::onTCPSocketEvent(TCPSocketEvent e)
{

  DBG("\r\nHTTPServer::onTCPSocketEvent : Event %d\r\n", e);

  if(e==TCPSOCKET_ACCEPT)
  {
    TCPSocket* pTCPSocket;
    Host client;

    if( !!m_pTCPSocket->accept(&client, &pTCPSocket) )
    {
      DBG("\r\nHTTPServer::onTCPSocketEvent : Could not accept connection.\r\n");
      return; //Error in accept, discard connection
    }
    
    HTTPRequestDispatcher* pDispatcher = new HTTPRequestDispatcher(this, pTCPSocket); //TCPSocket ownership is passed to dispatcher
    //The dispatcher object will destroy itself when done, or will be destroyed on Server destruction
   
  }
  
}
