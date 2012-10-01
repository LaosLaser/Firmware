#pragma diag_remark 1293
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

#include "NTPClient.h"

#include <stdio.h>

//#define __DEBUG
#include "dbg/dbg.h"

#define NTP_PORT 123
#define NTP_CLIENT_PORT 0//50420 //Random port
#define NTP_REQUEST_TIMEOUT 15000
#define NTP_TIMESTAMP_DELTA 2208988800ull //Diff btw a UNIX timestamp (Starting Jan, 1st 1970) and a NTP timestamp (Starting Jan, 1st 1900)

#define htons( x ) ( (( x << 8 ) & 0xFF00) | (( x >> 8 ) & 0x00FF) )
#define ntohs( x ) (htons(x))

#define htonl( x ) ( (( x << 24 ) & 0xFF000000)  \
                   | (( x <<  8 ) & 0x00FF0000)  \
                   | (( x >>  8 ) & 0x0000FF00)  \
                   | (( x >> 24 ) & 0x000000FF)  )
#define ntohl( x ) (htonl(x))

NTPClient::NTPClient() : NetService(false), m_state(NTP_PING), m_pCbItem(NULL), m_pCbMeth(NULL), m_pCb(NULL),
m_watchdog(), m_timeout(0), m_closed(true), m_host(), m_pDnsReq(NULL), m_blockingResult(NTP_PROCESSING)
{
  setTimeout(NTP_REQUEST_TIMEOUT);
  DBG("\r\nNew NTPClient %p\r\n",this);
}

NTPClient::~NTPClient()
{
  close();
}

//High level setup functions
NTPResult NTPClient::setTime(const Host& host) //Blocking
{
  doSetTime(host);
  return blockingProcess();
}

NTPResult NTPClient::setTime(const Host& host, void (*pMethod)(NTPResult)) //Non blocking
{
  setOnResult(pMethod);
  doSetTime(host);
  return NTP_PROCESSING;
}

#if 0 //For doc only
template<class T> 
NTPResult NTPClient::setTime(const Host& host, T* pItem, void (T::*pMethod)(NTPResult)) //Non blocking
{
  setOnResult(pItem, pMethod);
  doSetTime(host);
  return NTP_PROCESSING;
}
#endif

void NTPClient::doSetTime(const Host& host)
{
  init();
  resetTimeout();
  m_host = host;
  if(!m_host.getPort())
  {
    m_host.setPort(NTP_PORT);
  }
  if(m_host.getIp().isNull())
  {
    //DNS query required
    m_pDnsReq = new DNSRequest();
    DBG("\r\nNTPClient : DNSRequest %p\r\n", m_pDnsReq);
    m_pDnsReq->setOnReply(this, &NTPClient::onDNSReply);
    m_pDnsReq->resolve(&m_host);
    return;
  }
  open();
}

void NTPClient::setOnResult( void (*pMethod)(NTPResult) )
{
  m_pCb = pMethod;
}

void NTPClient::close()
{
  if(m_closed)
    return;
  m_closed = true; //Prevent recursive calling or calling on an object being destructed by someone else
  m_watchdog.stop();
  m_pUDPSocket->resetOnEvent();
  m_pUDPSocket->close();
  delete m_pUDPSocket;
  if( m_pDnsReq )
  {
    m_pDnsReq->close();
    delete m_pDnsReq;
    m_pDnsReq = NULL;
  }
}

void NTPClient::poll() //Called by NetServices
{
  if( (!m_closed) && (m_watchdog.read_ms() >= m_timeout) )
  {
    onTimeout();
  }
}

void NTPClient::init() //Create and setup socket if needed
{
  if(!m_closed) //Already opened
    return;
  m_state = NTP_PING;
  m_pUDPSocket = new UDPSocket;
  m_pUDPSocket->setOnEvent(this, &NTPClient::onUDPSocketEvent);
  m_closed = false;
  DBG("NTPClient: Init OK\n");
}

void NTPClient::open()
{
  resetTimeout();
  DBG("Opening connection\n");
  m_state = NTP_PING;
  Host localhost(IpAddr(), NTP_CLIENT_PORT, "localhost"); //Any local address
  m_pUDPSocket->bind(localhost);
  if ((int)time(NULL) < 1280000000) set_time( 1280000000 ); //End of July 2010... just there to limit offset range
  process();
}

