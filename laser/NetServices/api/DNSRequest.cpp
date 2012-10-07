
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

#include "DNSRequest.h"
#include "if/net/netdnsrequest.h"

DNSRequest::DNSRequest() : m_pNetDnsRequest(NULL), m_pCbItem(NULL), m_pCbMeth(NULL), m_pCb(NULL)
{

}

DNSRequest::~DNSRequest()
{
  close();
}
  
DNSRequestErr DNSRequest::resolve(const char* hostname)
{
  if(!m_pNetDnsRequest)
  {
    m_pNetDnsRequest = Net::dnsRequest(hostname);
    if(!m_pNetDnsRequest)
    {
      return DNS_IF; //Interface did not return a NetDnsRequest (usually because a default interface does not exist)
    }
    m_pNetDnsRequest->setOnReply(this, &DNSRequest::onNetDnsReply);
    return DNS_OK;
  }
  else
  {
    return DNS_INUSE; //The previous req has not completed
  }
}

DNSRequestErr DNSRequest::resolve(Host* pHost)
{
  if(!m_pNetDnsRequest)
  {
    m_pNetDnsRequest = Net::dnsRequest(pHost);
    if(!m_pNetDnsRequest)
    {
      return DNS_IF; //Interface did not return a NetDnsRequest (usually because a default interface does not exist)
    }
    m_pNetDnsRequest->setOnReply(this, &DNSRequest::onNetDnsReply);
    return DNS_OK;
  }
  else
  {
    return DNS_INUSE; //The previous req has not completed
  }
}

//Callbacks
void DNSRequest::setOnReply( void (*pMethod)(DNSReply) )
{
  m_pCb = pMethod;
}

#if 0 //For doc only
template<class T> 
void DNSRequest::setOnReply( T* pItem, void (T::*pMethod)(DNSReply) )
{
  m_pCbItem = (CDummy*) pItem;
  m_pCbMeth = (void (CDummy::*)(DNSReply)) pMethod;
}
#endif

DNSRequestErr DNSRequest::getResult(IpAddr* pIp)
{
  if(!m_pNetDnsRequest)
  {
    return DNS_SETUP;
  }
  m_pNetDnsRequest->getResult(pIp);
  return DNS_OK;
}
  
DNSRequestErr DNSRequest::close()
{
  if(!m_pNetDnsRequest)
  {
    return DNS_SETUP;
  }
  m_pNetDnsRequest->close();
  m_pNetDnsRequest = NULL;
  return DNS_OK;
}
  
void DNSRequest::onNetDnsReply(NetDnsReply r)
{
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)((DNSReply) r);
  else if(m_pCb)
    m_pCb((DNSReply) r);
}
