
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

#include "UsbEndpoint.h"

#include "UsbDevice.h"

#include "usb_mem.h"

#include "netCfg.h"
#if NET_USB

//#define __DEBUG
#include "dbg/dbg.h"

UsbEndpoint::UsbEndpoint( UsbDevice* pDevice, uint8_t ep, bool dir, UsbEndpointType type, uint16_t size, int addr /*= -1*/ ) : m_pDevice(pDevice), m_result(true), m_status((int)USBERR_OK), m_len(0), m_pBufStartPtr(NULL),
m_pCbItem(NULL), m_pCbMeth(NULL), m_pNextEp(NULL)
{
  //Insert into Eps list
  //FIXME: Assert that no USB interrupt is triggered meanwhile
  if(m_pHeadEp)
  {
    m_pNextEp = m_pHeadEp;
    m_pHeadEp = this;
  }
  else
  {
    m_pNextEp = NULL;
    m_pHeadEp = this;
  }  

  m_pEd = (volatile HCED*)usb_get_ed();
  
  m_pTdHead = (volatile HCTD*)usb_get_td();
  m_pTdTail = (volatile HCTD*)usb_get_td();
  
  //printf("\r\n--m_pEd = %p--\r\n", m_pEd);
  DBG("m_pEd = %p\n", m_pEd);
  DBG("m_pTdHead = %p\n", m_pTdHead);
  DBG("m_pTdTail = %p\n", m_pTdTail);
  
  //Init Ed & Td
  //printf("\r\n--Ep Init--\r\n");
  memset((void*)m_pEd, 0, sizeof(HCED));
  //printf("\r\n--Td Init--\r\n");

  memset((void*)m_pTdHead, 0, sizeof(HCTD));
  memset((void*)m_pTdTail, 0, sizeof(HCTD));
  
  if(addr == -1)
    addr = pDevice->m_addr;
  
  //Setup Ed
  //printf("\r\n--Ep Setup--\r\n");
  m_pEd->Control =  addr        |      /* USB address           */
  ((ep & 0x7F) << 7)            |      /* Endpoint address      */
  (type!=USB_CONTROL?((dir?2:1) << 11):0)             |      /* direction : Out = 1, 2 = In */
  (size << 16);     /* MaxPkt Size           */
  
  m_dir = dir;
  m_setup = false;
  m_type = type;
  
  m_pEd->TailTd = m_pEd->HeadTd = (uint32_t) m_pTdTail; //Empty TD list
  
  DBG("Before link\n");
  
  //printf("\r\n--Ep Reg--\r\n");
  //Append Ed to Ed list
  volatile HCED* prevEd;
  switch( m_type )
  {
  case USB_CONTROL:
    prevEd = (volatile HCED*) LPC_USB->HcControlHeadED;
    break;
  case USB_BULK:
  default:
    prevEd = (volatile HCED*) LPC_USB->HcBulkHeadED;
    break;
  }
  
  DBG("prevEd is %p\n", prevEd);
  
  if(prevEd)
  {
    DBG("prevEd set\n")
  
    while(prevEd->Next)
    {
      DBG("prevEd->Next = %08x\n", prevEd->Next);
      prevEd = (volatile HCED*) prevEd->Next;
    }
    prevEd->Next = (uint32_t) m_pEd;
  
  }
  else
  {
    switch( m_type )
    {
    case USB_CONTROL:
      LPC_USB->HcControlHeadED = (uint32_t) m_pEd;
      break;
    case USB_BULK:
    default:
      LPC_USB->HcBulkHeadED = (uint32_t) m_pEd;
      break;
    }
  }
  
  DBG("Ep init\n");
}

