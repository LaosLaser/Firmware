
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
#include "HTTPClient.h"
#include "../util/base64.h"
#include "../util/url.h"

//#define __DEBUG
#include "dbg/dbg.h"

#define HTTP_REQUEST_TIMEOUT 30000//15000
#define HTTP_PORT 80

#define CHUNK_SIZE 256

HTTPClient::HTTPClient() : NetService(false) /*Not owned by the pool*/, m_meth(HTTP_GET), m_pCbItem(NULL), m_pCbMeth(NULL), m_pCb(NULL),
m_watchdog(), m_timeout(0), m_pDnsReq(NULL), m_server(), m_path(), 
m_closed(true), m_state(HTTP_CLOSED),
m_pDataOut(NULL), m_pDataIn(NULL), m_dataChunked(false), m_dataPos(0), m_dataLen(0), m_httpResponseCode(0), m_blockingResult(HTTP_PROCESSING)
 
{
  setTimeout(HTTP_REQUEST_TIMEOUT);
  m_buf = new char[CHUNK_SIZE];
  DBG("New HTTPClient %p\n",this);
}

HTTPClient::~HTTPClient()
{
  close();
  delete[] m_buf;
}
  
void HTTPClient::basicAuth(const char* user, const char* password) //Basic Authentification
{
  if(user==NULL)
  {
    m_reqHeaders.erase("Authorization"); //Remove auth str
    return;
  }
  string auth = "Basic ";
  string decStr = user;
  decStr += ":";
  decStr += password;
  auth.append( Base64::encode(decStr) );
  DBG("Auth str is %s\n", auth.c_str());
  m_reqHeaders["Authorization"] = auth; 
}

//High Level setup functions
HTTPResult HTTPClient::get(const char* uri, HTTPData* pDataIn) //Blocking
{
  doGet(uri, pDataIn);
  return blockingProcess();
}

HTTPResult HTTPClient::get(const char* uri, HTTPData* pDataIn, void (*pMethod)(HTTPResult)) //Non blocking
{
  setOnResult(pMethod);
  doGet(uri, pDataIn);
  return HTTP_PROCESSING;
}

#if 0 //For info only
template<class T> 
HTTPResult HTTPClient::get(const char* uri, HTTPData* pDataIn, T* pItem, void (T::*pMethod)(HTTPResult)) //Non blocking
{
  setOnResult(pItem, pMethod);
  doGet(uri, pDataIn);
  return HTTP_PROCESSING;
}
#endif

HTTPResult HTTPClient::post(const char* uri, const HTTPData& dataOut, HTTPData* pDataIn) //Blocking
{
  doPost(uri, dataOut, pDataIn);
  return blockingProcess();
}

HTTPResult HTTPClient::post(const char* uri, const HTTPData& dataOut, HTTPData* pDataIn, void (*pMethod)(HTTPResult)) //Non blocking
{
  setOnResult(pMethod);
  doPost(uri, dataOut, pDataIn);
  return HTTP_PROCESSING;
}

#if 0 //For info only
template<class T> 
HTTPResult HTTPClient::post(const char* uri, const HTTPData& dataOut, HTTPData* pDataIn, T* pItem, void (T::*pMethod)(HTTPResult)) //Non blocking 
{
  setOnResult(pItem, pMethod);
  doPost(uri, dataOut, pDataIn);
  return HTTP_PROCESSING;
}
#endif

void HTTPClient::doGet(const char* uri, HTTPData* pDataIn)
{
  m_meth = HTTP_GET;
  setup(uri, NULL, pDataIn);
}

void HTTPClient::doPost(const char* uri, const HTTPData& dataOut, HTTPData* pDataIn)
{
  m_meth = HTTP_POST;
  setup(uri, (HTTPData*) &dataOut, pDataIn);
}

void HTTPClient::setOnResult( void (*pMethod)(HTTPResult) )
{
  m_pCb = pMethod;
  m_pCbItem = NULL;
  m_pCbMeth = NULL;
}
  
#if 0 //For info only
template<class T> 
void HTTPClient::setOnResult( T* pItem, void (T::*pMethod)(NtpResult) )
{
  m_pCb = NULL;
  m_pCbItem = (CDummy*) pItem;
  m_pCbMeth = (void (CDummy::*)(NtpResult)) pMethod;
}
#endif
  
void HTTPClient::setTimeout(int ms)
{
  m_timeout = ms;
  //resetTimeout();
}

