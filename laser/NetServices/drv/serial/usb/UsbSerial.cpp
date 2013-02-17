
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

// #include "rpc.h"

#include "UsbSerial.h"

//#define __DEBUG
#include "dbg/dbg.h"

#include "netCfg.h"
#if NET_USB_SERIAL

namespace mbed {

#define BUF_LEN 64
#define FLUSH_TMOUT 100000 //US

UsbSerial::UsbSerial(UsbDevice* pDevice, int epIn, int epOut, const char* name /*= NULL*/) : Stream(name), m_epIn(pDevice, epIn, true, USB_BULK, BUF_LEN), m_epOut(pDevice, epOut, false, USB_BULK, BUF_LEN),
m_pInCbItem(NULL), m_pInCbMeth(NULL), m_pOutCbItem(NULL), m_pOutCbMeth(NULL)
{ 
  m_inBufEven = new char[BUF_LEN];
  m_inBufOdd = new char[BUF_LEN];
  m_pInBufPos = m_inBufUsr = m_inBufEven;
  m_inBufTrmt = m_inBufOdd;
  
  m_outBufEven = new char[BUF_LEN];
  m_outBufOdd = new char[BUF_LEN];
  m_pOutBufPos = m_outBufUsr = m_outBufEven;
  m_outBufTrmt = m_outBufOdd;
  
  m_inBufLen = m_outBufLen = 0;
  
  DBG("Starting RX'ing on in ep\n");
  
  m_timeout = false;
  
  m_epIn.setOnCompletion(this, &UsbSerial::onEpInTransfer);
  m_epOut.setOnCompletion(this, &UsbSerial::onEpOutTransfer);
  
  startRx();
}

UsbSerial::~UsbSerial()
{
  delete[] m_inBufEven;
  delete[] m_inBufOdd;
  delete[] m_outBufEven;
  delete[] m_outBufOdd;
}

void UsbSerial::baud(int baudrate) { 
    //
}

void UsbSerial::format(int bits, int parity, int stop) { 
  //
} 

#if 0 //For doc only
template <class T>
void attach(T* pCbItem, void (T::*pCbMeth)())
{
  m_pCbItem = (CDummy*) pCbItem;
  m_pCbMeth = (void (CDummy::*)()) pCbMeth;
}
#endif

int UsbSerial::_getc() { 
    NVIC_DisableIRQ(US_TICKER_TIMER_IRQn);
    NVIC_DisableIRQ(USB_IRQn);
    char c;
    c = *m_pInBufPos;
    m_pInBufPos++;
    NVIC_EnableIRQ(USB_IRQn);
    NVIC_EnableIRQ(US_TICKER_TIMER_IRQn);
    return c;
}

int UsbSerial::_putc(int c) { 
    NVIC_DisableIRQ(US_TICKER_TIMER_IRQn);
    NVIC_DisableIRQ(USB_IRQn);
    if( (m_pOutBufPos - m_outBufUsr) < BUF_LEN )
    {
      *m_pOutBufPos = (char) c;
      m_pOutBufPos++;
    }
    else
    {
      DBG("NO WAY!!!\n");
    }
    #if 1
    if( (m_pOutBufPos - m_outBufUsr) >= BUF_LEN ) //Must flush
    {
      if(m_timeout)
        m_txTimeout.detach();
      startTx();
    }
    else
    {
      /*if(m_timeout)
        m_txTimeout.detach();
      m_timeout = true;
      m_txTimeout.attach_us(this, &UsbSerial::startTx, FLUSH_TMOUT);*/
      if(!m_timeout)
      {
        m_timeout = true;
        m_txTimeout.attach_us(this, &UsbSerial::startTx, FLUSH_TMOUT);
      }
    }
    #endif
    //startTx();
    NVIC_EnableIRQ(USB_IRQn);
    NVIC_EnableIRQ(US_TICKER_TIMER_IRQn);
    return c;
}

int UsbSerial::readable() { 
    NVIC_DisableIRQ(US_TICKER_TIMER_IRQn);
    NVIC_DisableIRQ(USB_IRQn);
    int res;
    if( (m_pInBufPos - m_inBufUsr) < m_inBufLen )
    {
      //DBG("\r\nREADABLE\r\n");
      res = true;
    }
    else
    {
      //DBG("\r\nNOT READABLE\r\n");
      startRx(); //Try to swap packets & start another transmission
      res = ((m_pInBufPos - m_inBufUsr) < m_inBufLen )?true:false;
    }
    NVIC_EnableIRQ(USB_IRQn);
    NVIC_EnableIRQ(US_TICKER_TIMER_IRQn);
    return (bool)res;
}

int UsbSerial::writeable() { 
    NVIC_DisableIRQ(US_TICKER_TIMER_IRQn);
    NVIC_DisableIRQ(USB_IRQn);
  //  DBG("\r\nWRITEABLE???\r\n");
    int res = (bool)( (m_pOutBufPos - m_outBufUsr) < BUF_LEN);
    NVIC_EnableIRQ(USB_IRQn);
    NVIC_EnableIRQ(US_TICKER_TIMER_IRQn);
    return res;
}

void UsbSerial::onReadable()
{
  if(m_pInCbItem && m_pInCbMeth)
    (m_pInCbItem->*m_pInCbMeth)();
}

void UsbSerial::onWriteable()
{
  if(m_pOutCbItem && m_pOutCbMeth)
    (m_pOutCbItem->*m_pOutCbMeth)();
}

void UsbSerial::onEpInTransfer()
{
  int len = m_epIn.status();
  DBG("RX transfer completed w len=%d\n",len);
  startRx();
  if(len > 0)
    onReadable();
}

void UsbSerial::onEpOutTransfer()
{
  int len = m_epOut.status();
  DBG("TX transfer completed w len=%d\n",len);
  if(m_timeout)
    m_txTimeout.detach();
  startTx();
  if(len > 0)
    onWriteable();
}

void UsbSerial::startTx()
{
  
  DBG("Transfer>\n");
  
  m_timeout = false;
  
//  m_txTimeout.detach();
   
  if(!(m_pOutBufPos - m_outBufUsr))
  {
    DBG("?!?!?\n");
    return;
  }
  
  if( m_epOut.status() == USBERR_PROCESSING )
  {
    //Wait & retry
    //m_timeout = true;
    //m_txTimeout.attach_us(this, &UsbSerial::startTx, FLUSH_TMOUT);
    DBG("Ep is busy...\n");
    return;
  }
  
  if( m_epOut.status() < 0 )
  {
    DBG("Tx trying again...\n");
    m_epOut.transfer((volatile uint8_t*)m_outBufTrmt, m_outBufLen);
    return;
  }

  m_outBufLen = m_pOutBufPos - m_outBufUsr;
  
  //Swap buffers
  volatile char* swapBuf = m_outBufUsr;
  m_outBufUsr = m_outBufTrmt;
  m_outBufTrmt = swapBuf;
  
  m_epOut.transfer((volatile uint8_t*)m_outBufTrmt, m_outBufLen);
  
  m_pOutBufPos = m_outBufUsr;

}

void UsbSerial::startRx()
{
  if( (m_pInBufPos - m_inBufUsr) < m_inBufLen )
  {
    //User buf is not empty, cannot swap now...
    return;
  }
  int len = m_epIn.status();
  if( len == USBERR_PROCESSING )
  {
    //Previous transmission not completed
    return;
  }
  if( len < 0 )
  {
    DBG("Rx trying again...\n");
    m_epIn.transfer((volatile uint8_t*)m_inBufTrmt, BUF_LEN); //Start another transmission
    return;
  }
  
  m_inBufLen = len;
  
  //Swap buffers
  volatile char* swapBuf = m_inBufUsr;
  m_inBufUsr =  m_inBufTrmt;
  m_inBufTrmt = swapBuf;
  m_pInBufPos = m_inBufUsr;
  
  DBG("Starting new transfer\n");
  m_epIn.transfer((volatile uint8_t*)m_inBufTrmt, BUF_LEN); //Start another transmission
  
}

#ifdef MBED_RPC
const struct rpc_method *UsbSerial::get_rpc_methods() { 
    static const rpc_method methods[] = {
        { "readable", rpc_method_caller<int, UsbSerial, &UsbSerial::readable> },
        { "writeable", rpc_method_caller<int, UsbSerial, &UsbSerial::writeable> },
        RPC_METHOD_SUPER(Stream)
    };
    return methods;
}

struct rpc_class *UsbSerial::get_rpc_class() {
    static const rpc_function funcs[] = {
        /*{ "new", rpc_function_caller<const char*, UsbDevice*, int, int, const char*, Base::construct<UsbSerial,UsbDevice*,int,int,const char*> > },*/ //RPC is buggy
        RPC_METHOD_END
    };
    static rpc_class c = { "UsbSerial", funcs, NULL };
    return &c;
}
#endif

} // namespace mbed

#endif

