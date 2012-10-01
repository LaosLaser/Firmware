#pragma diag_remark 1464
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

#include "lwip/ip_addr.h"
#include "lwipNetUdpSocket.h"
#include "lwip/udp.h"
#include "lwip/igmp.h"


//#define __DEBUG
#include "dbg/dbg.h"

#include "netCfg.h"
#if NET_LWIP_STACK

LwipNetUdpSocket::LwipNetUdpSocket(udp_pcb* pPcb /*= NULL*/) : NetUdpSocket(), m_pPcb(pPcb), m_lInPkt(), m_multicastGroup() //Passes a pcb if already created (by an accept req for instance), in that case transfers ownership
{
  DBG("New LwipNetUdpSocket %p (pPCb=%p)\n", (void*)this, (void*) pPcb);
  if(!m_pPcb)
    m_pPcb = udp_new();
  if(m_pPcb)
  {
    //Setup callback
    udp_recv( (udp_pcb*) m_pPcb, LwipNetUdpSocket::sRecvCb, (void*) this );
  }
}

LwipNetUdpSocket::~LwipNetUdpSocket()
{
  close();
}
  
NetUdpSocketErr LwipNetUdpSocket::bind(const Host& me)
{
  err_t err;
  
  if(!m_pPcb)
    return NETUDPSOCKET_MEM; //NetUdpSocket was not properly initialised, should destroy it & retry
   
  #if LWIP_IGMP //Multicast support enabled
  if(me.getIp().isMulticast())
  {
    DBG("This is a multicast addr, joining multicast group\n");
    m_multicastGroup = me.getIp();
    err = igmp_joingroup(IP_ADDR_ANY, &(m_multicastGroup.getStruct()));
    if(err)
      return NETUDPSOCKET_IF; //Could not find or create group
  }
  #endif

  err = udp_bind( (udp_pcb*) m_pPcb, IP_ADDR_ANY, me.getPort()); //IP_ADDR_ANY : Bind the connection to all local addresses
  if(err)
    return NETUDPSOCKET_INUSE;

  //Setup callback
  udp_recv( (udp_pcb*) m_pPcb, LwipNetUdpSocket::sRecvCb, (void*) this );
    
  return NETUDPSOCKET_OK;
}

#define MAX(a,b) ((a>b)?a:b)
#define MIN(a,b) ((a<b)?a:b)

int /*if < 0 : NetUdpSocketErr*/ LwipNetUdpSocket::sendto(const char* buf, int len, Host* pHost)
{
  if( !m_pPcb ) //Pcb doesn't exist (anymore)
    return NETUDPSOCKET_MEM;
  pbuf* p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_POOL);
  if( !p )
    return NETUDPSOCKET_MEM;
  char* pBuf = (char*) buf;
  pbuf* q = p; 
  do
  {
    memcpy (q->payload, (void*)pBuf, q->len);  
    pBuf += q->len;
    q = q->next;
  } while(q != NULL);

  err_t err = udp_sendto( (udp_pcb*) m_pPcb, p, &(pHost->getIp().getStruct()), pHost->getPort() );
  pbuf_free( p );
  if(err)
    return NETUDPSOCKET_SETUP; //Connection problem
  DBG("%d bytes sent in UDP Socket.\n", len);
  return len;
}

