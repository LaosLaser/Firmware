
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

#include "lwipNetTcpSocket.h"
#include "lwip/tcp.h"

//#define __DEBUG
#include "dbg/dbg.h"

#include "netCfg.h"
#if NET_LWIP_STACK

LwipNetTcpSocket::LwipNetTcpSocket(tcp_pcb* pPcb /*= NULL*/) : NetTcpSocket(), m_pPcb(pPcb), m_lpInNetTcpSocket(), //Passes a pcb if already created (by an accept req for instance), in that case transfers ownership
m_pReadPbuf(NULL)
{
  DBG("New NetTcpSocket %p\n", (void*)this);
  if(!m_pPcb)
  {
    m_pPcb = tcp_new();
    DBG("Creating new PCB %p\n", m_pPcb);
  }
  if(m_pPcb)
  {
    //Setup callbacks
    tcp_arg( (tcp_pcb*) m_pPcb, (void*) this ); //this will be passed to each static callback
    
    tcp_recv( (tcp_pcb*) m_pPcb, LwipNetTcpSocket::sRecvCb );
    tcp_sent((tcp_pcb*) m_pPcb, LwipNetTcpSocket::sSentCb );
    tcp_err( (tcp_pcb*) m_pPcb, LwipNetTcpSocket::sErrCb );
    //Connected callback is defined in connect()
    //Accept callback is defined in listen()
    DBG("NetTcpSocket created.\n");
  }
}

LwipNetTcpSocket::~LwipNetTcpSocket()
{
/*  if(m_pPcb)
    tcp_close( (tcp_pcb*) m_pPcb); //Disconnect & free pcb*/
  close();
}
  
NetTcpSocketErr LwipNetTcpSocket::bind(const Host& me)
{
  if(!m_pPcb)
    return NETTCPSOCKET_MEM; //NetTcpSocket was not properly initialised, should destroy it & retry
    
  err_t err = tcp_bind( (tcp_pcb*) m_pPcb, IP_ADDR_ANY, me.getPort()); //IP_ADDR_ANY : Bind the connection to all local addresses
  if(err)
    return NETTCPSOCKET_INUSE;
    
  return NETTCPSOCKET_OK;
}

NetTcpSocketErr LwipNetTcpSocket::listen()
{
  if(!m_pPcb)
    return NETTCPSOCKET_MEM; //NetTcpSocket was not properly initialised, should destroy it & retry
/*
  From doc/rawapi.txt :
  
  The tcp_listen() function returns a new connection identifier, and
  the one passed as an argument to the function will be
  deallocated. The reason for this behavior is that less memory is
  needed for a connection that is listening, so tcp_listen() will
  reclaim the memory needed for the original connection and allocate a
  new smaller memory block for the listening connection.
*/

//  tcp_pcb* pNewPcb = tcp_listen(m_pPcb);
  tcp_pcb* pNewPcb = tcp_listen_with_backlog((tcp_pcb*)m_pPcb, 5);
  if( !pNewPcb ) //Not enough memory to create the listening pcb
    return NETTCPSOCKET_MEM;

  m_pPcb = pNewPcb;
  
  tcp_accept( (tcp_pcb*) m_pPcb, LwipNetTcpSocket::sAcceptCb );

  return NETTCPSOCKET_OK;
}

NetTcpSocketErr LwipNetTcpSocket::connect(const Host& host)
{
  if(!m_pPcb)
    return NETTCPSOCKET_MEM; //NetTcpSocket was not properly initialised, should destroy it & retry
  
  ip_addr_t ip = host.getIp().getStruct();
  err_t err = tcp_connect( (tcp_pcb*) m_pPcb, &ip, host.getPort(), LwipNetTcpSocket::sConnectedCb );
  
  if(err)
    return NETTCPSOCKET_MEM;
    
  return NETTCPSOCKET_OK;
}

