
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

#include "GPRSModem.h"
#include "mbed.h"

//#define __DEBUG
#include "dbg/dbg.h"

#define WAIT_BTW_NETW_POLLS 3.

#include "netCfg.h"
#if NET_GPRS

GPRSModem::GPRSModem() : ATIf()
{
  DBG("New GPRSModem@%p\n", this);
}

GPRSModem::~GPRSModem()
{

}
  
GPRSErr GPRSModem::getNetworkState()
{
  ATIf::flushBuffer();
  /*
  netState can be : (Telit_AT_Reference_Guide.pdf p.98)
  0 - not registered, ME is not currently searching a new operator to register to
  1 - registered, home network
  2 - not registered, but ME is currently searching a new operator to register to
  3 - registration denied
  4 - unknown
  5 - registered, roaming
  */
 // DBG("Network?...\r\n");
  ATIf::setReadMode(false); //Discard chars
  ATIf::setTimeout(10000);
  ATIf::setLineMode(true); //Line mode
  int netState = 0;
  int len;
  len = ATIf::printf("AT+CREG?"); //Registered ?
  if(!len) DBG("\r\nprintf - len=%d\r\n",len);
  if(!len)
    return GPRS_MODEM; //Nothing was actually sent
    
  len = ATIf::scanf("+CREG: 0,%d", &netState); //Get status
  if(len != 1) DBG("\r\nscanf - len=%d\r\n",len);
  if(len != 1) //Likely +CMS ERROR was returned
    return GPRS_MODEM;

  if( !!ATIf::checkOK() ) //Should not be a problem
    {DBG("\r\nNOK\r\n"); return GPRS_MODEM; }
    
  switch(netState)
  {
  case 1:
  case 5: //TODO: Option allow roaming
    DBG("\r\nNetwork is up!\r\n");
    return GPRS_OK;
  case 3:
    DBG("\r\nAccess to network denied.\r\n");
    return GPRS_DENIED;
  case 0:
    DBG("\r\nNo network.\r\n");
    return GPRS_NONETWORK;
  case 4:
  case 2:
    //DBG("\r\nRegistering...\r\n");
    return GPRS_REGISTERING;
  }
    
  return GPRS_MODEM; // Should not reach this
  
}

GPRSErr GPRSModem::setNetworkUp()
{
  ATIf::flushBuffer();
  GPRSErr err = GPRS_REGISTERING;
  while(true)
  {
    err = getNetworkState();
    if(err != GPRS_REGISTERING)
      break;
    wait(WAIT_BTW_NETW_POLLS);
  }
  return err;
}

//Same, but for GPRS
GPRSErr GPRSModem::getGPRSState()
{
  ATIf::flushBuffer();
  /*
  netState can be : (Telit_AT_Reference_Guide.pdf p.192)
  0 - not registered, terminal is not currently searching a new operator to register to
  1 - registered, home network
  2 - not registered, but terminal is currently searching a new operator to register to
  3 - registration denied
  4 - unknown
  5 - registered, roaming
  */
  
  DBG("GPRS?...\r\n");
  ATIf::setReadMode(false); //Discard chars
  ATIf::setTimeout(10000);
  ATIf::setLineMode(true); //Line mode
  int netState = 0;
  int len;
  len = ATIf::printf("AT+CGREG?"); //Registered ?
  if(!len)
    return GPRS_MODEM; //Nothing was actually sent
    
  len = ATIf::scanf("+CGREG: %*d,%d", &netState); //Get GPRS status, see GSM 07.07 spec as Telit AT ref is wrong
  if(len != 1) DBG("\r\nscanf - len=%d\r\n",len);
  if(len != 1) //Likely +CMS ERROR was returned
    return GPRS_MODEM;

  if( !!ATIf::checkOK() ) //Should not be a problem
    return GPRS_MODEM;
    
  switch(netState)
  {
  case 1:
  case 5: //TODO: Option allow roaming
    DBG("\r\nNetwork is up!\r\n");
    return GPRS_OK;
  case 3:
    DBG("\r\nAccess to network denied.\r\n");
    return GPRS_DENIED;
  case 0:
    DBG("\r\nNo network.\r\n");
    return GPRS_NONETWORK;
  case 4:
  case 2:
    DBG("\r\nRegistering...\r\n");
    return GPRS_REGISTERING;
  }
    
  return GPRS_MODEM; // Should not reach this
  
}