int /*if < 0 : NetUdpSocketErr*/ LwipNetUdpSocket::recvfrom(char* buf, int len, Host* pHost)
{
  if( !m_pPcb ) //Pcb doesn't exist (anymore)
    return NETUDPSOCKET_MEM;
  int inLen = 0;
  int cpyLen = 0;
  
  static int rmgLen = 0; 
  //Contains the remaining len in this pbuf
  
  if( m_lInPkt.empty() )
    return 0;
    
  pbuf* pBuf = (pbuf*) m_lInPkt.front().pBuf;
  
  if(pHost)
    *pHost = Host( IpAddr(&m_lInPkt.front().addr), m_lInPkt.front().port );
  
  if( !pBuf )
  {
    rmgLen = 0;
    return 0;
  }
  
  if ( !rmgLen ) //We did not know m_pReadPbuf->len last time we called this fn
  {
    rmgLen = pBuf->len;
  }
  
  while ( inLen < len )
  {
    cpyLen = MIN( (len - inLen), rmgLen ); //Remaining len to copy, remaining len in THIS pbuf
    memcpy((void*)buf, (void*)((char*)(pBuf->payload) + (pBuf->len - rmgLen)), cpyLen);
    inLen += cpyLen;
    buf += cpyLen;
    
    rmgLen = rmgLen - cpyLen; //Update rmgLen
    
    if( rmgLen > 0 )
    {
      //We did not read this pbuf completely, so let's save it's pos & return
      break;
    }
    
    if(pBuf->next)
    {
      pbuf* pNextPBuf = pBuf->next;
      pBuf->next = NULL; //So that it is not freed as well
      //We get the reference to pNextPBuf from m_pReadPbuf
      pbuf_free((pbuf*)pBuf);
      pBuf = pNextPBuf;
      rmgLen = pBuf->len;
    }
    else
    {
      pbuf_free((pbuf*)pBuf);
      pBuf = NULL;
      rmgLen = 0;
      m_lInPkt.pop_front();
      break; //No more data to read
    } 
  }
  
  return inLen;
}

NetUdpSocketErr LwipNetUdpSocket::close()
{
  DBG("LwipNetUdpSocket::close() : Closing...\n");

  if(m_closed)
    return NETUDPSOCKET_OK; //Already being closed
  m_closed = true;
  
  if( !m_pPcb ) //Pcb doesn't exist (anymore)
    return NETUDPSOCKET_MEM;
    
  DBG("LwipNetUdpSocket::close() : Cleanup...\n");
    
  //Cleanup incoming data
  cleanUp();
  

  DBG("LwipNetUdpSocket::close() : removing m_pPcb...\n");
  udp_remove( (udp_pcb*) m_pPcb);
    
  m_pPcb = NULL;
  return NETUDPSOCKET_OK;
}

NetUdpSocketErr LwipNetUdpSocket::poll()
{
  NetUdpSocket::flushEvents();
  return NETUDPSOCKET_OK;
}

// Callbacks events

void LwipNetUdpSocket::recvCb(udp_pcb* pcb, struct pbuf* p, ip_addr_t* addr, u16_t port)
{
  DBG(" Packet of length %d arrived in UDP Socket.\n", p->tot_len);
  list<InPacket>::iterator it;
  for ( it = m_lInPkt.begin(); it != m_lInPkt.end(); it++ )
  {
    if( ip_addr_cmp((&((*it).addr)), addr) && ((*it).port == port) )
    {
      //Let's tail this packet to the previous one
      pbuf_cat((pbuf*)((*it).pBuf), p);
      //No need to queue an event in that case since the read buf has not been processed yet
      return;
    }
  }

  //New host, add a packet to the queue
  InPacket pkt;
  pkt.pBuf = p;
  pkt.addr = *addr;
  pkt.port = port;
  m_lInPkt.push_back(pkt);
  
  queueEvent(NETUDPSOCKET_READABLE);
}

void LwipNetUdpSocket::cleanUp() //Flush input buffer
{
  //Ensure that further error won't be followed to this inst (which can be destroyed)
  if( m_pPcb )
  {
    udp_recv( (udp_pcb*) m_pPcb, NULL, (void*) NULL );
  }
  
  //Leaving multicast group(Ok because LwIP has a refscount for multicast group)
  #if LWIP_IGMP //Multicast support enabled
  if(m_multicastGroup.isMulticast())
  {
    igmp_leavegroup(IP_ADDR_ANY, &(m_multicastGroup.getStruct()));
    m_multicastGroup = IpAddr();
  }
  #endif
  
  list<InPacket>::iterator it;
  for ( it = m_lInPkt.begin(); it != m_lInPkt.end(); it++ )
  {
    //Free buf
    pbuf_free((pbuf*)((*it).pBuf));
  } 
  m_lInPkt.clear();
}

// Static callback from LwIp

void LwipNetUdpSocket::sRecvCb(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port)
{
  LwipNetUdpSocket* pMe = (LwipNetUdpSocket*) arg;
  return pMe->recvCb( pcb, p, addr, port );
}

#endif