void HTTPClient::poll() //Called by NetServices
{
  if(m_closed)
  {
    return;
  }
  if(m_watchdog.read_ms()>m_timeout)
  {
    onTimeout();
  }
  else if(m_state == HTTP_READ_DATA_INCOMPLETE)
  {
    readData(); //Try to read more data
    if( m_state == HTTP_DONE ) 
    {
      //All data has been read, close w/ success :)
      DBG("Done :)!\n");
      onResult(HTTP_OK);
      close();
    }
  }
  
}

int HTTPClient::getHTTPResponseCode()
{
  return m_httpResponseCode;
}

void HTTPClient::setRequestHeader(const string& header, const string& value)
{
  m_reqHeaders[header] = value;
}

string& HTTPClient::getResponseHeader(const string& header)
{
  return m_respHeaders[header];
}

void HTTPClient::resetRequestHeaders()
{
  m_reqHeaders.clear();
}
  
void HTTPClient::resetTimeout()
{
  m_watchdog.reset();
  m_watchdog.start();
}

void HTTPClient::init() //Create and setup socket if needed
{
  close(); //Remove previous elements
  if(!m_closed) //Already opened
    return;
  m_state = HTTP_WRITE_HEADERS;
  m_pTCPSocket = new TCPSocket;
  m_pTCPSocket->setOnEvent(this, &HTTPClient::onTCPSocketEvent);
  m_closed = false;
  m_httpResponseCode = 0;
}
  
