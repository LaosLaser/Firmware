
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
#if NET_UMTS

#include "UMTSStickNetIf.h"

#define __DEBUG
#include "dbg/dbg.h"

UMTSStickNetIf::UMTSStickNetIf() : PPPNetIf(NULL), m_umtsStick(), m_pUsbSerial(NULL)
{
  PPPNetIf::m_pIf = new GPRSModem();
}

UMTSStickNetIf::~UMTSStickNetIf()
{
  delete PPPNetIf::m_pIf;
  if(m_pUsbSerial)
    delete m_pUsbSerial;
}
  
UMTSStickErr UMTSStickNetIf::setup() //UMTSStickErr is from /drv/umtsstick/UMTSStick.h
{
  return m_umtsStick.getSerial(&m_pUsbSerial);
}

PPPErr UMTSStickNetIf::connect(const char* apn /*= NULL*/, const char* userId /*= NULL*/, const char* password /*= NULL*/) //Connect using GPRS
{
  if(!m_pUsbSerial)
    return PPP_MODEM;

  ATErr atErr;
  for(int i=0; i<3; i++)
  {
    atErr = m_pIf->open(m_pUsbSerial); //3 tries
    if(!atErr)
      break;
    wait(4);
  }
    
  if(atErr)
    return PPP_MODEM;

  PPPErr pppErr;
  for(int i=0; i<3; i++)
  {  
    pppErr = PPPNetIf::GPRSConnect(apn, userId, password);
    if(!pppErr)
      break;
    wait(4);
  }
  if(pppErr)
    return pppErr;
  
  return PPP_OK;
}

PPPErr UMTSStickNetIf::disconnect()
{
  PPPErr pppErr = PPPNetIf::disconnect();
  if(pppErr)
    return pppErr;
    
  m_pIf->close();
  
  return PPP_OK;
}
  

#endif
