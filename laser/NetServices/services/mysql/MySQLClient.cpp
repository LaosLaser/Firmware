#pragma diag_remark 177
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

#include "MySQLClient.h"
#include "sha1.h" //For 4.1+ passwords
#include "mycrypt.h" //For 4.0- passwords

//#define __DEBUG
#include "dbg/dbg.h"

#define MYSQL_TIMEOUT_MS 45000
#define MYSQL_PORT 3306

#define BUF_SIZE 256

#define CLIENT_LONG_PASSWORD     1
#define CLIENT_CONNECT_WITH_DB   8
#define CLIENT_PROTOCOL_41       512
#define CLIENT_INTERACTIVE         1024
#define CLIENT_SECURE_CONNECTION 32768

#define MIN(a,b) ((a)<(b)?(a):(b))
#define ABS(a) (((a)>0)?(a):0)

//MySQL commands
#define COM_QUIT  0x01 //Exit
#define COM_QUERY 0x03 //Execute an SQL query

//#define htons( x ) ( (( x << 8 ) & 0xFF00) | (( x >> 8 ) & 0x00FF) )
#define ntohs( x ) (htons(x))

/*#define htonl( x ) ( (( x << 24 ) & 0xFF000000)  \
                   | (( x <<  8 ) & 0x00FF0000)  \
                   | (( x >>  8 ) & 0x0000FF00)  \
                   | (( x >> 24 ) & 0x000000FF)  )*/
#define htonl( x ) (x)
#define ntohl( x ) (htonl(x))

MySQLClient::MySQLClient() : NetService(false) /*Not owned by the pool*/, m_pCbItem(NULL), m_pCbMeth(NULL), m_pCb(NULL),
m_pTCPSocket(NULL), m_watchdog(), m_timeout(MYSQL_TIMEOUT_MS*1000), m_pDnsReq(NULL), m_closed(true), 
m_host(), m_user(), m_password(), m_db(), m_state(MYSQL_CLOSED)
{
  m_buf = new byte[BUF_SIZE];
  m_pPos = m_buf;
  m_len = 0;
  m_size = BUF_SIZE;
}

MySQLClient::~MySQLClient()
{
  close();
  delete[] m_buf;
}

//High Level setup functions
MySQLResult MySQLClient::open(Host& host, const string& user, const string& password, const string& db, void (*pMethod)(MySQLResult)) //Non blocking
{
  setOnResult(pMethod);
  setup(host, user, password, db);
  return MYSQL_PROCESSING;
}

#if 0 //Ref only
template<class T> 
MySQLResult MySQLClient::open(Host& host, const string& user, const string& password, const string& db, T* pItem, void (T::*pMethod)(MySQLResult)) //Non blocking
{
  setOnResult(pItem, pMethod);
  setup(host, user, password, db);
  return MYSQL_PROCESSING;
}
#endif

MySQLResult MySQLClient::sql(string& sqlCommand)
{
  if(m_state!=MYSQL_COMMANDS)
    return MYSQL_SETUP;
  sendCommand(COM_QUERY, (byte*)sqlCommand.data(), sqlCommand.length());
  return MYSQL_PROCESSING;
}

MySQLResult MySQLClient::exit()
{
  sendCommand(COM_QUIT, NULL, 0);
  close();
  return MYSQL_OK;
}
  
void MySQLClient::setOnResult( void (*pMethod)(MySQLResult) )
{
  m_pCb = pMethod;
  m_pCbItem = NULL;
  m_pCbMeth = NULL;
}

#if 0 //Ref only
template<class T> 
void MySQLClient::setOnResult( T* pItem, void (T::*pMethod)(MySQLResult) )
{
  m_pCb = NULL;
  m_pCbItem = (CDummy*) pItem;
  m_pCbMeth = (void (CDummy::*)(MySQLResult)) pMethod;
}
#endif

void MySQLClient::setTimeout(int ms)
{
  m_timeout = 1000*ms;
}

void MySQLClient::poll() //Called by NetServices
{
  if(m_closed)
  {
    return;
  }
  if(m_watchdog.read_us()>m_timeout)
  {
    onTimeout();
  }
}

