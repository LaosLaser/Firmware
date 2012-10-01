
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

#include "netdnsrequest.h"
#include <string.h>

NetDnsRequest::NetDnsRequest(const char* hostname) : NetService(), m_ip(0,0,0,0), m_pCbItem(NULL), m_pCbMeth(NULL), m_pHost(NULL), m_closed(false)
{
  m_hostname = new char[strlen(hostname)+1];
  strcpy( m_hostname, hostname );
}

NetDnsRequest::NetDnsRequest(Host* pHost)  : NetService(), m_ip(0,0,0,0), m_pCbItem(NULL), m_pCbMeth(NULL), m_closed(false)
{
  m_hostname = (char*) pHost->getName();
  m_pHost = pHost;
}

NetDnsRequest::~NetDnsRequest()
{
  close();
}
   
void NetDnsRequest::getResult(IpAddr* pIp)
{
  if( pIp != NULL )
    *pIp = m_ip;
}

void NetDnsRequest::close()
{
  if(m_closed)
    return;
  m_closed = true;
  if( !m_pHost )
    delete[] m_hostname;
  NetService::close();
}
  
void NetDnsRequest::onReply(NetDnsReply reply) //Must be called by impl when the request completes
{
  if( m_pHost )
    m_pHost->setIp(m_ip);
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)(reply);
}