UsbEndpoint::~UsbEndpoint()
{
  //Remove from Eps list
  //FIXME: Assert that no USB interrupt is triggered meanwhile
  if(m_pHeadEp != this)
  {
    UsbEndpoint* prevEp = m_pHeadEp;
    while(prevEp->m_pNextEp != this)
      prevEp = prevEp->m_pNextEp;
    prevEp->m_pNextEp = m_pNextEp;
  }
  else
  {
    m_pHeadEp = m_pNextEp;
  }  
  
  m_pEd->Control |= ED_SKIP; //Skip this Ep in queue

  //Remove from queue
  volatile HCED* prevEd;
  switch( m_type )
  {
  case USB_CONTROL:
    prevEd = (volatile HCED*) LPC_USB->HcControlHeadED;
    break;
  case USB_BULK:
  default:
    prevEd = (volatile HCED*) LPC_USB->HcBulkHeadED;
    break;
  }
  if( m_pEd == prevEd )
  {
    switch( m_type )
    {
    case USB_CONTROL:
      LPC_USB->HcControlHeadED = m_pEd->Next;
      break;
    case USB_BULK:
    default:
      LPC_USB->HcBulkHeadED = m_pEd->Next;
      break;
    }
    LPC_USB->HcBulkHeadED = m_pEd->Next;
  }
  else
  {
    while( prevEd->Next != (uint32_t) m_pEd )
    {
      prevEd = (volatile HCED*) prevEd->Next;
    }
    prevEd->Next = m_pEd->Next;
  }
  
  //
  usb_free_ed((volatile byte*)m_pEd);
  
  usb_free_td((volatile byte*)m_pTdHead);
  usb_free_td((volatile byte*)m_pTdTail);
}

void UsbEndpoint::setNextToken(uint32_t token) //Only for control Eps
{
  switch(token)
  {
    case TD_SETUP:
      m_dir = false;
      m_setup = true;
      break;
    case TD_IN:
      m_dir = true;
      m_setup = false;
      break;
    case TD_OUT:
      m_dir = false;
      m_setup = false;
      break;
  }
}

UsbErr UsbEndpoint::transfer(volatile uint8_t* buf, uint32_t len)
{
  if(!m_result)
    return USBERR_BUSY; //The previous trasnfer is not completed 
    //FIXME: We should be able to queue the next transfer, still needs to be implemented
  
  if( !m_pDevice->connected() )
    return USBERR_DISCONNECTED;
  
  m_result = false;
  
  volatile uint32_t token = (m_setup?TD_SETUP:(m_dir?TD_IN:TD_OUT));

  volatile uint32_t td_toggle;
  if (m_type == USB_CONTROL)
  {
    if (m_setup)
    {
      td_toggle = TD_TOGGLE_0;
    }
    else
    {
      td_toggle = TD_TOGGLE_1;
    }
  }
  else
  {
    td_toggle = 0;
  }

  //Swap Tds
  volatile HCTD* pTdSwap;
  pTdSwap = m_pTdTail;
  m_pTdTail = m_pTdHead;
  m_pTdHead = pTdSwap;

  m_pTdHead->Control = (TD_ROUNDING    |
                       token           |
                       TD_DELAY_INT(0) |//7
                       td_toggle       |
                       TD_CC);

  m_pTdTail->Control = 0;
  m_pTdHead->CurrBufPtr   = (uint32_t) buf;
  m_pBufStartPtr = buf;
  m_pTdTail->CurrBufPtr   = 0;
  m_pTdHead->Next         = (uint32_t) m_pTdTail;
  m_pTdTail->Next         = 0;
  m_pTdHead->BufEnd       = (uint32_t)(buf + (len - 1));
  m_pTdTail->BufEnd       = 0;
  
  m_pEd->HeadTd  = (uint32_t)m_pTdHead | ((m_pEd->HeadTd) & 0x00000002); //Carry bit
  m_pEd->TailTd  = (uint32_t)m_pTdTail;
  
  //DBG("m_pEd->HeadTd = %08x\n", m_pEd->HeadTd);

  if(m_type == USB_CONTROL)
  {
    LPC_USB->HcCommandStatus = LPC_USB->HcCommandStatus | OR_CMD_STATUS_CLF;
    LPC_USB->HcControl       = LPC_USB->HcControl       | OR_CONTROL_CLE; //Enable control list
  }
  else //USB_BULK
  {
    LPC_USB->HcCommandStatus = LPC_USB->HcCommandStatus | OR_CMD_STATUS_BLF;
    LPC_USB->HcControl       = LPC_USB->HcControl       | OR_CONTROL_BLE; //Enable bulk list
  }
  
  //m_done = false;
  m_len = len;

  return USBERR_PROCESSING;
 
}
  
