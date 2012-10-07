
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

#include "UsbHostMgr.h"

#include "usb_mem.h"

#include "string.h" //For memcpy, memmove, memset

#include "netCfg.h"
#if NET_USB

//#define __DEBUG
#include "dbg/dbg.h"

// bits of the USB/OTG clock control register
#define HOST_CLK_EN     (1<<0)
#define DEV_CLK_EN      (1<<1)
#define PORTSEL_CLK_EN  (1<<3)
#define AHB_CLK_EN      (1<<4)

// bits of the USB/OTG clock status register
#define HOST_CLK_ON     (1<<0)
#define DEV_CLK_ON      (1<<1)
#define PORTSEL_CLK_ON  (1<<3)
#define AHB_CLK_ON      (1<<4)

// we need host clock, OTG/portsel clock and AHB clock
#define CLOCK_MASK (HOST_CLK_EN | PORTSEL_CLK_EN | AHB_CLK_EN)

static UsbHostMgr* pMgr = NULL;

extern "C" void sUsbIrqhandler(void) __irq
{
  DBG("\n+Int\n");
  if(pMgr)
    pMgr->UsbIrqhandler();
  DBG("\n-Int\n");
  return;
}

UsbHostMgr::UsbHostMgr() : m_lpDevices()
{
  /*if(!pMgr)*/ //Assume singleton
    pMgr = this;
  usb_mem_init();
  memset(m_lpDevices, NULL, sizeof(UsbDevice*) * USB_HOSTMGR_MAX_DEVS);
  m_pHcca = (HCCA*) usb_get_hcca();
  memset((void*)m_pHcca, 0, 0x100);
  DBG("Host manager at %p\n", this);
}

UsbHostMgr::~UsbHostMgr()
{
  if(pMgr == this)
    pMgr = NULL;
}

UsbErr UsbHostMgr::init() //Initialize host
{
  NVIC_DisableIRQ(USB_IRQn);                           /* Disable the USB interrupt source           */
  
  LPC_SC->PCONP       &= ~(1UL<<31); //Cut power
  wait(1);
  
  
  // turn on power for USB
  LPC_SC->PCONP       |= (1UL<<31);
  // Enable USB host clock, port selection and AHB clock
  LPC_USB->USBClkCtrl |= CLOCK_MASK;
  // Wait for clocks to become available
  while ((LPC_USB->USBClkSt & CLOCK_MASK) != CLOCK_MASK)
      ;
  
  // it seems the bits[0:1] mean the following
  // 0: U1=device, U2=host
  // 1: U1=host, U2=host
  // 2: reserved
  // 3: U1=host, U2=device
  // NB: this register is only available if OTG clock (aka "port select") is enabled!!
  // since we don't care about port 2, set just bit 0 to 1 (U1=host)
  LPC_USB->OTGStCtrl |= 1;
  
  // now that we've configured the ports, we can turn off the portsel clock
  LPC_USB->USBClkCtrl &= ~PORTSEL_CLK_EN;
  
  // power pins are not connected on mbed, so we can skip them
  /* P1[18] = USB_UP_LED, 01 */
  /* P1[19] = /USB_PPWR,     10 */
  /* P1[22] = USB_PWRD, 10 */
  /* P1[27] = /USB_OVRCR, 10 */
  /*LPC_PINCON->PINSEL3 &= ~((3<<4) | (3<<6) | (3<<12) | (3<<22));  
  LPC_PINCON->PINSEL3 |=  ((1<<4)|(2<<6) | (2<<12) | (2<<22));   // 0x00802080
  */

  // configure USB D+/D- pins
  /* P0[29] = USB_D+, 01 */
  /* P0[30] = USB_D-, 01 */
  LPC_PINCON->PINSEL1 &= ~((3<<26) | (3<<28));  
  LPC_PINCON->PINSEL1 |=  ((1<<26)|(1<<28));     // 0x14000000
      
  DBG("Initializing Host Stack\n");
  
  wait_ms(100);                                   /* Wait 50 ms before apply reset              */
  LPC_USB->HcControl       = 0;                       /* HARDWARE RESET                             */
  LPC_USB->HcControlHeadED = 0;                       /* Initialize Control list head to Zero       */
  LPC_USB->HcBulkHeadED    = 0;                       /* Initialize Bulk list head to Zero          */
  
                                                      /* SOFTWARE RESET                             */
  LPC_USB->HcCommandStatus = OR_CMD_STATUS_HCR;
  LPC_USB->HcFmInterval    = DEFAULT_FMINTERVAL;      /* Write Fm Interval and Largest Data Packet Counter */

                                                      /* Put HC in operational state                */
  LPC_USB->HcControl  = (LPC_USB->HcControl & (~OR_CONTROL_HCFS)) | OR_CONTROL_HC_OPER;
  LPC_USB->HcRhStatus = OR_RH_STATUS_LPSC;            /* Set Global Power                           */
  
  LPC_USB->HcHCCA = (uint32_t)(m_pHcca);
  LPC_USB->HcInterruptStatus |= LPC_USB->HcInterruptStatus;                   /* Clear Interrrupt Status                    */


  LPC_USB->HcInterruptEnable  = OR_INTR_ENABLE_MIE |
                       OR_INTR_ENABLE_WDH |
                       OR_INTR_ENABLE_RHSC;

  NVIC_SetPriority(USB_IRQn, 0);       /* highest priority */
  /* Enable the USB Interrupt */
  NVIC_SetVector(USB_IRQn, (uint32_t)(sUsbIrqhandler));
  LPC_USB->HcRhPortStatus1 = OR_RH_PORT_CSC;
  LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRSC;
  
  /* Check for any connected devices */
  if (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_CCS)  //Root device connected
  {
    //Device connected
    wait(1);
    DBG("Device connected (%08x)\n", LPC_USB->HcRhPortStatus1);
    onUsbDeviceConnected(0, 1); //Hub 0 (root hub), Port 1 (count starts at 1)
  }
  
  DBG("Enabling IRQ\n");
  NVIC_EnableIRQ(USB_IRQn);
  DBG("End of host stack initialization\n");
  return USBERR_OK;
}