GPRSErr GPRSModem::setGPRSUp()
{
  ATIf::flushBuffer();
  GPRSErr err;
  
  err = setNetworkUp();
  if(err)
    return err;
  
  DBG("\r\nAttaching GPRS...\r\n");
  ATIf::setReadMode(false); //Discard chars
  ATIf::setTimeout(10000);
  ATIf::setLineMode(true); //Line mode
  int len;  
  
  err = getGPRSState();
  if(err == GPRS_NONETWORK)
  {  
    len = ATIf::printf("AT+CGATT=1"); //Attach
    if(!len)
      return GPRS_MODEM; //Nothing was actually sent  

    if( !!ATIf::checkOK() ) //Should not be a problem
      return GPRS_MODEM;
  }
  
  while(true)
  {
    err = getGPRSState();
    if(err != GPRS_REGISTERING)
      break;
    wait(WAIT_BTW_NETW_POLLS);
  }
  return err;
}

GPRSErr GPRSModem::setGPRSDown()
{ 
  ATIf::flushBuffer();
  DBG("\r\nDetaching GPRS...\r\n");
  ATIf::setReadMode(false); //Discard chars
  ATIf::setTimeout(10000);
  ATIf::setLineMode(true); //Line mode
  int len;
  
  len = ATIf::printf("AT+CGATT=0"); //Detach
  if(!len)
    return GPRS_MODEM; //Nothing was actually sent  

  if( !!ATIf::checkOK() ) //Should not be a problem
    return GPRS_MODEM;
 
  return GPRS_OK;
}

  
GPRSErr GPRSModem::connect(const char* apn /*=NULL*/)
{
  ATIf::flushBuffer();
  GPRSErr err;
    
  ATIf::setReadMode(false); //Discard chars
  ATIf::setTimeout(5000);
  ATIf::setLineMode(true); //Line mode
  
  DBG("\r\nConnecting...\r\n");  
  
  int len;
  
  if( apn != NULL ) //Config APN
  {
    len = ATIf::printf("AT+CGDCONT=1,\"IP\",\"%s\"",apn); //Define APN
    if(!len)
      return GPRS_MODEM; //Nothing was actually sent

    if( !!ATIf::checkOK() ) //Should not be a problem
      return GPRS_MODEM;
  }
    
  err = setGPRSUp();
  if(err)
    return err;
    
  ATIf::setReadMode(false); //Discard chars
  ATIf::setTimeout(60000);
  ATIf::setLineMode(true); //Line mode
  //
  //len = ATIf::printf("AT+CGDATA=\"PPP\",1"); //Connect using PDP context #1
//  len = ATIf::printf("ATDT *99***1#");
  len = ATIf::printf("ATDT *99#");
  if(!len)
    return GPRS_MODEM; //Nothing was actually sent
    
  len = ATIf::scanf("CONNECT"); //Beginning of session
  if(len != 0) //Likely +CME ERROR was returned or NO CARRIER
    return GPRS_MODEM;
    
  //ATIf::setSignals(false);
  
  DBG("\r\nConnected.\r\n");
    
  return GPRS_OK; //Time to enter a PPP Session !   
  
}

GPRSErr GPRSModem::disconnect()
{
  ATIf::flushBuffer();
  ATIf::setReadMode(false); //Discard chars
  ATIf::setTimeout(5000);
  ATIf::setLineMode(true); //Line mode
  
  if( !!ATIf::checkOK() ) //Should be present at the end of connection
    return GPRS_MODEM;
  
  GPRSErr err;  
  err = setGPRSDown();
  if(err)
    return err;
    
  DBG("\r\nDisconnected.\r\n");
    
  return GPRS_OK;
}

#endif

