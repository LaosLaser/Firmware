
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

#include "netCfg.h"
#if NET_GPRS_MODULE

#include "GPRSModuleNetIf.h"

#define __DEBUG
#include "dbg/dbg.h"

GPRSModuleNetIf::GPRSModuleNetIf(PinName tx, PinName rx, int baud /*= 115200*/) : PPPNetIf(NULL), m_serial(tx, rx)
{
  PPPNetIf::m_pIf = new GPRSModem();
  m_serial.baud(baud);
}

GPRSModuleNetIf::~GPRSModuleNetIf()
{
  delete PPPNetIf::m_pIf;
}
  
PPPErr GPRSModuleNetIf::connect(const char* apn /*= NULL*/, const char* userId /*= NULL*/, const char* password /*= NULL*/) //Connect using GPRS
{

  DBG("Powering on module.\n")
  if(!setOn()) //Could not power on module
  {
    DBG("Could not power on module.\n");
    return PPP_MODEM;
  }
  
  //wait(10); //Wait for module to init.

  ATErr atErr;
  for(int i=0; i<3; i++)
  {
    atErr = m_pIf->open(&m_serial); //3 tries
    if(!atErr)
      break;
    DBG("Could not open AT If, trying again.\n");
    wait(4);
  }
    
  if(atErr)
  {
    setOff();
    return PPP_MODEM;
  }
    
  DBG("AT If opened.\n");

  PPPErr pppErr;
  for(int i=0; i<3; i++)
  {  
    DBG("Trying to connect.\n");
    pppErr = PPPNetIf::GPRSConnect(apn, userId, password);
    if(!pppErr)
      break;
    DBG("Could not connect.\n");
    wait(4);
  }
  if(pppErr)
  {
    setOff();
    return pppErr;
  }
    
  DBG("Connected.\n");
  
  return PPP_OK;
}

PPPErr GPRSModuleNetIf::disconnect()
{
  DBG("Disconnecting...\n");
  PPPErr pppErr = PPPNetIf::disconnect();
  if(pppErr)
    return pppErr;
    
  m_pIf->close();
  
  DBG("Powering off module.\n")
  setOff(); //Power off module
  
  DBG("Off.\n")
  
  return PPP_OK;
}
  

#endif
