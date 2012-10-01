
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

#include "lwipNetDnsRequest.h"
extern "C"
{
    #include "lwip/err.h" //err_t, ERR_xxx
    #include "lwip/dns.h"
}
#include "netCfg.h"
#if NET_LWIP_STACK

//#define __DEBUG
#include "dbg/dbg.h"

LwipNetDnsRequest::LwipNetDnsRequest(const char* hostname) : NetDnsRequest(hostname), m_state(LWIPNETDNS_START), m_cbFired(false), m_closing(false)
{
  DBG("New LwipNetDnsRequest %p\n", this);
}

LwipNetDnsRequest::LwipNetDnsRequest(Host* pHost) : NetDnsRequest(pHost), m_state(LWIPNETDNS_START), m_cbFired(false), m_closing(false)
{
  DBG("New LwipNetDnsRequest %p\n", this);
}

LwipNetDnsRequest::~LwipNetDnsRequest()
{
  DBG("LwipNetDnsRequest %p destroyed\n", this);
}

/*
Main useful function here (see lwip/dns.h):
err_t          dns_gethostbyname(const char *hostname, ip_addr_t *addr,
                                 dns_found_callback found, void *callback_arg);
*/

//Execute request & return OK if found, NOTFOUND or ERROR on error, or PROCESSING if the request has not completed yet
void LwipNetDnsRequest::poll()
{
  err_t  err;
  switch(m_state)
  {
  case LWIPNETDNS_START: //First req, let's call dns_gethostbyname
    ip_addr_t ipStruct;
    err = dns_gethostbyname(m_hostname, &ipStruct, LwipNetDnsRequest::sFoundCb, (void*) this );
    if( err == ERR_OK )
    {
      m_ip = IpAddr(&ipStruct);
      m_state = LWIPNETDNS_OK;
      DBG("DNS: Ip found in cache.\n");
    }
    else if( err == ERR_INPROGRESS)
    {
      DBG("DNS: Processing.\n");
      m_state = LWIPNETDNS_PROCESSING;
    }
    else //Likely ERR_VAL
    {
      DBG("DNS: Error on init.\n");
      m_state = LWIPNETDNS_ERROR;
    }
    break;
  case LWIPNETDNS_PROCESSING:
    break; //Nothing to do, DNS is polled on interrupt
  case LWIPNETDNS_OK:
    if(!m_cbFired)
    {
      DBG("DNS: Ip found.\n");
      m_cbFired = true;
      onReply(NETDNS_FOUND); //Raise callback
    }
    break;
  case LWIPNETDNS_NOTFOUND:
    if(!m_cbFired)
    {
      DBG("DNS: could not be resolved.\n");
      m_cbFired = true;
      onReply(NETDNS_NOTFOUND); //Raise callback
    }  
    break;  
  case LWIPNETDNS_ERROR:
  default:
    if(!m_cbFired)
    {
      DBG("DNS: Error.\n");
      m_cbFired = true;
      onReply(NETDNS_ERROR); //Raise callback
    }  
    break; 
  }
  if(m_closing && (m_state!=LWIPNETDNS_PROCESSING)) //Check wether the closure has been reqd
  {
    DBG("LwipNetDnsRequest: Closing in poll()\n");
    NetDnsRequest::close();
  }
}

void LwipNetDnsRequest::close()
{
  DBG("LwipNetDnsRequest: Close req\n");
  if(m_state!=LWIPNETDNS_PROCESSING)
  {
    DBG("LwipNetDnsRequest: Closing in close()\n");
    NetDnsRequest::close();
  }
  else //Cannot close rightaway, waiting for callback from underlying layer
  {
    m_closing = true;
  }
}

void LwipNetDnsRequest::foundCb(const char *name, ip_addr_t *ipaddr)
{
  if( ipaddr == NULL )
  {
    DBG("LwipNetDnsRequest: Callback: Name not found\n");
    m_state = LWIPNETDNS_NOTFOUND;
    return;
  }
  DBG("LwipNetDnsRequest: Callback: Resolved\n");
  m_ip = IpAddr(ipaddr);
  m_state = LWIPNETDNS_OK;
}


void LwipNetDnsRequest::sFoundCb(const char *name, ip_addr_t *ipaddr, void *arg)
{
  DBG("LwipNetDnsRequest: Static callback\n");
  LwipNetDnsRequest* pMe = (LwipNetDnsRequest*) arg;
  return pMe->foundCb( name, ipaddr );
}

#endif

