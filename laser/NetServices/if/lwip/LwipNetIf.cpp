
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

#include "LwipNetIf.h"
#include "lwip/init.h"
#include "lwip/tcp_impl.h"
#include "lwip/dns.h"

#include "lwipNetTcpSocket.h"
#include "lwipNetUdpSocket.h"
#include "lwipNetDnsRequest.h"

#include "netCfg.h"
#if NET_LWIP_STACK

//See doc/rawapi.txt for details

LwipNetIf::LwipNetIf() : NetIf(), m_tcpTimer(), m_dnsTimer(), m_init(false)
{  

}


LwipNetIf::~LwipNetIf()
{

}

void LwipNetIf::init()
{
  if(m_init)
    return;
  m_init=true;
  lwip_init(); //init lwip, see init.c for details
  
  //Setup Clocks
  m_tcpTimer.start();
  m_dnsTimer.start();
  //m_tcpTicker.attach_us( tcp_tmr, TCP_TMR_INTERVAL * 1000 ); //TCP_TMR_INTERVAL = 250 ms in tcp_impl.h
  //m_dnsTicker.attach_us( dns_tmr, DNS_TMR_INTERVAL * 1000 ); //DNS_TMR_INTERVAL = 1000 ms in dns.h
}

NetTcpSocket* LwipNetIf::tcpSocket()  //Create a new tcp socket
{
  return new LwipNetTcpSocket();
}

NetUdpSocket* LwipNetIf::udpSocket()  //Create a new udp socket
{
  return new LwipNetUdpSocket();
}

NetDnsRequest* LwipNetIf::dnsRequest(const char* hostname) //Create a new NetDnsRequest object
{
  return new LwipNetDnsRequest(hostname);
}

NetDnsRequest* LwipNetIf::dnsRequest(Host* pHost) //Create a new NetDnsRequest object
{
  return new LwipNetDnsRequest(pHost);
}

void LwipNetIf::poll()
{
  if(m_init && m_tcpTimer.read_ms() >= TCP_TMR_INTERVAL)
  {
    m_tcpTimer.reset();
    tcp_tmr(); //Poll LwIP
  }
  if(m_init && m_dnsTimer.read_ms() >= DNS_TMR_INTERVAL)
  {
    m_dnsTimer.reset();
    dns_tmr();
  }
  //Do some stuff...
}

#endif