NetTcpSocketErr LwipNetTcpSocket::accept(Host* pClient, NetTcpSocket** ppNewNetTcpSocket)
{
  if( !m_pPcb ) //Pcb doesn't exist (anymore)
    return NETTCPSOCKET_MEM;
  //Dequeue a connection
  //if( m_lpInPcb.empty() )
  if( m_lpInNetTcpSocket.empty() )
    return NETTCPSOCKET_EMPTY;
  
  tcp_accepted( ((tcp_pcb*) m_pPcb) ); //Should fire proper events //WARN: m_pPcb is the GOOD param here (and not pInPcb)
  
/*  tcp_pcb* pInPcb = m_lpInPcb.front();
  m_lpInPcb.pop();*/
  
  if( (m_lpInNetTcpSocket.front()) == NULL )
  {
    m_lpInNetTcpSocket.pop();
    return NETTCPSOCKET_RST;
  }
  
  if( (m_lpInNetTcpSocket.front())->m_closed )
  {
    Net::releaseTcpSocket(m_lpInNetTcpSocket.front());
    m_lpInNetTcpSocket.pop();
    return NETTCPSOCKET_RST;
  }
  
  ip_addr_t* ip = (ip_addr_t*) &( (m_lpInNetTcpSocket.front()->m_pPcb)->remote_ip);
  
  *ppNewNetTcpSocket = m_lpInNetTcpSocket.front();
  *pClient = Host(
    IpAddr( 
      ip
    ), 
    m_lpInNetTcpSocket.front()->m_pPcb->remote_port 
  );
  m_lpInNetTcpSocket.pop();
//  *pClient = Host( IpAddr(pInPcb->remote_ip), pInPcb->remote_port );
  
  //Return a new socket
 // *ppNewNetTcpSocket = (NetTcpSocket*) new LwipNetTcpSocket(pInPcb);

  //tcp_accepted( ((tcp_pcb*) m_pPcb) ); //Should fire proper events //WARN: m_pPcb is the GOOD param here (and not pInPcb)
  
/*  if(*ppNewNetTcpSocket == NULL)
  {
    DBG("Not enough mem, socket dropped in LwipNetTcpSocket::accept.\n");
    tcp_abort(pInPcb);
  }*/
  
  return NETTCPSOCKET_OK;
}

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

int /*if < 0 : NetTcpSocketErr*/ LwipNetTcpSocket::send(const char* buf, int len)
{
  if( !m_pPcb ) //Pcb doesn't exist (anymore)
    return NETTCPSOCKET_MEM;
  int outLen = MIN( len, tcp_sndbuf( (tcp_pcb*) m_pPcb) ); 
  //tcp_sndbuf() returns the number of bytes available in the output queue, so never go above it
  err_t err = tcp_write( (tcp_pcb*) m_pPcb, (void*) buf, outLen, TCP_WRITE_FLAG_COPY );
  //Flags are TCP_WRITE_FLAG_COPY & TCP_WRITE_FLAG_MORE (see tcp_out.c) :
  //If TCP_WRITE_FLAG_MORE is not set ask client to push buffered data to app
  if(err)
  {
    switch( err )
    {
    case ERR_CONN:
      return (int) NETTCPSOCKET_SETUP; //Not connected properly
    case ERR_ARG:
      return (int) NETTCPSOCKET_SETUP; //Wrong args ! (like buf pointing to NULL)
    case ERR_MEM:
    default:
      return (int) NETTCPSOCKET_MEM; //Not enough memory
    }
  }
  return outLen;
}