void MySQLClient::resetTimeout()
{
  m_watchdog.reset();
  m_watchdog.start();
}

void MySQLClient::init()
{
  close(); //Remove previous elements
  if(!m_closed) //Already opened
    return;
  m_state = MYSQL_HANDSHAKE;
  m_pTCPSocket = new TCPSocket;
  m_pTCPSocket->setOnEvent(this, &MySQLClient::onTCPSocketEvent);
  m_closed = false;
}

void MySQLClient::close()
{
  if(m_closed)
    return;
  m_state = MYSQL_CLOSED;
  m_closed = true; //Prevent recursive calling or calling on an object being destructed by someone else
  m_watchdog.stop(); //Stop timeout
  m_watchdog.reset();
  m_pTCPSocket->resetOnEvent();
  m_pTCPSocket->close();
  delete m_pTCPSocket;
  m_pTCPSocket = NULL;
  if( m_pDnsReq )
  {
    m_pDnsReq->close();
    delete m_pDnsReq;
    m_pDnsReq = NULL;
  }
}

void MySQLClient::setup(Host& host, const string& user, const string& password, const string& db) //Setup connection, make DNS Req if necessary
{
  init(); //Initialize client in known state, create socket
  resetTimeout();
  m_host = host;
  if(!host.getPort())
    host.setPort( MYSQL_PORT ); //Default port
    
  m_user = user;
  m_password = password;
  
  m_db = db;

  if( !host.getIp().isNull() )
  {
    connect();
  }
  else //Need to do a DNS Query...
  {
    DBG("DNS Query...\n");
    m_pDnsReq = new DNSRequest();
    m_pDnsReq->setOnReply(this, &MySQLClient::onDNSReply);
    m_pDnsReq->resolve(&m_host);
    DBG("MySQLClient : DNSRequest %p\n", m_pDnsReq);
  }
}

void MySQLClient::connect() //Start Connection
{
  resetTimeout();
  DBG("Connecting...\n");
  m_pTCPSocket->connect(m_host);
  m_packetId = 0;
}

void MySQLClient::handleHandshake()
{
  readData();
  if( ! (( m_len > 1 ) && ( memchr( m_buf + 1, 0, m_len ) != NULL )) )
  {
    DBG("Connected but could not find pcsz...\n");
    onResult(MYSQL_PRTCL);
    return;
  }
  
  DBG("Connected to server: %d bytes read ; Protocol version %d, mysql-%s.\n", m_len, m_buf[0], &m_buf[1]);
  
  m_pPos = (byte*) memchr( (char*)(m_buf + 1), 0, m_len ) + 1;
  
  sendAuth();
}

void MySQLClient::sendAuth()
{
  if( m_len - (m_pPos - m_buf) != 44)
  {
    //We only support protocol >= mysql-4.1
    DBG("Message after pcsz has wrong len (%d != 44)...\n", m_len - (m_pPos - m_buf));
    onResult(MYSQL_PRTCL);
    return;
  }
   
  uint16_t serverFlags = *((uint16_t*)&m_pPos[13]);
  DBG("Server capabilities are %04X.\n", serverFlags);
  
  uint32_t clientFlags = CLIENT_CONNECT_WITH_DB | CLIENT_PROTOCOL_41 | CLIENT_SECURE_CONNECTION | CLIENT_INTERACTIVE;;
  
  //if(serverFlags & CLIENT_LONG_PASSWORD)
  
  DBG("Using auth 4.1+\n");
  //Encrypt pw using scramble
  byte scramble[20+20]={0};
  memcpy(scramble, m_pPos+4, 8); 
  memcpy(scramble+8, m_pPos+31, 12); // *(m_pPos+43) == 0 (zero-terminated char*)

  byte stage1_hash[20] = {0};
  sha1( (byte*)m_password.data(), m_password.length(), stage1_hash );

  sha1( stage1_hash, 20, ((byte*)scramble + 20) );

  byte token[20] = {0};
  sha1( scramble, 40, token );

  for(int i=0;i<20;i++)
    token[i] = token[i] ^ stage1_hash[i];
    
  clientFlags |= CLIENT_LONG_PASSWORD;
  
  DBG("Building response\n"); 
  //Build response

  //BE
  *((uint32_t*)&m_buf[0]) = htonl(clientFlags);
  *((uint32_t*)&m_buf[4]) = BUF_SIZE; //Max packets size
  m_buf[8] = 8; //latin1 charset
  memset((char*)(m_buf+9),0,23);
  strcpy((char*)(m_buf+32),m_user.c_str());
  m_pPos = m_buf + 32 + m_user.length() + 1;
  m_pPos[0] = 20;
  memcpy((char*)&m_pPos[1],token,20);
  strcpy((char*)(m_pPos+21),m_db.c_str());
  m_len = 32 + m_user.length() + 1 + 21 + m_db.length() + 1;
  
  //Save first part of scramble in case we need it again
  memcpy(&m_buf[BUF_SIZE-8], scramble, 8);

  m_state = MYSQL_AUTH;
  
  DBG("Writing data\n"); 
  writeData();
}