void HTTPClient::close()
{
  if(m_closed)
    return;
  m_state = HTTP_CLOSED;
  //Now Request headers are kept btw requests unless resetRequestHeaders() is called
  //m_reqHeaders.clear(); //Clear headers for next requests
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

void HTTPClient::setup(const char* uri, HTTPData* pDataOut, HTTPData* pDataIn) //Setup request, make DNS Req if necessary
{
  init(); //Initialize client in known state, create socket
  m_pDataOut = pDataOut;
  m_pDataIn = pDataIn;
  resetTimeout();
  
  //Erase previous headers
  //Do NOT clear m_reqHeaders as they might have already set before connecting
  m_respHeaders.clear();
  
  //Erase response buffer
  if(m_pDataIn)
    m_pDataIn->clear();
  
  //Assert that buffers are initialized properly
  m_dataLen = 0;
  m_bufRemainingLen = 0;
  
  Url url;
  url.fromString(uri);
  
  m_path = url.getPath();
  
  m_server.setName(url.getHost().c_str());
  
  if( url.getPort() > 0 )
  {
    m_server.setPort( url.getPort() );
  }
  else
  {
    m_server.setPort( HTTP_PORT );
  }
  
  DBG("URL parsed,\r\nHost: %s\r\nPort: %d\r\nPath: %s\n", url.getHost().c_str(), url.getPort(), url.getPath().c_str());
  
  IpAddr ip;
  if( url.getHostIp(&ip) )
  {
    m_server.setIp(ip);
    connect();
  }
  else
  {
    DBG("DNS Query...\n");
    m_pDnsReq = new DNSRequest();
    m_pDnsReq->setOnReply(this, &HTTPClient::onDNSReply);
    m_pDnsReq->resolve(&m_server);
    DBG("HTTPClient : DNSRequest %p\n", m_pDnsReq);
  }
  
}

void HTTPClient::connect() //Start Connection
{
  resetTimeout();
  DBG("Connecting...\n");
  m_pTCPSocket->connect(m_server);
}

#define MIN(a,b) ((a)<(b)?(a):(b))
#define ABS(a) (((a)>0)?(a):0)
int HTTPClient::tryRead() //Try to read data from tcp packet and put in the HTTPData object
{
  int len = 0;
  int readLen;
  do
  {
    if(m_state == HTTP_READ_DATA_INCOMPLETE) //First try to complete buffer copy
    {
      readLen = m_bufRemainingLen;
   /*   if (readLen == 0)
      {
        m_state = HTTP_READ_DATA;
        continue;
      }*/
    }
    else
    {
      readLen = m_pTCPSocket->recv(m_buf, MIN(ABS(m_dataLen-m_dataPos),CHUNK_SIZE));
      if(readLen < 0) //Error
      {
        return readLen;
      }
      
      DBG("%d bytes read\n", readLen);
      
      m_pBufRemaining = m_buf;
    }
    if (readLen == 0)
    { 
      m_state = HTTP_READ_DATA;
      return len;
    }
    
    DBG("Trying to write %d bytes\n", readLen);
  
    int writtenLen = m_pDataIn->write(m_pBufRemaining, readLen);
    m_dataPos += writtenLen;
    
    DBG("%d bytes written\n", writtenLen);
     
    if(writtenLen<readLen) //Data was not completely written
    {
      m_pBufRemaining += writtenLen;
      m_bufRemainingLen = readLen - writtenLen;
      m_state = HTTP_READ_DATA_INCOMPLETE;
      return len + writtenLen;
    }
    else
    {
      m_state = HTTP_READ_DATA;
    }
    len += readLen;
  } while(readLen>0);
  
  return len;
}

void HTTPClient::readData() //Data has been read
{
  if(m_pDataIn == NULL) //Nothing to read (in HEAD for instance, not supported now)
  {
    m_state = HTTP_DONE;
    return;
  }
  DBG("Reading response...\n");
  int len = 0;
  do
  {
    if(m_dataChunked && (m_state != HTTP_READ_DATA_INCOMPLETE))
    {
      if(m_dataLen==0)
      {
        DBG("Reading chunk length...\n");
        //New block
        static char chunkHeader[16];
        //We use m_dataPos to retain the read position in chunkHeader, it has been set to 0 before the first call of readData()
        m_dataPos += readLine(chunkHeader + m_dataPos, ABS(16 - m_dataPos));
        if( m_dataPos > 0 )
        {
          if( chunkHeader[strlen(chunkHeader)-1] == 0x0d )
          {
            sscanf(chunkHeader, "%x%*[^\r\n]", &m_dataLen);
            DBG("Chunk length is %d\n", m_dataLen);
            m_dataPos = 0;
          }
          else
          {
            //Wait for end of line
            DBG("Wait for CRLF\n");
            return;
          }
        }
        else
        {
          DBG("Wait for data\n");
          //Wait for data
          return;
        }
      }
    }  
      
    //Proper data recovery
    len = tryRead();
    if(len<0) //Error
    {
      onResult(HTTP_CONN);
      return;
    }

    if(len>0)
      resetTimeout();
    
    if(m_state == HTTP_READ_DATA_INCOMPLETE)
      return;
      
    //Chunk Tail
    if(m_dataChunked)
    {  
      if(m_dataPos >= m_dataLen)
      {
        DBG("Chunk read, wait for CRLF\n");
        char chunkTail[3];
        m_dataPos += readLine(chunkTail, 3);
      }
      
      if(m_dataPos >= m_dataLen + 1) //1 == strlen("\n"),
      {
        DBG("End of chunk\n");
        if(m_dataLen==0)
        {
          DBG("End of file\n");
          //End of file
          m_state = HTTP_DONE; //Done
        }
        m_dataLen = 0;
        m_dataPos = 0;
      }
    }
  
  } while(len>0);
  
  
  if(!m_dataChunked && (m_dataPos >= m_dataLen)) //All Data has been received
  {
    DBG("End of file\n");
    m_state = HTTP_DONE; //Done
  }
}

void HTTPClient::writeData() //Data has been written & buf is free
{
  if(m_pDataOut == NULL) //Nothing to write (in POST for instance)
  {
    m_dataLen = 0; //Reset Data Length
    m_state = HTTP_READ_HEADERS;
    return;
  }
  int len = m_pDataOut->read(m_buf, CHUNK_SIZE);
  if( m_dataChunked )
  {
    //Write chunk header
    char chunkHeader[16];
    sprintf(chunkHeader, "%d\r\n", len);
    int ret = m_pTCPSocket->send(chunkHeader, strlen(chunkHeader));
    if(ret < 0)//Error
    {
      onResult(HTTP_CONN);
      return;
    }
  }
  m_pTCPSocket->send(m_buf, len);
  m_dataPos+=len;
  if( m_dataChunked )
  {
    m_pTCPSocket->send("\r\n", 2); //Chunk-terminating CRLF
  }
  if( ( !m_dataChunked && (m_dataPos >= m_dataLen) )
    || ( m_dataChunked && !len ) ) //All Data has been sent
  {
    m_dataLen = 0; //Reset Data Length
    m_state = HTTP_READ_HEADERS; //Wait for resp
  }
}
  
void HTTPClient::onTCPSocketEvent(TCPSocketEvent e)
{
  DBG("Event %d in HTTPClient::onTCPSocketEvent()\n", e);

  if(m_closed)
  {
    DBG("WARN: Discarded\n");
    return;
  }

  switch(e)
  {
  case TCPSOCKET_READABLE: //Incoming data
    resetTimeout();
    switch(m_state)
    {
    case HTTP_READ_HEADERS:
      if( !readHeaders() ) 
      {
        return; //Connection has been closed or incomplete data
      }
      if( m_pDataIn )
      {
        //Data chunked?
        if(m_respHeaders["Transfer-Encoding"].find("chunked")!=string::npos)
        {
          m_dataChunked = true;
          m_dataPos = 0;
          m_dataLen = 0; 
          DBG("Encoding is chunked, Content-Type is %s\n", m_respHeaders["Content-Type"].c_str() );
        }
        else
        {    
          m_dataChunked = false;
          int len = 0;
          //DBG("Preparing read... len = %s\n", m_respHeaders["Content-Length"].c_str());
          sscanf(m_respHeaders["Content-Length"].c_str(), "%d", &len);
          m_pDataIn->setDataLen( len );
          m_dataPos = 0;
          m_dataLen = len; 
          DBG("Content-Length is %d, Content-Type is %s\n", len, m_respHeaders["Content-Type"].c_str() );
        }
        m_pDataIn->setDataType( m_respHeaders["Content-Type"] );
      }
    case HTTP_READ_DATA:
      readData();
      break;
    case HTTP_READ_DATA_INCOMPLETE:
      break; //We need to handle previously received data first
    default:
      //Should not receive data now, req is not complete
      onResult(HTTP_PRTCL);
    }
 //All data has been read, close w/ success :)
    if( m_state == HTTP_DONE ) 
    {
      DBG("Done :)!\n");
      onResult(HTTP_OK);
    }
    break;
  case TCPSOCKET_CONNECTED:
  case TCPSOCKET_WRITEABLE: //We can send data
    resetTimeout();
    switch(m_state)
    {
    case HTTP_WRITE_HEADERS:
      //Update headers fields according to m_pDataOut
      if( m_pDataOut )
      {
        //Data is chunked?
        if(m_pDataOut->getIsChunked())
        {
          m_dataChunked = true;
          m_reqHeaders.erase("Content-Length");
          m_reqHeaders["Transfer-Encoding"] = "chunked";
        }
        else
        {
          m_dataChunked = false;
          char c_len[16] = "0";
          int len = m_pDataOut->getDataLen();
          sprintf(c_len, "%d", len);
          m_dataPos = 0;
          m_dataLen = len;
          m_reqHeaders.erase("Transfer-Encoding");
          m_reqHeaders["Content-Length"] = string(c_len);
        } 
        string type = m_pDataOut->getDataType();
        if(!type.empty())
        {
          m_reqHeaders["Content-Type"] = type;
        }
        else
        {
          m_reqHeaders.erase("Content-Type");
        }
      }
      if( !writeHeaders() ) 
      {
        return; //Connection has been closed
      }
      break; //Wait for writeable event before sending payload
    case HTTP_WRITE_DATA:
      writeData();
      break;
    }
    //Otherwise request has been sent, now wait for resp
    break;
  case TCPSOCKET_CONTIMEOUT:
  case TCPSOCKET_CONRST:
  case TCPSOCKET_CONABRT:
  case TCPSOCKET_ERROR:
    DBG("Connection error.\n");
    onResult(HTTP_CONN);
  case TCPSOCKET_DISCONNECTED:
    //There might still be some data available for reading
    //So if we are in a reading state, do not close the socket yet
    if( (m_state != HTTP_READ_DATA_INCOMPLETE) && (m_state != HTTP_DONE) && (m_state != HTTP_CLOSED)  )
    {
      onResult(HTTP_CONN);
    }
    DBG("Connection closed by remote host.\n");
    break;
  }
}

void HTTPClient::onDNSReply(DNSReply r)
{
  if(m_closed)
  {
    DBG("WARN: Discarded\n");
    return;
  }
  
  if( r != DNS_FOUND )
  {
    DBG("Could not resolve hostname.\n");
    onResult(HTTP_DNS);
    return;
  }
  
  DBG("DNS Resolved to %d.%d.%d.%d.\n",m_server.getIp()[0],m_server.getIp()[1],m_server.getIp()[2],m_server.getIp()[3]);
  //If no error, m_server has been updated by m_pDnsReq so we're set to go !
  m_pDnsReq->close();
  delete m_pDnsReq;
  m_pDnsReq = NULL;
  connect();
}

void HTTPClient::onResult(HTTPResult r) //Called when exchange completed or on failure
{
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)(r);
  else if(m_pCb)
    m_pCb(r);
  m_blockingResult = r; //Blocking mode
  close(); //FIXME:Remove suppl. close() calls
}