void UsbHostMgr::poll() //Enumerate connected devices, etc
{
  /* Check for any connected devices */
  if (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_CCS)  //Root device connected
  {
    //Device connected
    wait(1);
    DBG("Device connected (%08x)\n", LPC_USB->HcRhPortStatus1);
    onUsbDeviceConnected(0, 1); //Hub 0 (root hub), Port 1 (count starts at 1)
  }
  
  for(int i = 0; i < devicesCount(); i++)
  {
    if( (m_lpDevices[i]->m_connected)
    &&  !(m_lpDevices[i]->m_enumerated) )
    {
      m_lpDevices[i]->enumerate();
      return;
    }
  }
}

int UsbHostMgr::devicesCount()
{
  int i;
  for(i = 0; i < USB_HOSTMGR_MAX_DEVS; i++)
  {
    if (m_lpDevices[i] == NULL)
      break;
  }
  return i;
}

UsbDevice* UsbHostMgr::getDevice(int item)
{
  UsbDevice* pDev = m_lpDevices[item];
  if(!pDev)
    return NULL;
    
  pDev->m_refs++;
  return pDev;
}

void UsbHostMgr::releaseDevice(UsbDevice* pDev)
{
  pDev->m_refs--;
  if(pDev->m_refs > 0)
    return;
  //If refs count = 0, delete
  //Find & remove from list
  int i;
  for(i = 0; i < USB_HOSTMGR_MAX_DEVS; i++)
  {
    if (m_lpDevices[i] == pDev)
      break;
  }
  if(i!=USB_HOSTMGR_MAX_DEVS)
    memmove(&m_lpDevices[i], &m_lpDevices[i+1], sizeof(UsbDevice*) * (USB_HOSTMGR_MAX_DEVS - (i + 1))); //Safer than memcpy because of overlapping mem
  m_lpDevices[USB_HOSTMGR_MAX_DEVS - 1] = NULL;
  delete pDev;
}

