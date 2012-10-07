
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

#include "netudpsocket.h"

//#define __DEBUG
#include "dbg/dbg.h"

NetUdpSocket::NetUdpSocket() : m_host(), m_client(), m_refs(0), m_closed(false), m_removed(false), m_pCbItem(NULL), m_pCbMeth(NULL), m_events()
{
  Net::registerNetUdpSocket(this);
}

NetUdpSocket::~NetUdpSocket() //close()
{
  Net::unregisterNetUdpSocket(this);
}
  
   
//Callbacks
#if 0 //Just for info
void NetUdpSocket::setOnEvent()
{
  m_pCbItem = (CDummy*) pItem;
  m_pCbMeth = (void (CDummy::*)(NetUdpSocketEvent)) pMethod;
}
#endif

void NetUdpSocket::resetOnEvent()
{
  m_pCbItem = NULL;
  m_pCbMeth = NULL;
}

void NetUdpSocket::queueEvent(NetUdpSocketEvent e)
{
  m_events.push(e);
}

void NetUdpSocket::discardEvents()
{
  while( !m_events.empty() )
  {
    m_events.pop();
  }
}

void NetUdpSocket::flushEvents() //To be called during polling
{ 
  while( !m_events.empty() )
  {
    onEvent(m_events.front());
    m_events.pop();
  }
}

void NetUdpSocket::onEvent(NetUdpSocketEvent e) //To be called during polling
{
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)(e);
}

