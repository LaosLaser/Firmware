
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
NTP Client header file
*/

#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include "core/net.h"
#include "core/netservice.h"
#include "api/UDPSocket.h"
#include "api/DNSRequest.h"
#include "mbed.h"
#include "arch/cc.h"

///NTP Client results
enum NTPResult
{
  NTP_OK, ///<Success
  NTP_PROCESSING, ///<Processing
  NTP_PRTCL, ///<Protocol error
  NTP_TIMEOUT, ///<Connection timeout
  NTP_DNS ///<Could not resolve DNS hostname
};

///A NTP Client
/**
The NTP client is a simple UDP client that will update the mbed's RTC
*/
class NTPClient : protected NetService
{
public:
  /**
  Instantiates the NTP client
  */
  NTPClient();
  virtual ~NTPClient();
  
  //High level setup functions
  
  ///Gets current time (blocking)
  /**
  Updates the time using the server host
  Blocks until completion
  @param host : NTP server
  */
  NTPResult setTime(const Host& host); //Blocking
  
  ///Gets current time (non-blocking)
  /**
  Updates the time using the server host
  The function returns immediately and calls the callback on completion or error
  @param host : NTP server
  @param pMethod : callback function
  */
  NTPResult setTime(const Host& host, void (*pMethod)(NTPResult)); //Non blocking
  
  ///Gets current time (non-blocking)
  /**
  Updates the time
  @param host : NTP server
  @param pItem : instance of class on which to execute the callback method
  @param pMethod : callback method
  The function returns immediately and calls the callback on completion or error
  */
  template<class T> 
  NTPResult setTime(const Host& host, T* pItem, void (T::*pMethod)(NTPResult)) //Non blocking
  {
    setOnResult(pItem, pMethod);
    doSetTime(host);
    return NTP_PROCESSING;
  }
  
  ///Gets current time (non-blocking)
  /**
  Updates the time using the server host
  The function returns immediately and calls the previously set callback on completion or error
  @param host : NTP server
  */
  void doSetTime(const Host& host);
  
  ///Setups the result callback
  /**
  @param pMethod : callback function
  */
  void setOnResult( void (*pMethod)(NTPResult) );
  
  ///Setups the result callback
  /**
  @param pItem : instance of class on which to execute the callback method
  @param pMethod : callback method
  */
  class CDummy;
  template<class T> 
  void setOnResult( T* pItem, void (T::*pMethod)(NTPResult) )
  {
    m_pCbItem = (CDummy*) pItem;
    m_pCbMeth = (void (CDummy::*)(NTPResult)) pMethod;
  }
  
  void close();
  
protected:
  virtual void poll(); //Called by NetServices
  
private:
  void init();
  void open();
  
  #ifdef PACK_STRUCT_USE_INCLUDES
  #  include "arch/bpstruct.h"
  #endif
  PACK_STRUCT_BEGIN
  struct NTPPacket //See RFC 4330 for Simple NTP
  {
    //WARN: We are in LE! Network is BE!
    //LSb first
    unsigned mode : 3;
    unsigned vn : 3;
    unsigned li : 2;
    
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    //32 bits header
    
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint32_t refId;
    
    uint32_t refTm_s;
    uint32_t refTm_f;
    uint32_t origTm_s;
    uint32_t origTm_f;
    uint32_t rxTm_s;
    uint32_t rxTm_f;
    uint32_t txTm_s;
    uint32_t txTm_f;
  } PACK_STRUCT_STRUCT;
  PACK_STRUCT_END
  #ifdef PACK_STRUCT_USE_INCLUDES
  #  include "arch/epstruct.h"
  #endif

  void process(); //Main state-machine

  void setTimeout(int ms);
  void resetTimeout();
  
  void onTimeout(); //Connection has timed out
  void onDNSReply(DNSReply r);
  void onUDPSocketEvent(UDPSocketEvent e);
  void onResult(NTPResult r); //Called when exchange completed or on failure
  
  NTPResult blockingProcess(); //Called in blocking mode, calls Net::poll() until return code is available

  UDPSocket* m_pUDPSocket;

  enum NTPStep
  {
    NTP_PING,
    NTP_PONG
  };
  
  NTPStep m_state;
  
  NTPPacket m_pkt;
  
  CDummy* m_pCbItem;
  void (CDummy::*m_pCbMeth)(NTPResult);
  
  void (*m_pCb)(NTPResult);
  
  Timer m_watchdog;
  int m_timeout;
  
  bool m_closed;
  
  Host m_host;
  
  DNSRequest* m_pDnsReq;
  
  NTPResult m_blockingResult; //Result if blocking mode

};

#endif