void UsbHostMgr::UsbIrqhandler()
{
  uint32_t   int_status;
  uint32_t   ie_status;
  
  int_status    = LPC_USB->HcInterruptStatus;                          /* Read Interrupt Status                */
  ie_status     = LPC_USB->HcInterruptEnable;                          /* Read Interrupt enable status         */

  if (!(int_status & ie_status))
  {
    return;
  }
  else
  {
    int_status = int_status & ie_status;
    if (int_status & OR_INTR_STATUS_RHSC) /* Root hub status change interrupt     */
    {
      DBG("LPC_USB->HcRhPortStatus1 = %08x\n", LPC_USB->HcRhPortStatus1);
      if (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_CSC)
      {
        if (LPC_USB->HcRhStatus & OR_RH_STATUS_DRWE)
        {
            /*
             * When DRWE is on, Connect Status Change
             * means a remote wakeup event.
            */
            //HOST_RhscIntr = 1;// JUST SOMETHING FOR A BREAKPOINT
        }
        else
        {
          /*
           * When DRWE is off, Connect Status Change
           * is NOT a remote wakeup event
          */
          if (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_CCS)  //Root device connected
          {
            //Device connected
            DBG("Device connected (%08x)\n", LPC_USB->HcRhPortStatus1);
            onUsbDeviceConnected(0, 1); //Hub 0 (root hub), Port 1 (count starts at 1)
          }
          else //Root device disconnected
          {
            //Device disconnected
            DBG("Device disconnected\n");
            onUsbDeviceDisconnected(0, 1);
          }
          //TODO: HUBS
        }
        LPC_USB->HcRhPortStatus1 = OR_RH_PORT_CSC;
      }
      if (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_PRSC)
      {
        LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRSC;
      }
    }  
    if (int_status & OR_INTR_STATUS_WDH) /* Writeback Done Head interrupt        */
    {                  
      //UsbEndpoint::sOnCompletion((LPC_USB->HccaDoneHead) & 0xFE);
      if(m_pHcca->DoneHead)
      {
        UsbEndpoint::sOnCompletion(m_pHcca->DoneHead);
        m_pHcca->DoneHead = 0;
        LPC_USB->HcInterruptStatus = OR_INTR_STATUS_WDH;
        if(m_pHcca->DoneHead)
          DBG("??????????????????????????????\n\n\n");
      }
      else
      {
        //Probably an error
        int_status = LPC_USB->HcInterruptStatus;
        DBG("HcInterruptStatus = %08x\n", int_status);
        if (int_status & OR_INTR_STATUS_UE) //Unrecoverable error, disconnect devices and resume
        {
          onUsbDeviceDisconnected(0, 1);
          LPC_USB->HcInterruptStatus = OR_INTR_STATUS_UE;
          LPC_USB->HcCommandStatus = 0x01; //Host Controller Reset
        }
      }
    }
    LPC_USB->HcInterruptStatus = int_status; /* Clear interrupt status register      */
  }
  return;
}

void UsbHostMgr::onUsbDeviceConnected(int hub, int port)
{
  int item = devicesCount();
  if( item == USB_HOSTMGR_MAX_DEVS )
    return; //List full...
  //Find a free address (not optimized, but not really important)
  int i;
  int addr = 1;
  for(i = 0; i < item; i++)
  {
    addr = MAX( addr, m_lpDevices[i]->m_addr + 1 );
  }
  m_lpDevices[item] = new UsbDevice( this, hub, port, addr );
  m_lpDevices[item]->m_connected = true;
}

void UsbHostMgr::onUsbDeviceDisconnected(int hub, int port)
{
  for(int i = 0; i < devicesCount(); i++)
  {
     if( (m_lpDevices[i]->m_hub == hub)
     &&  (m_lpDevices[i]->m_port == port) )
     {
       m_lpDevices[i]->m_connected = false;
       if(!m_lpDevices[i]->m_enumerated)
       {
         delete m_lpDevices[i];
         m_lpDevices[i] = NULL;
       }
       return;
     }
  }
}

void UsbHostMgr::resetPort(int hub, int port)
{
  DBG("Resetting hub %d, port %d\n", hub, port);
  if(hub == 0) //Root hub
  {
    wait_ms(100); /* USB 2.0 spec says at least 50ms delay before port reset */
    LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRS; // Initiate port reset
    DBG("Before loop\n");
    while (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_PRS)
      ;
    LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRSC; // ...and clear port reset signal
    DBG("After loop\n");
    wait_ms(200); /* Wait for 100 MS after port reset  */
  }
  else
  {
    //TODO: Hubs
  }
  DBG("Port reset OK\n");
}

#endif
