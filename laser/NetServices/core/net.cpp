
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

#include "net.h"

#include "host.h"
#include "ipaddr.h"
#include "netservice.h"
#include "if/net/netif.h"
#include "if/net/nettcpsocket.h"
#include "if/net/netudpsocket.h"
#include "if/net/netdnsrequest.h"

//#define __DEBUG
#include "dbg/dbg.h"

Net::Net() : m_defaultIf(NULL), m_lpIf(), m_lpNetTcpSocket(), m_lpNetUdpSocket()
{

}

Net::~Net()
{

}


void Net::poll()
{
  //DBG("\r\nNet : Services polling\r\n");

  //Poll Services
  NetService::servicesPoll();  
  
  //DBG("\r\nNet : Interfaces polling\r\n");   
      
  //Poll Interfaces
  list<NetIf*>::iterator pIfIt;

  for ( pIfIt = net().m_lpIf.begin() ; pIfIt != net().m_lpIf.end(); pIfIt++ )
  {
    (*pIfIt)->poll();
  }
  
  //DBG("\r\nNet : Sockets polling\r\n");
  
  //Poll Tcp Sockets
  list<NetTcpSocket*>::iterator pNetTcpSocketIt;

  for ( pNetTcpSocketIt = net().m_lpNetTcpSocket.begin() ; pNetTcpSocketIt != net().m_lpNetTcpSocket.end(); )
  {
    (*pNetTcpSocketIt)->poll();
    
    if( (*pNetTcpSocketIt)->m_closed && !((*pNetTcpSocketIt)->m_refs) )
    {
      (*pNetTcpSocketIt)->m_removed = true;
      delete (*pNetTcpSocketIt);
      (*pNetTcpSocketIt) = NULL;
      pNetTcpSocketIt = net().m_lpNetTcpSocket.erase(pNetTcpSocketIt);
    }
    else
    {
      pNetTcpSocketIt++;
    }
  }
  
  //Poll Udp Sockets
  list<NetUdpSocket*>::iterator pNetUdpSocketIt;

  for ( pNetUdpSocketIt = net().m_lpNetUdpSocket.begin() ; pNetUdpSocketIt != net().m_lpNetUdpSocket.end(); )
  {
    (*pNetUdpSocketIt)->poll();
    
    if( (*pNetUdpSocketIt)->m_closed && !((*pNetUdpSocketIt)->m_refs) )
    {
      (*pNetUdpSocketIt)->m_removed = true;
      delete (*pNetUdpSocketIt);
      (*pNetUdpSocketIt) = NULL;
      pNetUdpSocketIt = net().m_lpNetUdpSocket.erase(pNetUdpSocketIt);
    }
    else
    {
      pNetUdpSocketIt++;
    }
  }


}

NetTcpSocket* Net::tcpSocket(NetIf& netif) {
  NetTcpSocket* pNetTcpSocket = netif.tcpSocket();
  pNetTcpSocket->m_refs++;
  return pNetTcpSocket; 
}

NetTcpSocket* Net::tcpSocket() { //NetTcpSocket on default if
 if ( net().m_defaultIf == NULL )
      return NULL;
  NetTcpSocket* pNetTcpSocket = net().m_defaultIf->tcpSocket();
  pNetTcpSocket->m_refs++;
  return pNetTcpSocket;
}

void Net::releaseTcpSocket(NetTcpSocket* pNetTcpSocket)
{
  pNetTcpSocket->m_refs--;
  if(!pNetTcpSocket->m_closed && !pNetTcpSocket->m_refs)
    pNetTcpSocket->close();
}

NetUdpSocket* Net::udpSocket(NetIf& netif) {
  NetUdpSocket* pNetUdpSocket = netif.udpSocket();
  pNetUdpSocket->m_refs++;
  return pNetUdpSocket; 
}

NetUdpSocket* Net::udpSocket() { //NetTcpSocket on default if
 if ( net().m_defaultIf == NULL )
      return NULL;
  NetUdpSocket* pNetUdpSocket = net().m_defaultIf->udpSocket();
  pNetUdpSocket->m_refs++;
  return pNetUdpSocket;
}

void Net::releaseUdpSocket(NetUdpSocket* pNetUdpSocket)
{
  pNetUdpSocket->m_refs--;
  if(!pNetUdpSocket->m_closed && !pNetUdpSocket->m_refs)
    pNetUdpSocket->close();
}

NetDnsRequest* Net::dnsRequest(const char* hostname, NetIf& netif) {
  return netif.dnsRequest(hostname);
}

NetDnsRequest* Net::dnsRequest(const char* hostname) { //Create a new NetDnsRequest object from default if
  if ( net().m_defaultIf == NULL )
      return NULL;
  return net().m_defaultIf->dnsRequest(hostname);
}

NetDnsRequest* Net::dnsRequest(Host* pHost, NetIf& netif)
{
  return netif.dnsRequest(pHost);
}

NetDnsRequest* Net::dnsRequest(Host* pHost) //Creats a new NetDnsRequest object from default if
{
  if ( net().m_defaultIf == NULL )
      return NULL;
  return net().m_defaultIf->dnsRequest(pHost);
}

void Net::setDefaultIf(NetIf& netif) {
  net().m_defaultIf = &netif;
}

void Net::setDefaultIf(NetIf* pIf)
{
  net().m_defaultIf = pIf;
}

NetIf* Net::getDefaultIf() {
  return net().m_defaultIf;
}

void Net::registerIf(NetIf* pIf)
{
  net().m_lpIf.push_back(pIf);
}

void Net::unregisterIf(NetIf* pIf)
{
  if( net().m_defaultIf == pIf )
    net().m_defaultIf = NULL;
  net().m_lpIf.remove(pIf);
}

void Net::registerNetTcpSocket(NetTcpSocket* pNetTcpSocket)
{
  net().m_lpNetTcpSocket.push_back(pNetTcpSocket);
  DBG("\r\nNetTcpSocket [ + %p ] %d\r\n", (void*)pNetTcpSocket, net().m_lpNetTcpSocket.size());
}

void Net::unregisterNetTcpSocket(NetTcpSocket* pNetTcpSocket)
{
  DBG("\r\nNetTcpSocket [ - %p ] %d\r\n", (void*)pNetTcpSocket, net().m_lpNetTcpSocket.size() - 1);
  if(!pNetTcpSocket->m_removed)
    net().m_lpNetTcpSocket.remove(pNetTcpSocket);
}

void Net::registerNetUdpSocket(NetUdpSocket* pNetUdpSocket)
{
  net().m_lpNetUdpSocket.push_back(pNetUdpSocket);
  DBG("\r\nNetUdpSocket [ + %p ] %d\r\n", (void*)pNetUdpSocket, net().m_lpNetUdpSocket.size());
}

void Net::unregisterNetUdpSocket(NetUdpSocket* pNetUdpSocket)
{
  DBG("\r\nNetUdpSocket [ - %p ] %d\r\n", (void*)pNetUdpSocket, net().m_lpNetUdpSocket.size() - 1);
  if(!pNetUdpSocket->m_removed)
    net().m_lpNetUdpSocket.remove(pNetUdpSocket);
}

Net& Net::net()
{
  static Net* pInst = new Net(); //Called only once
  return *pInst;
}