void HTTPClient::onTimeout() //Connection has timed out
{
  DBG("Timed out.\n");
  onResult(HTTP_TIMEOUT);
  close();
}

//Headers

//TODO: Factorize w/ HTTPRequestHandler in a single HTTPHeader class

HTTPResult HTTPClient::blockingProcess() //Called in blocking mode, calls Net::poll() until return code is available
{
  //Disable callbacks
  m_pCb = NULL;
  m_pCbItem = NULL;
  m_pCbMeth = NULL;
  m_blockingResult = HTTP_PROCESSING;
  do
  {
    Net::poll();
  } while(m_blockingResult == HTTP_PROCESSING);
  Net::poll(); //Necessary for cleanup
  return m_blockingResult;
}

bool HTTPClient::readHeaders()
{
  static char* line = m_buf;
  static char key[128];
  static char value[128];
  if(!m_dataLen) //No incomplete header in buffer, this is the first time we read data
  {
    if( readLine(line, 128) > 0 )
    {
      //Check RC
      m_httpResponseCode = 0;
      if( sscanf(line, "HTTP/%*d.%*d %d %*[^\r\n]", &m_httpResponseCode) != 1 )
      {
        //Cannot match string, error
        DBG("Not a correct HTTP answer : %s\n", line);
        onResult(HTTP_PRTCL);
        close();
        return false;
      }
      
      if(m_httpResponseCode != 200)
      {
        DBG("Response: error code %d\n", m_httpResponseCode);
        HTTPResult res = HTTP_ERROR;
        switch(m_httpResponseCode)
        {
        case 404:
          res = HTTP_NOTFOUND;
          break;
        case 403:
          res = HTTP_REFUSED;
          break;
        default:
          res = HTTP_ERROR;
        }
        onResult(res);
        close();
        return false;
      }
      DBG("Response OK\n");
    }
    else
    {
      //Empty packet, weird!
      DBG("Empty packet!\n");
      onResult(HTTP_PRTCL);
      close();
      return false;
    }
  }
  bool incomplete = false;
  while( true )
  {
    int readLen = readLine(line + m_dataLen, 128 - m_dataLen, &incomplete);
    m_dataLen = 0;
    if( readLen <= 2 ) //if == 1 or 2, it is an empty line = end of headers
    {
      DBG("All headers read.\n");
      m_state = HTTP_READ_DATA;
      break;
    }
    else if( incomplete == true )
    {
      m_dataLen = readLen;//Sets data length available in buffer
      return false;
    }
    //DBG("Header : %s\n", line);
    int n = sscanf(line, "%[^:] : %[^\r\n]", key, value);
    if ( n == 2 )
    {
      DBG("Read header : %s: %s\n", key, value);
      m_respHeaders[key] = value;
    }
    //TODO: Impl n==1 case (part 2 of previous header)
  }

  return true;
}