int UsbEndpoint::status()
{
  if( !m_pDevice->connected() )
  {
    if(!m_result)
      onCompletion();
    m_result = true;
    return (int)USBERR_DISCONNECTED;
  }
  else if( !m_result )
  { 
    return (int)USBERR_PROCESSING;
  }
  /*else if( m_done )
  {
    return (int)USBERR_OK;
  }*/
  else
  {
    return m_status;
  }
}

void UsbEndpoint::updateAddr(int addr)
{
  DBG("m_pEd->Control = %08x\n", m_pEd->Control);
  m_pEd->Control &= ~0x7F;
  m_pEd->Control |= addr;
  DBG("m_pEd->Control = %08x\n", m_pEd->Control);
}

void UsbEndpoint::updateSize(uint16_t size)
{
  DBG("m_pEd->Control = %08x\n", m_pEd->Control);
  m_pEd->Control &= ~0x3FF0000;
  m_pEd->Control |= (size << 16);
  DBG("m_pEd->Control = %08x\n", m_pEd->Control);
}

#if 0 //For doc only
template <class T>
void UsbEndpoint::setOnCompletion( T* pCbItem, void (T::*pCbMeth)() )
{
  m_pCbItem = (CDummy*) pCbItem;
  m_pCbMeth = (void (CDummy::*)()) pCbMeth;
}
#endif

void UsbEndpoint::onCompletion()
{
  //DBG("Transfer completed\n");
  if( m_pTdHead->Control >> 28  )
  {
    DBG("TD Failed with condition code %01x\n", m_pTdHead->Control >> 28 );
    m_status = (int)USBERR_TDFAIL;
  }
  else if( m_pEd->HeadTd & 0x1 )
  {
    m_pEd->HeadTd = m_pEd->HeadTd & ~0x1;
    DBG("\r\nHALTED!!\r\n");
    m_status = (int)USBERR_HALTED;
  }
  else if( (m_pEd->HeadTd & ~0xF) == (uint32_t) m_pTdTail )
  {
    //Done
    int len;
    //DBG("m_pTdHead->CurrBufPtr = %08x, m_pBufStartPtr=%08x\n", m_pTdHead->CurrBufPtr, (uint32_t) m_pBufStartPtr);
    if(m_pTdHead->CurrBufPtr)
      len = m_pTdHead->CurrBufPtr - (uint32_t) m_pBufStartPtr;
    else
      len = m_len;
    /*if(len == 0) //Packet transfered completely
      len = m_len;*/
    //m_done = true;
    DBG("Transfered %d bytes\n", len);
    m_status = len; 
  }
  else
  {
    DBG("Unknown error...\n");
    m_status = (int)USBERR_ERROR;
  }
  m_result = true;
  if(m_pCbItem && m_pCbMeth)
    (m_pCbItem->*m_pCbMeth)();
}

void UsbEndpoint::sOnCompletion(uint32_t pTd)
{
  if(!m_pHeadEp)
      return;
  do
  {
    //DBG("sOnCompletion (pTd = %08x)\n", pTd);
    UsbEndpoint* pEp = m_pHeadEp;  
    do
    {
      if((uint32_t)pEp->m_pTdHead == pTd)
      {
        pEp->onCompletion();
        break;
      }
    } while(pEp = pEp->m_pNextEp);
  } while( pTd = (uint32_t)( ((HCTD*)pTd)->Next ) ); //Go around the Done queue
}

UsbEndpoint* UsbEndpoint::m_pHeadEp = NULL;

#endif
