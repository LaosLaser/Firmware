
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

#include "core/netservice.h"
#include "HTTPRequestDispatcher.h"
#include "HTTPRequestHandler.h"
#include <string.h>

//#define __DEBUG
#include "dbg/dbg.h"

HTTPRequestDispatcher::HTTPRequestDispatcher(HTTPServer* pSvr, TCPSocket* pTCPSocket) : NetService(), m_pSvr(pSvr), m_pTCPSocket(pTCPSocket), m_watchdog(), m_closed(false)
{
  m_pTCPSocket->setOnEvent(this, &HTTPRequestDispatcher::onTCPSocketEvent);
  m_watchdog.attach_us<HTTPRequestDispatcher>(this, &HTTPRequestDispatcher::onTimeout, HTTP_REQUEST_TIMEOUT * 1000);
}

HTTPRequestDispatcher::~HTTPRequestDispatcher()
{
  close();
}

void HTTPRequestDispatcher::dispatchRequest()
{
  string path;
  string meth;
  HTTP_METH methCode;
  
  DBG("Dispatching req\r\n");
  
  if( !getRequest(&path, &meth ) )
  {
    close();
    return; //Invalid request
  }
  
  if( !meth.compare("GET") )
  {
    methCode = HTTP_GET;
  }
  else if( !meth.compare("POST") )
  {
    methCode = HTTP_POST;
  }
  else if( !meth.compare("HEAD") )
  {
    methCode = HTTP_HEAD;
  }
  else
  {
    close(); //Parse error
    return;
  }
  
  DBG("Looking for a handler\r\n");
  
  map< string, HTTPRequestHandler*(*)(const char*, const char*, TCPSocket*) >::iterator it;
//  it = m_pSvr->m_lpHandlers.find(rootPath); //We are friends so we can do that
// NEW CODE START: 
  int root_len = 0;
  for (it = m_pSvr->m_lpHandlers.begin(); it != m_pSvr->m_lpHandlers.end(); it++)
  {
    DBG("Checking %s...\n", (*it).first.c_str());
    root_len = (*it).first.length();
    if ( root_len &&
      !path.compare( 0, root_len, (*it).first ) && 
      (path[root_len] == '/' || path[root_len] == '\0'))
    {
      DBG("Found (%s)\n", (*it).first.c_str());
	    // Found!
	    break;	// for
	  }
  }
// NEW CODE END
  if((it == m_pSvr->m_lpHandlers.end()) && !(m_pSvr->m_lpHandlers.empty()))
  {
    DBG("Using default handler\n");
    it = m_pSvr->m_lpHandlers.end();
    it--; //Get the last element
    if( ! (((*it).first.length() == 0) || !(*it).first.compare("/")) ) //This is not the default handler
      it = m_pSvr->m_lpHandlers.end();
    root_len = 0;
  }
  if(it == m_pSvr->m_lpHandlers.end())
  {    
    DBG("No handler found\n");
    close(); //No handler found
    return;
  }
  
  DBG("Handler found.\r\n");
  
//HTTPRequestHandler* pHdlr = (*it).second(rootPath.c_str(), subPath.c_str(), m_pTCPSocket);
//NEW CODE 1 LINE:
  HTTPRequestHandler* pHdlr = (*it).second((*it).first.c_str(), path.c_str() + root_len, m_pTCPSocket);
  m_pTCPSocket = NULL; //We don't own it anymore
  
  switch(methCode)
  {
  case HTTP_GET:
    pHdlr->doGet();
    break;
  case HTTP_POST:
    pHdlr->doPost();
    break;
  case HTTP_HEAD:
    pHdlr->doHead();
    break;
  }
  
  DBG("Req handled (or being handled)\r\n");
  close();
}

void HTTPRequestDispatcher::close() //Close socket and destroy data
{
  if(m_closed)
    return;
  m_closed = true; //Prevent recursive calling or calling on an object being destructed by someone else
  m_watchdog.detach();
  if(m_pTCPSocket) //m_pTCPSocket Should only be destroyed if ownership not passed to an handler
  {
    m_pTCPSocket->resetOnEvent();
    m_pTCPSocket->close();
    delete m_pTCPSocket; //This fn might have been called by this socket (through an event), so DO NOT DESTROY IT HERE
  }
  NetService::close();
}


void HTTPRequestDispatcher::onTimeout() //Connection has timed out
{
  close();
}

bool HTTPRequestDispatcher::getRequest(string* path, string* meth)
{
  char req[128];
  char c_path[128];
  char c_meth[128];
  const int maxLen = 128;
  char* p = req;
  //Read Line
  int ret;
  int len = 0;
  for(int i = 0; i < maxLen - 1; i++)
  {
    ret = m_pTCPSocket->recv(p, 1);
    if(!ret)
    {
      break;
    }
    if( (len > 1) && *(p-1)=='\r' && *p=='\n' )
    {
      p--;
      len-=2;
      break;
    }
    else if( *p=='\n' )
    {
      len--;
      break;    
    }
    p++;
    len++;
  }
  *p = 0;
  
  DBG("Parsing request : %s\r\n", req);
  
  ret = sscanf(req, "%s %s HTTP/%*d.%*d", c_meth, c_path);
  if(ret !=2)
    return false;
    
  *meth = string(c_meth);
// NEW CODE (old code removed):
   *path = string(c_path);
  return true;
}



void HTTPRequestDispatcher::onTCPSocketEvent(TCPSocketEvent e)
{

  DBG("\r\nEvent %d\r\n", e);
  
  if(m_closed)
  {
    DBG("\r\nWARN: Discarded\r\n");
    return;
  }

  switch(e)
  {
  case TCPSOCKET_READABLE:
    m_watchdog.detach();
    m_pTCPSocket->resetOnEvent();
    //Req arrived, dispatch :
    dispatchRequest();
    break;
  case TCPSOCKET_CONTIMEOUT:
  case TCPSOCKET_CONRST:
  case TCPSOCKET_CONABRT:
  case TCPSOCKET_ERROR:
  case TCPSOCKET_DISCONNECTED:
    close();
    break;
  }
  
}