void MySQLClient::handleAuthResult()
{
  readData();
  if(m_len==1 && *m_buf==0xfe)
  {
    //Re-send auth using 4.0- auth
    sendAuth323();
    return;
  }
  m_watchdog.stop(); //Stop timeout
  m_watchdog.reset();
  if(m_len<2)
  {
    DBG("Response too short..\n");
    onResult(MYSQL_PRTCL);
    return;
  }
  DBG("RC=%d ",m_buf[0]);
  if(m_buf[0]==0)
  {
    m_buf[m_len] = 0;
    m_pPos = m_buf + 1;
    m_pPos += m_buf[1] +1;
    m_pPos += m_pPos[0];
    m_pPos += 1;
    DBG("(OK) : Server status %d, Message : %s\n", *((uint16_t*)&m_pPos[0]), m_pPos+4);
    onResult(MYSQL_OK);
  }
  else
  {
    m_buf[m_len] = 0;
    DBG("(Error %d) : %s\n", *((uint16_t*)&m_buf[1]), &m_buf[9]); //LE
    onResult(MYSQL_AUTHFAILED);
    return;
  }
  m_state = MYSQL_COMMANDS;
}

void MySQLClient::sendAuth323()
{
  DBG("Using auth 4.0-\n");
  byte scramble[8]={0};
  
  memcpy(scramble, &m_buf[BUF_SIZE-8], 8); //Recover scramble
  
  //memcpy(scramble+8, m_pPos+31, 12); // *(m_pPos+43) == 0 (zero-terminated char*)
  
  byte token[9]={0};
  
  scramble_323((char*)token, (const char*)scramble, m_password.c_str());
  
  DBG("Building response\n"); 
  //Build response
  
  memcpy((char*)m_buf,token,9);
  m_len = 9;

  #if 0
  *((uint32_t*)&m_buf[0]) = htonl(clientFlags);
  *((uint32_t*)&m_buf[4]) = BUF_SIZE; //Max packets size
  m_buf[8] = 8; //latin1 charset
  memset((char*)(m_buf+9),0,23);
  strcpy((char*)(m_buf+32),m_user.c_str());
  m_pPos = m_buf + 32 + m_user.length() + 1;
  m_pPos[0] = 8;
  memcpy((char*)&m_pPos[1],token+1,8);
  strcpy((char*)(m_pPos+9),m_db.c_str());
  m_len = 32 + m_user.length() + 1 + 9 + m_db.length() + 1;
  #endif
  
  DBG("Writing data\n"); 
  writeData();
}

void MySQLClient::sendCommand(byte command, byte* arg, int len)
{
  DBG("Sending command %d, payload of len %d\n", command, len);
  m_packetId=0;//Reset packet ID (New sequence)
  m_buf[0] = command;
  memcpy(&m_buf[1], arg, len);
  m_len = 1 + len;
  writeData();
  m_watchdog.start();
}

void MySQLClient::handleCommandResult()
{
  readData();
  m_watchdog.stop(); //Stop timeout
  m_watchdog.reset();
  if(m_len<2)
  {
    DBG("Response too short..\n");
    onResult(MYSQL_PRTCL);
    return;
  }
  DBG("RC=%d ",m_buf[0]);
  if(m_buf[0]==0)
  {
    DBG("(OK)\n");
    onResult(MYSQL_OK);
  }
  else
  {
    m_buf[m_len] = 0;
    DBG("(SQL Error %d) : %s\n", *((uint16_t*)&m_buf[1]), &m_buf[9]); //LE
    onResult(MYSQL_SQL);
    return;
  }
}