#define MIN(a,b) ((a)<(b))?(a):(b)
void NTPClient::process() //Main state-machine
{
  int len;
  Host host;
  
  switch(m_state)
  {
  case NTP_PING:
    DBG("\r\nPing\r\n");
    //Prepare NTP Packet:
    m_pkt.li = 0; //Leap Indicator : No warning
    m_pkt.vn = 4; //Version Number : 4
    m_pkt.mode = 3; //Client mode
    m_pkt.stratum = 0; //Not relevant here
    m_pkt.poll = 0; //Not significant as well
    m_pkt.precision = 0; //Neither this one is
    
    m_pkt.rootDelay = 0; //Or this one
    m_pkt.rootDispersion = 0; //Or that one
    m_pkt.refId = 0; //...
    
    m_pkt.refTm_s = 0;
    m_pkt.origTm_s = 0;
    m_pkt.rxTm_s = 0;
    m_pkt.txTm_s = htonl( NTP_TIMESTAMP_DELTA + time(NULL) ); //WARN: We are in LE format, network byte order is BE
    
    m_pkt.refTm_f = m_pkt.origTm_f = m_pkt.rxTm_f = m_pkt.txTm_f = 0;
    
    #ifdef __DEBUG
    //Hex Dump:
    DBG("\r\nDump Tx:\r\n");
    for(int i = 0; i< sizeof(NTPPacket); i++)
    {
      DBG("%02x ", *((char*)&m_pkt + i));
    }
    DBG("\r\n\r\n");
    #endif
    
    len = m_pUDPSocket->sendto( (char*)&m_pkt, sizeof(NTPPacket), &m_host );
    if(len < sizeof(NTPPacket))
      { onResult(NTP_PRTCL); close(); return; }
      
    m_state = NTP_PONG; 
          
    break;
  
  case NTP_PONG:
    DBG("\r\nPong\r\n");
    while( len = m_pUDPSocket->recvfrom( (char*)&m_pkt, sizeof(NTPPacket), &host ) )
    {
      if( len <= 0 )
        break;
      if( !host.getIp().isEq(m_host.getIp()) )
        continue; //Not our packet
      if( len > 0 )
        break;
    }
    
    if(len == 0)
      return; //Wait for the next packet
    
    if(len < 0)
      { onResult(NTP_PRTCL); close(); return; }
    
    if(len < sizeof(NTPPacket)) //TODO: Accept chunks
      { onResult(NTP_PRTCL); close(); return; }
      
    #ifdef __DEBUG
    //Hex Dump:
    DBG("\r\nDump Rx:\r\n");
    for(int i = 0; i< sizeof(NTPPacket); i++)
    {
      DBG("%02x ", *((char*)&m_pkt + i));
    }
    DBG("\r\n\r\n");
    #endif
      
    if( m_pkt.stratum == 0)  //Kiss of death message : Not good !
    {
      onResult(NTP_PRTCL); close(); return;
    }
    
    //Correct Endianness
    m_pkt.refTm_s = ntohl( m_pkt.refTm_s );
    m_pkt.refTm_f = ntohl( m_pkt.refTm_f );
    m_pkt.origTm_s = ntohl( m_pkt.origTm_s );
    m_pkt.origTm_f = ntohl( m_pkt.origTm_f );
    m_pkt.rxTm_s = ntohl( m_pkt.rxTm_s );
    m_pkt.rxTm_f = ntohl( m_pkt.rxTm_f );
    m_pkt.txTm_s = ntohl( m_pkt.txTm_s );
    m_pkt.txTm_f = ntohl( m_pkt.txTm_f );
    
    //Compute offset, see RFC 4330 p.13
    uint32_t destTm_s = (NTP_TIMESTAMP_DELTA + time(NULL));
    //int32_t origTm = (int32_t) ((uint64_t) m_pkt.origTm - NTP_TIMESTAMP_DELTA); //Convert in local 32 bits timestamps
    //int32_t rxTm = (int32_t) ((uint64_t) m_pkt.rxTm - NTP_TIMESTAMP_DELTA); //Convert in local 32 bits timestamps
    //int32_t txTm = (int32_t) ((uint64_t) m_pkt.txTm - NTP_TIMESTAMP_DELTA); //Convert in local 32 bits timestamps
   // int64_t offset = ( ( ( m_pkt.rxTm_s - m_pkt.origTm_s ) + ( m_pkt.txTm_s - destTm_s ) ) << 32 + ( ( m_pkt.rxTm_f - m_pkt.origTm_f ) + ( m_pkt.txTm_f - 0 ) ) ) / 2;
    int64_t offset = ( (int64_t)( m_pkt.rxTm_s - m_pkt.origTm_s ) + (int64_t) ( m_pkt.txTm_s - destTm_s ) ) / 2; //Avoid overflow
    DBG("\r\nSent @%d\r\n", m_pkt.txTm_s);
    DBG("\r\nOffset: %d\r\n", offset);
    //Set time accordingly
    set_time( time(NULL) + (offset /*>> 32*/) );
    
    onResult(NTP_OK);
    close();
      
    break;
  }
}

void NTPClient::setTimeout(int ms)
{
  m_timeout = ms;
}

void NTPClient::resetTimeout()
{
  m_watchdog.reset();
  m_watchdog.start();
}

void NTPClient::onTimeout() //Connection has timed out
{
  close();
  onResult(NTP_TIMEOUT);
}

void NTPClient::onDNSReply(DNSReply r)
{
  if(m_closed)
  {
    DBG("\r\nWARN: Discarded\r\n");
    return;
  }
  
  if( r != DNS_FOUND )
  {
    DBG("\r\nCould not resolve hostname.\r\n");
    onResult(NTP_DNS);
    close();
    return;
  }
  DBG("\r\nDNS resolved.\r\n");
  m_pDnsReq->close();
  delete m_pDnsReq;
  m_pDnsReq=NULL;
  open();
}
  
void NTPClient::onUDPSocketEvent(UDPSocketEvent e)
{
  resetTimeout();
  switch(e)
  {
  case UDPSOCKET_READABLE: //The only event for now
    resetTimeout();
    process();
    break;
  }
}

void NTPClient::onResult(NTPResult r) //Must be called by impl when the request completes
{
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)(r);
  else if(m_pCb)
    m_pCb(r);
  m_blockingResult = r; //Blocking mode
}

NTPResult NTPClient::blockingProcess() //Called in blocking mode, calls Net::poll() until return code is available
{
  m_blockingResult = NTP_PROCESSING;
  do
  {
    Net::poll();
  } while(m_blockingResult == NTP_PROCESSING);
  Net::poll(); //Necessary for cleanup
  return m_blockingResult;
}