bool HTTPClient::writeHeaders() //Called at the first writeData call
{
  static char* line = m_buf;
  const char* HTTP_METH_STR[] = {"GET", "POST", "HEAD"};
  
  //Req
  sprintf(line, "%s %s HTTP/1.1\r\nHost: %s\r\n", HTTP_METH_STR[m_meth], m_path.c_str(), m_server.getName()); //Write request
  m_pTCPSocket->send(line, strlen(line));
  DBG("Request: %s\n", line);
  
  DBG("Writing headers:\n");
  map<string,string>::iterator it;
  for( it = m_reqHeaders.begin(); it != m_reqHeaders.end(); it++ )
  {
    sprintf(line, "%s: %s\r\n", (*it).first.c_str(), (*it).second.c_str() );
    DBG("\r\n%s", line);
    m_pTCPSocket->send(line, strlen(line));
  }
  m_pTCPSocket->send("\r\n",2); //End of head
  m_state = HTTP_WRITE_DATA;
  return true;
}

int HTTPClient::readLine(char* str, int maxLen, bool* pIncomplete /* = NULL*/)
{
  int ret;
  int len = 0;
  if(pIncomplete)
    *pIncomplete = false;
  for(int i = 0; i < maxLen - 1; i++)
  {
    ret = m_pTCPSocket->recv(str, 1);
    if(ret != 1)
    {
      if(pIncomplete)
        *pIncomplete = true;
      break;
    }
    if( (len > 1) && *(str-1)=='\r' && *str=='\n' )
    {
      break;
    }
    else if( *str=='\n' )
    {
      break;    
    }
    str++;
    len++;
  }
  *str = 0;
  return len;
}