void MySQLClient::readData() //Copy to buf
{
  byte head[4];
  int ret = m_pTCPSocket->recv((char*)head, 4); //Packet header
  m_len = *((uint16_t*)&head[0]);
  m_packetId = head[3];
  DBG("Packet Id %d of length %d\n", head[3], m_len);
  m_packetId++;
  if(ret>0)
    ret = m_pTCPSocket->recv((char*)m_buf, m_len);
  if(ret < 0)//Error
  {
    onResult(MYSQL_CONN);
    return;
  }
  if(ret < m_len)
  {
    DBG("WARN: Incomplete packet\n");
  }
  m_len = ret;
}

void MySQLClient::writeData() //Copy from buf
{
  byte head[4] = { 0 };
  *((uint16_t*)&head[0]) = m_len;
  head[3] = m_packetId;
  DBG("Packet Id %d\n", head[3]);
  m_packetId++;
  int ret = m_pTCPSocket->send((char*)head, 4); //Packet header
  if(ret>0)
    ret = m_pTCPSocket->send((char*)m_buf, m_len);
  if(ret < 0)//Error
  {
    onResult(MYSQL_CONN);
    return;
  }
  m_len = 0;//FIXME... incomplete packets handling
}

void MySQLClient::onTCPSocketEvent(TCPSocketEvent e)
{
  DBG("Event %d in MySQLClient::onTCPSocketEvent()\n", e);

  if(m_closed)
  {
    DBG("WARN: Discarded\n");
    return;
  }
  
  switch(e)
  {
  case TCPSOCKET_READABLE: //Incoming data
    resetTimeout();
    if(m_state == MYSQL_HANDSHAKE)
      handleHandshake();
    else if(m_state == MYSQL_AUTH)
      handleAuthResult();
    else if(m_state == MYSQL_COMMANDS)
      handleCommandResult();
    break;
  case TCPSOCKET_WRITEABLE: //We can send data
    resetTimeout();
    break;
  case TCPSOCKET_CONNECTED: //Connected, wait for handshake packet
    resetTimeout();
    break;
  case TCPSOCKET_CONTIMEOUT:
  case TCPSOCKET_CONRST:
  case TCPSOCKET_CONABRT:
  case TCPSOCKET_ERROR:
    DBG("Connection error.\n");
    onResult(MYSQL_CONN);
  case TCPSOCKET_DISCONNECTED:
    //There might still be some data available for reading
    //So if we are in a reading state, do not close the socket yet
    if(m_state != MYSQL_CLOSED)
    {
      onResult(MYSQL_CONN);
    }
    DBG("Connection closed by remote host.\n");
    break;
  }
}

void MySQLClient::onDNSReply(DNSReply r)
{
  if(m_closed)
  {
    DBG("WARN: Discarded\n");
    return;
  }
  
  if( r != DNS_FOUND )
  {
    DBG("Could not resolve hostname.\n");
    onResult(MYSQL_DNS);
    return;
  }
  
  DBG("DNS Resolved to %d.%d.%d.%d.\n",m_host.getIp()[0],m_host.getIp()[1],m_host.getIp()[2],m_host.getIp()[3]);
  //If no error, m_host has been updated by m_pDnsReq so we're set to go !
  m_pDnsReq->close();
  delete m_pDnsReq;
  m_pDnsReq = NULL;
  connect();
}

void MySQLClient::onResult(MySQLResult r) //Called when exchange completed or on failure
{
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)(r);
  else if(m_pCb)
    m_pCb(r);
    
  if( (r==MYSQL_DNS) || (r==MYSQL_PRTCL) || (r==MYSQL_AUTHFAILED) || (r==MYSQL_TIMEOUT) || (r==MYSQL_CONN) ) //Fatal error, close connection
    close();
}

void MySQLClient::onTimeout() //Connection has timed out
{
  DBG("Timed out.\n");
  onResult(MYSQL_TIMEOUT);
  close();
}
