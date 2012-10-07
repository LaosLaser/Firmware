
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

/** \file
HTTP Client header file
*/

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

class HTTPData;

#include "core/net.h"
#include "api/TCPSocket.h"
#include "api/DNSRequest.h"
#include "HTTPData.h"
#include "mbed.h"

#include <string>
using std::string;

#include <map>
using std::map;

///HTTP client results
enum HTTPResult
{
  HTTP_OK, ///<Success
  HTTP_PROCESSING, ///<Processing
  HTTP_PARSE, ///<URI Parse error
  HTTP_DNS, ///<Could not resolve name
  HTTP_PRTCL, ///<Protocol error
  HTTP_NOTFOUND, ///<HTTP 404 Error
  HTTP_REFUSED, ///<HTTP 403 Error
  HTTP_ERROR, ///<HTTP xxx error
  HTTP_TIMEOUT, ///<Connection timeout
  HTTP_CONN ///<Connection error
};

#include "core/netservice.h"

///A simple HTTP Client
/**
The HTTPClient is composed of:
- The actual client (HTTPClient)
- Classes that act as a data repository, each of which deriving from the HTTPData class (HTTPText for short text content, HTTPFile for file I/O, HTTPMap for key/value pairs, and HTTPStream for streaming purposes)
*/
class HTTPClient : protected NetService
{
public:
  ///Instantiates the HTTP client
  HTTPClient();
  virtual ~HTTPClient();
  
  ///Provides a basic authentification feature (Base64 encoded username and password)
  void basicAuth(const char* user, const char* password); //Basic Authentification
  
  //High Level setup functions
  ///Executes a GET Request (blocking)
  /**
  Executes a GET request on the URI uri
  @param uri : URI on which to execute the request
  @param pDataIn : pointer to an HTTPData instance that will collect the data returned by the request, can be NULL
  Blocks until completion
  */
  HTTPResult get(const char* uri, HTTPData* pDataIn); //Blocking
  
  ///Executes a GET Request (non blocking)
  /**
  Executes a GET request on the URI uri
  @param uri : URI on which to execute the request
  @param pDataIn : pointer to an HTTPData instance that will collect the data returned by the request, can be NULL
  @param pMethod : callback function
  The function returns immediately and calls the callback on completion or error
  */
  HTTPResult get(const char* uri, HTTPData* pDataIn, void (*pMethod)(HTTPResult)); //Non blocking
  
  ///Executes a GET Request (non blocking)
  /**
  Executes a GET request on the URI uri
  @param uri : URI on which to execute the request
  @param pDataIn : pointer to an HTTPData instance that will collect the data returned by the request, can be NULL
  @param pItem : instance of class on which to execute the callback method
  @param pMethod : callback method
  The function returns immediately and calls the callback on completion or error
  */
  template<class T> 
  HTTPResult get(const char* uri, HTTPData* pDataIn, T* pItem, void (T::*pMethod)(HTTPResult)) //Non blocking
  {
    setOnResult(pItem, pMethod);
    doGet(uri, pDataIn);
    return HTTP_PROCESSING;
  }
  
  ///Executes a POST Request (blocking)
  /**
  Executes a POST request on the URI uri
  @param uri : URI on which to execute the request
  @param dataOut : a HTTPData instance that contains the data that will be posted
  @param pDataIn : pointer to an HTTPData instance that will collect the data returned by the request, can be NULL
  Blocks until completion
  */
  HTTPResult post(const char* uri, const HTTPData& dataOut, HTTPData* pDataIn); //Blocking
  
  ///Executes a POST Request (non blocking)
  /**
  Executes a POST request on the URI uri
  @param uri : URI on which to execute the request
  @param dataOut : a HTTPData instance that contains the data that will be posted
  @param pDataIn : pointer to an HTTPData instance that will collect the data returned by the request, can be NULL
  @param pMethod : callback function
  The function returns immediately and calls the callback on completion or error
  */
  HTTPResult post(const char* uri, const HTTPData& dataOut, HTTPData* pDataIn, void (*pMethod)(HTTPResult)); //Non blocking
  
  ///Executes a POST Request (non blocking)
  /**
  Executes a POST request on the URI uri
  @param uri : URI on which to execute the request
  @param dataOut : a HTTPData instance that contains the data that will be posted
  @param pDataIn : pointer to an HTTPData instance that will collect the data returned by the request, can be NULL
  @param pItem : instance of class on which to execute the callback method
  @param pMethod : callback method
  The function returns immediately and calls the callback on completion or error
  */
  template<class T> 
  HTTPResult post(const char* uri, const HTTPData& dataOut, HTTPData* pDataIn, T* pItem, void (T::*pMethod)(HTTPResult)) //Non blocking  
  {
    setOnResult(pItem, pMethod);
    doPost(uri, dataOut, pDataIn);
    return HTTP_PROCESSING;
  }

