
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

#include "PPPNetIf.h"
#include "mbed.h"
#include "ppp/ppp.h"
#include "lwip/init.h"
#include "lwip/sio.h"

#define __DEBUG
#include "dbg/dbg.h"

#include "netCfg.h"
#if NET_PPP

#define PPP_TIMEOUT 60000

#define BUF_SIZE 256

PPPNetIf::PPPNetIf(GPRSModem* pIf) : LwipNetIf(), m_pIf(pIf),/* m_open(false),*/ m_connected(false), m_status(PPP_DISCONNECTED), m_fd(0) //, m_id(0)
{
  //FIXME: Check static refcount
  m_buf = new uint8_t[BUF_SIZE];
}

PPPNetIf::~PPPNetIf()
{
  delete[] m_buf;
}

#if 0
PPPErr PPPNetIf::open(Serial* pSerial)
{
  GPRSErr err = m_pIf->open(pSerial);
  if(err)
    return PPP_MODEM;
  m_open = true;
  #if 0
  m_id = sioMgr::registerSerialIf(this);
  if(!m_id)
  {
    close();
    return PPP_CLOSED;
  }
  #endif
  return PPP_OK;
}
#endif


PPPErr PPPNetIf::GPRSConnect(const char* apn, const char* userId, const char* password) //Connect using GPRS
{
  LwipNetIf::init();
  pppInit();
  //TODO: Tell ATIf that we get ownership of the serial port
    
  GPRSErr gprsErr;
  gprsErr = m_pIf->connect(apn);
  if(gprsErr)
    return PPP_NETWORK;
    
  DBG("PPPNetIf: If Connected.\n");

  if( userId == NULL )
    pppSetAuth(PPPAUTHTYPE_NONE, NULL, NULL);
  else
    pppSetAuth(PPPAUTHTYPE_PAP, userId, password); //TODO: Allow CHAP as well
    
  DBG("PPPNetIf: Set Auth.\n");
  
  //wait(1.);
  
  //m_pIf->flushBuffer(); //Flush buffer before passing serial port to PPP
  
  m_status = PPP_CONNECTING;
  DBG("m_pIf = %p\n", m_pIf);
  int res = pppOverSerialOpen((void*)m_pIf, sPppCallback, (void*)this);
  DBG("PPP connected\n");
  if(res<0)
  {
    disconnect();
    return PPP_PROTOCOL;
  }
  
  DBG("PPPNetIf: PPP Started with res = %d.\n", res);
  
  m_fd = res;
  m_connected = true;
  Timer t;
  t.start();
  while( m_status == PPP_CONNECTING ) //Wait for callback
  {
    poll();
    if(t.read_ms()>PPP_TIMEOUT)
    {
      DBG("PPPNetIf: Timeout.\n");  
      disconnect();
      return PPP_PROTOCOL;
    }
  }
  
  DBG("PPPNetIf: Callback returned.\n");
  
  if( m_status == PPP_DISCONNECTED )
  {
    disconnect();
    return PPP_PROTOCOL;
  }
  
  return PPP_OK;

}

PPPErr PPPNetIf::ATConnect(const char* number) //Connect using a "classic" voice modem or GSM
{
  //TODO: IMPL
  return PPP_MODEM;
}

PPPErr PPPNetIf::disconnect()
{
  if(m_fd)
    pppClose(m_fd); //0 if ok, else should gen a WARN
  m_connected = false;
  
  m_pIf->flushBuffer();
  m_pIf->printf("+++\r\n");
  wait(.5);
  m_pIf->flushBuffer();
  
  GPRSErr gprsErr;  
  gprsErr = m_pIf->disconnect();
  if(gprsErr)
    return PPP_NETWORK;
    
  return PPP_OK;
}

#if 0
PPPErr PPPNetIf::close()
{
  GPRSErr err = m_pIf->close();
  if(err)
    return PPP_MODEM;
  m_open = false;
  return PPP_OK;
}
#endif


#if 0
//We have to use :

/** Pass received raw characters to PPPoS to be decoded. This function is
 * thread-safe and can be called from a dedicated RX-thread or from a main-loop.
 *
 * @param pd PPP descriptor index, returned by pppOpen()
 * @param data received data
 * @param len length of received data
 */
void
pppos_input(int pd, u_char* data, int len)
{
  pppInProc(&pppControl[pd].rx, data, len);
}
#endif

void PPPNetIf::poll()
{
  if(!m_connected)
    return;
  LwipNetIf::poll();
  //static u8_t buf[128];
  int len;
  do
  {
    len = sio_tryread((sio_fd_t) m_pIf, m_buf, BUF_SIZE);
    if(len > 0)
      pppos_input(m_fd, m_buf, len);
  } while(len>0);
}

//Link Callback
void PPPNetIf::pppCallback(int errCode, void *arg)
{
  switch ( errCode )
  {
    //No error
    case PPPERR_NONE:
      {
        struct ppp_addrs* addrs = (struct ppp_addrs*) arg;
        m_ip = IpAddr(&(addrs->our_ipaddr)); //Set IP
      }
      m_status = PPP_CONNECTED;
      break;
    default:
      //Disconnected
      DBG("PPPNetIf: Callback errCode = %d.\n", errCode);
      m_status = PPP_DISCONNECTED;
    break;
  }
}

#endif
