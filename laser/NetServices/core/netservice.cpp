
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

#include "netservice.h"
#include "net.h"

//#define __DEBUG
#include "dbg/dbg.h"

NetService::NetService(bool owned /*= true*/) : m_closed(false), m_removed(false), m_owned(owned)
{
  NetService::lpServices().push_back(this);
  DBG("\r\n[ + %p ] %d\r\n", (void*)this, lpServices().size());
}

NetService::~NetService()
{
 // DBG("\r\nService removed\r\n");
//  DBG("\r\nNow %d services running\r\n", lpServices().size()-1);
  DBG("\r\n[ - %p ] %d\r\n", (void*)this, lpServices().size()-1);
  if((!m_owned) || (!m_removed)) //Destructor was not called by servicesPoll()
  { 
    if(m_owned)
      DBG("\r\nWARN!!!Service removed in dtor!!!\r\n");
    NetService::lpServices().remove(this);
  }
}

void NetService::poll()
{

}
  
void NetService::servicesPoll() //Poll all registered services & destroy closed ones
{
  list<NetService*>::iterator it;
  DBG("\r\nServices polling over %d services\r\n", lpServices().size());
  for( it = lpServices().begin(); it != lpServices().end();  )
  {
    if( (*it)->m_owned && (*it)->m_closed  )
    {
      DBG("\r\nService %p is flagged as closed\r\n", (*it));
      (*it)->m_removed = true;
      delete (*it);
      it = lpServices().erase(it);
    }
    else
    {
      //DBG("Service %p polling start\n", (*it));
      (*it)->poll();
      //DBG("Service %p polling end\n", (*it));
      it++;
    }
  }
  
}

void NetService::close()
{
  DBG("\r\nService %p to be closed (owned = %d)\r\n", this, m_owned);
  m_closed = true;
}

list<NetService*>& NetService::lpServices()
{
  static list<NetService*>* pInst = new list<NetService*>(); //Called only once
  return *pInst;
}
