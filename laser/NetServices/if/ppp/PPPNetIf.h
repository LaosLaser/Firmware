
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
PPP Generic network interface header file
*/

//This is a backend for PPP, using lwIP

#ifndef PPPNETIF_H
#define PPPNETIF_H

#include "mbed.h"

//For now, only a GPRS Modem is supported, 
//but we could easily split this class into a base class + two derived classes
// (PPP over GPRS + PPP over Serial Modem)
#include "drv/gprs/GPRSModem.h"
#include "if/lwip/LwipNetIf.h"

///PPP connection error codes
enum PPPErr
{
  __PPP_MIN = -0xFFFF,
  PPP_MODEM, ///<AT error returned
  PPP_NETWORK, ///<Network is down
  PPP_PROTOCOL, ///<PPP Protocol error
  PPP_CLOSED, ///<Connection is closed
  PPP_OK = 0 ///<Success
};

enum PPPStatus
{
  PPP_CONNECTING,
  PPP_CONNECTED,
  PPP_DISCONNECTED,
};

class PPPNetIf : public LwipNetIf
{
public:
  PPPNetIf(GPRSModem* pIf);
  virtual ~PPPNetIf();
  
  #if 0
  PPPErr open(Serial* pSerial);
  #endif
  
  PPPErr GPRSConnect(const char* apn, const char* userId, const char* password); //Connect using GPRS
  PPPErr ATConnect(const char* number); //Connect using a "classic" voice modem or GSM
  
  virtual void poll();
  
  PPPErr disconnect();
  
  #if 0
  PPPErr close();
  #endif
  
protected:
  GPRSModem* m_pIf;
  
private:
  void pppCallback(int errCode, void *arg);
  
  static void sPppCallback(void *ctx, int errCode, void *arg) //Callback from ppp.c
  { return ((PPPNetIf*)ctx)->pppCallback(errCode, arg); }
  
  bool m_connected;
  /*bool m_open;*/
  volatile PPPStatus m_status;
  volatile int m_fd; //PPP Session descriptor
  
  //int m_id;
  
  uint8_t* m_buf;

};

#endif