  ///Executes a GET Request (non blocking)
  /**
  Executes a GET request on the URI uri
  @param uri : URI on which to execute the request
  @param pDataIn : pointer to an HTTPData instance that will collect the data returned by the request, can be NULL
  The function returns immediately and calls the previously set callback on completion or error
  */  
  void doGet(const char* uri, HTTPData* pDataIn);  
  
  ///Executes a POST Request (non blocking)
  /**
  Executes a POST request on the URI uri
  @param uri : URI on which to execute the request
  @param dataOut : a HTTPData instance that contains the data that will be posted
  @param pDataIn : pointer to an HTTPData instance that will collect the data returned by the request, can be NULL
  @param pMethod : callback function
  The function returns immediately and calls the previously set callback on completion or error
  */
  void doPost(const char* uri, const HTTPData& dataOut, HTTPData* pDataIn); 
  
  ///Setups the result callback
  /**
  @param pMethod : callback function
  */
  void setOnResult( void (*pMethod)(HTTPResult) );
  
  ///Setups the result callback
  /**
  @param pItem : instance of class on which to execute the callback method
  @param pMethod : callback method
  */
  class CDummy;
  template<class T> 
  void setOnResult( T* pItem, void (T::*pMethod)(HTTPResult) )
  {
    m_pCb = NULL;
    m_pCbItem = (CDummy*) pItem;
    m_pCbMeth = (void (CDummy::*)(HTTPResult)) pMethod;
  }

  ///Setups timeout
  /**
  @param ms : time of connection inactivity in ms after which the request should timeout
  */
  void setTimeout(int ms);
  
  virtual void poll(); //Called by NetServices
  
  ///Gets last request's HTTP response code
  /**
  @return The HTTP response code of the last request
  */
  int getHTTPResponseCode();
  
  ///Sets a specific request header
  void setRequestHeader(const string& header, const string& value);
  
  ///Gets a response header
  string& getResponseHeader(const string& header);
  
  ///Clears request headers
  void resetRequestHeaders();
  
protected:
  void resetTimeout();
  
  void init();
  void close();
  
  void setup(const char* uri, HTTPData* pDataOut, HTTPData* pDataIn); //Setup request, make DNS Req if necessary
  void connect(); //Start Connection
  
  int  tryRead(); //Read data and try to feed output
  void readData(); //Data has been read
  void writeData(); //Data has been written & buf is free
  
  void onTCPSocketEvent(TCPSocketEvent e);
  void onDNSReply(DNSReply r);
  void onResult(HTTPResult r); //Called when exchange completed or on failure
  void onTimeout(); //Connection has timed out
  
private:
  HTTPResult blockingProcess(); //Called in blocking mode, calls Net::poll() until return code is available

  bool readHeaders(); //Called first when receiving data
  bool writeHeaders(); //Called to create req
  int readLine(char* str, int maxLen, bool* pIncomplete = NULL);
  
  enum HTTP_METH
  {
    HTTP_GET,
    HTTP_POST,
    HTTP_HEAD
  };
  
  HTTP_METH m_meth;
  
  CDummy* m_pCbItem;
  void (CDummy::*m_pCbMeth)(HTTPResult);
  
  void (*m_pCb)(HTTPResult);
  
  TCPSocket* m_pTCPSocket;
  map<string, string> m_reqHeaders;
  map<string, string> m_respHeaders;
  
  Timer m_watchdog;
  int m_timeout;
  
  DNSRequest* m_pDnsReq;
  
  Host m_server;
  string m_path;
  
  bool m_closed;
  
  enum HTTPStep
  {
   // HTTP_INIT,
    HTTP_WRITE_HEADERS,
    HTTP_WRITE_DATA,
    HTTP_READ_HEADERS,
    HTTP_READ_DATA,
    HTTP_READ_DATA_INCOMPLETE,
    HTTP_DONE,
    HTTP_CLOSED
  };
  
  HTTPStep m_state;
  
  HTTPData* m_pDataOut;
  HTTPData* m_pDataIn;
  
  bool m_dataChunked; //Data is encoded as chunks
  int m_dataPos; //Position in data
  int m_dataLen; //Data length
  char* m_buf;
  char* m_pBufRemaining; //Remaining
  int m_bufRemainingLen; //Data length in m_pBufRemaining
  
  int m_httpResponseCode;
  
  HTTPResult m_blockingResult; //Result if blocking mode
  
};

//Including data containers here for more convenience
#include "data/HTTPFile.h"
#include "data/HTTPStream.h"
#include "data/HTTPText.h"
#include "data/HTTPMap.h"

#endif
