
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

#include "TCPSocket.h"
#include "if/net/nettcpsocket.h"

TCPSocket::TCPSocket() : m_pNetTcpSocket(NULL), m_pCbItem(NULL), m_pCbMeth(NULL), m_pCb(NULL)
{

}

TCPSocket::TCPSocket(NetTcpSocket* pNetTcpSocket) : m_pNetTcpSocket(pNetTcpSocket), m_pCbItem(NULL), m_pCbMeth(NULL), m_pCb(NULL)
{
  m_pNetTcpSocket->setOnEvent(this, &TCPSocket::onNetTcpSocketEvent);
}

TCPSocket::~TCPSocket() //close()
{
  close();
}
  
TCPSocketErr TCPSocket::bind(const Host& me)
{
  TCPSocketErr tcpSocketErr = checkInst();
  if(tcpSocketErr)
    return tcpSocketErr;
  return (TCPSocketErr) m_pNetTcpSocket->bind(me); 
}

TCPSocketErr TCPSocket::listen()
{
  TCPSocketErr tcpSocketErr = checkInst();
  if(tcpSocketErr)
    return tcpSocketErr;
  return (TCPSocketErr) m_pNetTcpSocket->listen(); 
}

TCPSocketErr TCPSocket::connect(const Host& host)
{
  TCPSocketErr tcpSocketErr = checkInst();
  if(tcpSocketErr)
    return tcpSocketErr;
  return (TCPSocketErr) m_pNetTcpSocket->connect(host); 
}

TCPSocketErr TCPSocket::accept(Host* pClient, TCPSocket** ppNewTCPSocket)
{
  TCPSocketErr tcpSocketErr = checkInst();
  if(tcpSocketErr)
    return tcpSocketErr;
  NetTcpSocket* pNewNetTcpSocket;
  tcpSocketErr = (TCPSocketErr) m_pNetTcpSocket->accept(pClient, &pNewNetTcpSocket);
  if(pNewNetTcpSocket)
    *ppNewTCPSocket = new TCPSocket(pNewNetTcpSocket);
  return tcpSocketErr;
}
  
int /*if < 0 : TCPSocketErr*/ TCPSocket::send(const char* buf, int len)
{
  TCPSocketErr tcpSocketErr = checkInst();
  if(tcpSocketErr)
    return tcpSocketErr;
  return m_pNetTcpSocket->send(buf, len);  
}

int /*if < 0 : TCPSocketErr*/ TCPSocket::recv(char* buf, int len)
{
  TCPSocketErr tcpSocketErr = checkInst();
  if(tcpSocketErr)
    return tcpSocketErr;
  return m_pNetTcpSocket->recv(buf, len);
}

TCPSocketErr TCPSocket::close()
{
  if(!m_pNetTcpSocket)
    return TCPSOCKET_SETUP;
  m_pNetTcpSocket->resetOnEvent();
  TCPSocketErr tcpSocketErr = (TCPSocketErr) m_pNetTcpSocket->close(); //Close (can already be closed)
  Net::releaseTcpSocket(m_pNetTcpSocket); //And release it so it can be freed when properly removed
  m_pNetTcpSocket = NULL;
  return tcpSocketErr;
}

//Callbacks
void TCPSocket::setOnEvent( void (*pMethod)(TCPSocketEvent) )
{
  m_pCb = pMethod;
}

#if 0 //For info only
template<class T> 
void TCPSocket::setOnEvent( T* pItem, void (T::*pMethod)(TCPSocketEvent) )
{
  m_pCbItem = (CDummy*) pItem;
  m_pCbMeth = (void (CDummy::*)(TCPSocketEvent)) pMethod;
}
#endif
  
void TCPSocket::resetOnEvent() //Disable callback
{
  m_pCb = NULL;
  m_pCbItem = NULL;
  m_pCbMeth = NULL;
}

void TCPSocket::onNetTcpSocketEvent(NetTcpSocketEvent e)
{
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)((TCPSocketEvent) e);
  else if(m_pCb)
    m_pCb((TCPSocketEvent) e);
}

TCPSocketErr TCPSocket::checkInst()
{
  if(!m_pNetTcpSocket)
  {
    m_pNetTcpSocket = Net::tcpSocket();
    if(!m_pNetTcpSocket)
    {
      return TCPSOCKET_IF; //Interface did not return a socket (usually because a default interface does not exist)
    }
    m_pNetTcpSocket->setOnEvent(this, &TCPSocket::onNetTcpSocketEvent);
  }
  return TCPSOCKET_OK;
}