int /*if < 0 : NetTcpSocketErr*/ LwipNetTcpSocket::recv(char* buf, int len)
{
  if( !m_pPcb ) //Pcb doesn't exist (anymore)
    return NETTCPSOCKET_MEM;
  int inLen = 0;
  int cpyLen = 0;
  
  static int rmgLen = 0; 
  //Contains the remaining len in this pbuf
  
  if( !m_pReadPbuf )
  {
    rmgLen = 0;
    return 0;
  }
  
  if ( !rmgLen ) //We did not know m_pReadPbuf->len last time we called this fn
  {
    rmgLen = m_pReadPbuf->len;
  }
  
  while ( inLen < len )
  {
    cpyLen = MIN( (len - inLen), rmgLen ); //Remaining len to copy, remaining len in THIS pbuf
    memcpy((void*)buf, (void*)((char*)(m_pReadPbuf->payload) + (m_pReadPbuf->len - rmgLen)), cpyLen);
    inLen += cpyLen;
    buf += cpyLen;
    
    rmgLen = rmgLen - cpyLen; //Update rmgLen
    
    if( rmgLen > 0 )
    {
      //We did not read this pbuf completely, so let's save it's pos & return
      break;
    }
    
    if(m_pReadPbuf->next)
    {
      pbuf* pNextPBuf = m_pReadPbuf->next;
      m_pReadPbuf->next = NULL; //So that it is not freed as well
      //We get the reference to pNextPBuf from m_pReadPbuf
      pbuf_free((pbuf*)m_pReadPbuf);
      m_pReadPbuf = pNextPBuf;
      rmgLen = m_pReadPbuf->len;
    }
    else
    {
      pbuf_free((pbuf*)m_pReadPbuf);
      m_pReadPbuf = NULL;
      rmgLen = 0;
      break; //No more data to read
    }
    
  }
  
  //tcp_recved(m_pPcb, inLen); //Acknowledge the reception
  
  return inLen;
}

NetTcpSocketErr LwipNetTcpSocket::close()
{
  //DBG("LwipNetTcpSocket::close() : Closing...\n");

  if(m_closed)
    return NETTCPSOCKET_OK; //Already being closed
  m_closed = true;
  
  if( !m_pPcb ) //Pcb doesn't exist (anymore)
    return NETTCPSOCKET_MEM;
    
  //Cleanup incoming data
  cleanUp();
 
  if( !!tcp_close( (tcp_pcb*) m_pPcb) )
  {
    DBG("LwipNetTcpSocket::close() could not close properly, abort.\n");
    tcp_abort( (tcp_pcb*) m_pPcb);
    m_pPcb = NULL;
    return NETTCPSOCKET_MEM;
  }
  
  DBG("LwipNetTcpSocket::close() : connection closed successfully.\n");
  
  m_pPcb = NULL;
  return NETTCPSOCKET_OK;
}

NetTcpSocketErr LwipNetTcpSocket::poll()
{
  NetTcpSocket::flushEvents();
  return NETTCPSOCKET_OK;
}

// Callbacks events

err_t LwipNetTcpSocket::acceptCb(struct tcp_pcb *newpcb, err_t err)
{
  if(err)
  {
    DBG("Error %d in LwipNetTcpSocket::acceptCb.\n", err);
    return err;
  }
  //FIXME: MEM Errs
  //m_lpInPcb.push(newpcb); //Add connection to the queue
  LwipNetTcpSocket* pNewNetTcpSocket = new LwipNetTcpSocket(newpcb);
  
  if(pNewNetTcpSocket == NULL)
  {
    DBG("Not enough mem, socket dropped in LwipNetTcpSocket::acceptCb.\n");
    tcp_abort(newpcb);
    return ERR_ABRT;
  }
  
  pNewNetTcpSocket->m_refs++;
  m_lpInNetTcpSocket.push( pNewNetTcpSocket );
  
 // tcp_accepted(newpcb);
 // tcp_accepted( m_pPcb ); //Should fire proper events //WARN: m_pPcb is the GOOD param here (and not pInPcb)
  queueEvent(NETTCPSOCKET_ACCEPT);
  return ERR_OK;
}

err_t LwipNetTcpSocket::connectedCb(struct tcp_pcb *tpcb, err_t err)
{
  queueEvent(NETTCPSOCKET_CONNECTED);
  return ERR_OK;
}

void LwipNetTcpSocket::errCb(err_t err)
{
  DBG("NetTcpSocket %p - Error %d in LwipNetTcpSocket::errCb.\n", (void*)this, err);
  //WARN: At this point, m_pPcb has been freed by lwIP
  m_pPcb = NULL;
  //These errors are fatal, discard all events queued before so that the errors are handled first
  discardEvents();
  m_closed = true;
  cleanUp();
  if( err == ERR_ABRT)
    queueEvent(NETTCPSOCKET_CONABRT);
  else //if( err == ERR_RST)
    queueEvent(NETTCPSOCKET_CONRST);
}
  
