
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

#include "UDPSocket.h"
#include "if/net/netudpsocket.h"

UDPSocket::UDPSocket() : m_pNetUdpSocket(NULL), m_pCbItem(NULL), m_pCbMeth(NULL), m_pCb(NULL)
{

}

UDPSocket::~UDPSocket() //close()
{
  close();
}
  
UDPSocketErr UDPSocket::bind(const Host& me)
{
  UDPSocketErr udpSocketErr = checkInst();
  if(udpSocketErr)
    return udpSocketErr;
  return (UDPSocketErr) m_pNetUdpSocket->bind(me); 
}
  
int /*if < 0 : UDPSocketErr*/ UDPSocket::sendto(const char* buf, int len, Host* pHost)
{
  UDPSocketErr udpSocketErr = checkInst();
  if(udpSocketErr)
    return udpSocketErr;
  return m_pNetUdpSocket->sendto(buf, len, pHost);  
}

int /*if < 0 : UDPSocketErr*/ UDPSocket::recvfrom(char* buf, int len, Host* pHost)
{
  UDPSocketErr udpSocketErr = checkInst();
  if(udpSocketErr)
    return udpSocketErr;
  return m_pNetUdpSocket->recvfrom(buf, len, pHost);
}

UDPSocketErr UDPSocket::close()
{
  if(!m_pNetUdpSocket)
    return UDPSOCKET_SETUP;
  m_pNetUdpSocket->resetOnEvent();
  UDPSocketErr udpSocketErr = (UDPSocketErr) m_pNetUdpSocket->close(); //Close (can already be closed)
  Net::releaseUdpSocket(m_pNetUdpSocket); //And release it so it can be freed when properly removed
  m_pNetUdpSocket = NULL;
  return udpSocketErr;
}

//Callbacks
void UDPSocket::setOnEvent( void (*pMethod)(UDPSocketEvent) )
{
  m_pCb = pMethod;
}

#if 0 //For info only
template<class T> 
void UDPSocket::setOnEvent( T* pItem, void (T::*pMethod)(UDPSocketEvent) )
{
  m_pCbItem = (CDummy*) pItem;
  m_pCbMeth = (void (CDummy::*)(UDPSocketEvent)) pMethod;
}
#endif
  
void UDPSocket::resetOnEvent() //Disable callback
{
  m_pCb = NULL;
  m_pCbItem = NULL;
  m_pCbMeth = NULL;
}

void UDPSocket::onNetUdpSocketEvent(NetUdpSocketEvent e)
{
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)((UDPSocketEvent) e);
  else if(m_pCb)
    m_pCb((UDPSocketEvent) e);
}

UDPSocketErr UDPSocket::checkInst()
{
  if(!m_pNetUdpSocket)
  {
    m_pNetUdpSocket = Net::udpSocket();
    if(!m_pNetUdpSocket)
    {
      return UDPSOCKET_IF; //Interface did not return a socket (usually because a default interface does not exist)
    }
    m_pNetUdpSocket->setOnEvent(this, &UDPSocket::onNetUdpSocketEvent);
  }
  return UDPSOCKET_OK;
}
