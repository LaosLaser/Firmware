
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

#include "url.h"

Url::Url() : m_port(0)
{
  
}

string Url::getProtocol()
{
  return m_protocol;
}

string Url::getHost()
{
  return m_host;
}

bool Url::getHostIp(IpAddr* ip)
{
  unsigned int ipArr[4] = {0};
  if ( sscanf(m_host.c_str(), "%u.%u.%u.%u", &ipArr[0], &ipArr[1], &ipArr[2], &ipArr[3]) != 4 )
  {
    return false;
  }
  *ip = IpAddr(ipArr[0], ipArr[1], ipArr[2], ipArr[3]);
  return true;
}

uint16_t Url::getPort()
{
  return m_port;
}

string Url::getPath()
{
  return m_path;
}

void Url::setProtocol(string protocol)
{
  m_protocol = protocol;
}

void Url::setHost(string host)
{
  m_host = host;
}

void Url::setPort(uint16_t port)
{
  m_port = port;
}

void Url::setPath(string path)
{
  m_path = path;
}
  
void Url::fromString(string str)
{
  //URI form [protocol://]host[:port]/path
  int pos = 0;
  int len = str.find("://");
  if( len > 0)
  {
    m_protocol = str.substr(0, len);
    pos += len + 3;
  }
  else
  {
    m_protocol = "";
  }
  
  bool isPort = false;
  int cln_pos = str.find(":", pos);
  int slash_pos = str.find("/", pos);
  if( slash_pos == -1 )
  {
    slash_pos = str.length();
  }
  if( (cln_pos != -1) && (cln_pos < slash_pos) )
  {
    isPort = true;
    len = cln_pos - pos;
  }
  else
  {
    len = slash_pos - pos;
  }
  
  m_host = str.substr(pos, len);
  
  pos += len;
  if( isPort )
  {
    pos+=1; //Do not keep :
    unsigned int port = 0;
    sscanf(str.substr(pos, cln_pos-pos).c_str(),"%u", &port);
    m_port = (uint16_t)port;
    pos = slash_pos;
  }
  
  m_path = str.substr(pos);  
}

string Url::toString()
{
  string url;
  if( !m_protocol.empty() )
  {
    url.append(m_protocol);
    url.append("://");
  }
  url.append(m_host);
  if( m_port )
  {
    char c_port[6] = {0};
    sprintf(c_port, "%d", m_port);
    url.append(":");
    url.append(c_port);
  }
  if(m_path[0]!='/')
  {
    url.append("/");
  }
  url.append(m_path);
  return url;
}