err_t LwipNetTcpSocket::sentCb(tcp_pcb* tpcb, u16_t len)
{
//  DBG("%d bytes ACKed by host.\n", len);
  queueEvent(NETTCPSOCKET_WRITEABLE);
  return ERR_OK;
}

err_t LwipNetTcpSocket::recvCb(tcp_pcb* tpcb, pbuf *p, err_t err)
{
  //Store pbuf ptr
 // DBG("Receive CB with err = %d & len = %d.\n", err, p->tot_len);
//  tcp_recved( (tcp_pcb*) m_pPcb, p->tot_len); //Acknowledge the reception
  
  if(err)
  {
    queueEvent(NETTCPSOCKET_ERROR);
    return ERR_OK; //FIXME: More robust error handling there
  }
  else if(!p)
  {
    DBG("NetTcpSocket %p - Connection closed by remote host (LwipNetTcpSocket::recvCb).\n", (void*)this);
    //Buf is NULL, that means that the connection has been closed by remote host
    
    //FIX: 27/05/2010: We do not want to deallocate the socket while some data might still be readable
    //REMOVED:   close();
 
    //However we do not want to close the socket yet
 
    queueEvent(NETTCPSOCKET_DISCONNECTED);
    return ERR_OK; 
  }
  
  //We asserted that p is a valid pointer

  //New data processing
  tcp_recved( tpcb, p->tot_len); //Acknowledge the reception
  if(!m_pReadPbuf)
  {
    m_pReadPbuf = p;
    queueEvent(NETTCPSOCKET_READABLE);
  }
  else
  {
    pbuf_cat((pbuf*)m_pReadPbuf, p); //m_pReadPbuf is not empty, tail p to it and drop our ref
    //No need to queue an event in that case since the read buf has not been processed yet
  }
  return ERR_OK;
}


void LwipNetTcpSocket::cleanUp() //Flush input buffer
{
  //Ensure that further error won't be followed to this inst (which can be destroyed)
  if( m_pPcb )
  {
    tcp_arg( (tcp_pcb*) m_pPcb, (void*) NULL );
    tcp_recv( (tcp_pcb*) m_pPcb, NULL );
    tcp_sent((tcp_pcb*) m_pPcb, NULL );
    tcp_err( (tcp_pcb*) m_pPcb, NULL );
  }
  
  if( m_pReadPbuf )
  {
    DBG("Deallocating unread data.\n");
    pbuf_free((pbuf*)m_pReadPbuf); //Free all unread data
    m_pReadPbuf = NULL;
    recv(NULL,0); //Update recv ptr position
  }
}

// Static callbacks from LwIp

err_t LwipNetTcpSocket::sAcceptCb(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  LwipNetTcpSocket* pMe = (LwipNetTcpSocket*) arg;
  return pMe->acceptCb( newpcb, err );
}

err_t LwipNetTcpSocket::sConnectedCb(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  LwipNetTcpSocket* pMe = (LwipNetTcpSocket*) arg;
  return pMe->connectedCb( tpcb, err );
}

void LwipNetTcpSocket::sErrCb(void *arg, err_t err)
{
  if( !arg )
  {
    DBG("NetTcpSocket - Error %d in LwipNetTcpSocket::sErrCb.\n", err);
    return; //The socket has been destroyed, discard error
  }
  LwipNetTcpSocket* pMe = (LwipNetTcpSocket*) arg;
  return pMe->errCb( err );
}
  
err_t LwipNetTcpSocket::sSentCb(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  LwipNetTcpSocket* pMe = (LwipNetTcpSocket*) arg;
  return pMe->sentCb( tpcb, len );
}

err_t LwipNetTcpSocket::sRecvCb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  if( tpcb->flags & TF_RXCLOSED )
  {
    //The Pcb is in a closing state
    //Discard that data here since we might have destroyed the corresponding socket object
    tcp_recved( tpcb, p->tot_len);
    pbuf_free( p );    
    return ERR_OK;
  }
  LwipNetTcpSocket* pMe = (LwipNetTcpSocket*) arg;
  return pMe->recvCb( tpcb, p, err );
}

#endif
