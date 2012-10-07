
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

#include "UsbDevice.h"

#include "netCfg.h"
#if NET_USB

//#define __DEBUG
#include "dbg/dbg.h"

UsbDevice::UsbDevice( UsbHostMgr* pMgr, int hub, int port, int addr ) : m_pControlEp(NULL), /*m_controlEp( this, 0x00, false, USB_CONTROL, 8 ),*/
m_pMgr(pMgr), m_connected(false), m_enumerated(false), m_hub(hub), m_port(port), m_addr(addr), m_refs(0),
m_vid(0), m_pid(0)
{

}

UsbDevice::~UsbDevice()
{
  if(m_pControlEp)
    delete m_pControlEp;
}

UsbErr UsbDevice::enumerate()
{
 // USB_INT32S  rc;
 
  UsbErr rc;

  DBG("Starting enumeration (m_pMgr = %p)\n", m_pMgr);
  
#if 1
  m_pMgr->resetPort(m_hub, m_port);
#else 
  wait_ms(100);                             /* USB 2.0 spec says atleast 50ms delay beore port reset */
  LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRS; // Initiate port reset
  while (LPC_USB->HcRhPortStatus1 & OR_RH_PORT_PRS)
    __WFI(); // Wait for port reset to complete...
  LPC_USB->HcRhPortStatus1 = OR_RH_PORT_PRSC; // ...and clear port reset signal
  wait_ms(200); /* Wait for 100 MS after port reset  */
#endif

  DBG("Port reset\n");
  
  wait_ms(200);
   
  m_pControlEp = new UsbEndpoint( this, 0x00, false, USB_CONTROL, 8, 0 );
  
  //EDCtrl->Control = 8 << 16;/* Put max pkt size = 8              */
  /* Read first 8 bytes of device desc */
  rc = controlReceive(USB_DEVICE_TO_HOST | USB_RECIPIENT_DEVICE, GET_DESCRIPTOR, (USB_DESCRIPTOR_TYPE_DEVICE << 8)|(0), 0, m_controlDataBuf, 8);
  if (rc)
  {
    DBG("RC=%d",rc);
    return (rc);
  }
  
  DBG("Got descriptor, max ep size is %d\n", m_controlDataBuf[7]);
  
  m_pControlEp->updateSize(m_controlDataBuf[7]); /* Get max pkt size of endpoint 0    */
  rc = controlSend(USB_HOST_TO_DEVICE | USB_RECIPIENT_DEVICE, SET_ADDRESS, m_addr, 0, NULL, 0); /* Set the device address to m_addr       */
  if (rc)
  {
  //  PRINT_Err(rc);
    return (rc);
  }
  wait_ms(2);
  //EDCtrl->Control = (EDCtrl->Control) | 1; /* Modify control pipe with address 1 */
  
  //Update address
  m_pControlEp->updateAddr(m_addr);
  DBG("Ep addr is now %d", m_addr);
  /**/
  
  //rc = HOST_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_DEVICE, 0, TDBuffer, 17); //Read full device descriptor
  rc = controlReceive(USB_DEVICE_TO_HOST | USB_RECIPIENT_DEVICE, GET_DESCRIPTOR, (USB_DESCRIPTOR_TYPE_DEVICE << 8)|(0), 0, m_controlDataBuf, 17);
  if (rc)
  {
    //PRINT_Err(rc);
    return (rc);
  }
  /*
  rc = SerialCheckVidPid();
  if (rc != OK) {
    PRINT_Err(rc);
    return (rc);
  }
  */
  /**/
  
  m_vid = *((uint16_t*)&m_controlDataBuf[8]);
  m_pid = *((uint16_t*)&m_controlDataBuf[10]);
  
  DBG("VID: %02x, PID: %02x\n", m_vid, m_pid);
  /* Get the configuration descriptor   */
  //rc = HOST_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, TDBuffer, 9);
  rc = controlReceive(USB_DEVICE_TO_HOST | USB_RECIPIENT_DEVICE, GET_DESCRIPTOR, (USB_DESCRIPTOR_TYPE_CONFIGURATION << 8)|(0), 0, m_controlDataBuf, 9);
  if (rc)
  {
    //PRINT_Err(rc);
    return (rc);
  }
  /* Get the first configuration data  */
  //rc = HOST_GET_DESCRIPTOR(USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, TDBuffer, *((uint16_t*)&TDBuffer[2]));
  rc = controlReceive(USB_DEVICE_TO_HOST | USB_RECIPIENT_DEVICE, GET_DESCRIPTOR, (USB_DESCRIPTOR_TYPE_CONFIGURATION << 8)|(0), 0, m_controlDataBuf, *((uint16_t*)&m_controlDataBuf[2]));
  if (rc)
  {
    //PRINT_Err(rc);
    return (rc);
  }
  
  DBG("Desc len is %d\n", *((uint16_t*)&m_controlDataBuf[2]));
  
  DBG("Set configuration\n");
  
  //rc = USBH_SET_CONFIGURATION(1);/* Select device configuration 1     */
  rc = controlSend(USB_HOST_TO_DEVICE | USB_RECIPIENT_DEVICE, SET_CONFIGURATION, 1, 0, NULL, 0);
  if (rc)
  {
   // PRINT_Err(rc);
   return rc;
  }
  wait_ms(100);/* Some devices may require this delay */
  
  m_enumerated = true;
  return USBERR_OK;
}

bool UsbDevice::connected()
{
  return m_connected;
}

bool UsbDevice::enumerated()
{
  return m_enumerated;
}

int UsbDevice::getPid()
{
  return m_pid;
}

int UsbDevice::getVid()
{
  return m_vid;
}

UsbErr UsbDevice::getConfigurationDescriptor(int config, uint8_t** pBuf)
{
  //For now olny one config
  *pBuf = m_controlDataBuf;
  return USBERR_OK;
}

UsbErr UsbDevice::getInterfaceDescriptor(int config, int item, uint8_t** pBuf)
{
  byte* desc_ptr = m_controlDataBuf;

/*  if (desc_ptr[1] != USB_DESCRIPTOR_TYPE_CONFIGURATION)
  {    
    return USBERR_BADCONFIG;
  }*/
  
  if(item>=m_controlDataBuf[4])//Interfaces count
    return USBERR_NOTFOUND;
  
  desc_ptr += desc_ptr[0];
  
  *pBuf = NULL;
  
  while (desc_ptr < m_controlDataBuf + *((uint16_t*)&m_controlDataBuf[2]))
  {

    switch (desc_ptr[1]) {
      case USB_DESCRIPTOR_TYPE_INTERFACE: 
        if(desc_ptr[2] == item)
        {
          *pBuf = desc_ptr;
          return USBERR_OK;
        }
        desc_ptr += desc_ptr[0]; // Move to next descriptor start
        break;
    }
      
  }
  
  if(*pBuf == NULL)
    return USBERR_NOTFOUND;
    
  return USBERR_OK;
}


UsbErr UsbDevice::setConfiguration(int config)
{
  return USBERR_OK;
}

UsbErr UsbDevice::controlSend(byte requestType, byte request, word value, word index, const byte* buf, int len)
{
  UsbErr rc;
  fillControlBuf(requestType, request, value, index, len);
  m_pControlEp->setNextToken(TD_SETUP);
  rc = m_pControlEp->transfer(m_controlBuf, 8);
  while(m_pControlEp->status() == USBERR_PROCESSING);
  rc = (UsbErr) MIN(0, m_pControlEp->status());
  if(rc)
    return rc;
  if(len)
  {
    m_pControlEp->setNextToken(TD_OUT);
    rc = m_pControlEp->transfer((byte*)buf, len);
    while(m_pControlEp->status() == USBERR_PROCESSING);
    rc = (UsbErr) MIN(0, m_pControlEp->status());
    if(rc)
      return rc;
  }
  m_pControlEp->setNextToken(TD_IN);
  rc = m_pControlEp->transfer(NULL, 0);
  while(m_pControlEp->status() == USBERR_PROCESSING);
  rc = (UsbErr) MIN(0, m_pControlEp->status());
  if(rc)
    return rc;
  return USBERR_OK;
}

UsbErr UsbDevice::controlReceive(byte requestType, byte request, word value, word index, const byte* buf, int len)
{
  UsbErr rc;
  fillControlBuf(requestType, request, value, index, len);
  m_pControlEp->setNextToken(TD_SETUP);
  rc = m_pControlEp->transfer(m_controlBuf, 8);
  while(m_pControlEp->status() == USBERR_PROCESSING);
  rc = (UsbErr) MIN(0, m_pControlEp->status());
  if(rc)
    return rc;
  if(len)
  {
    m_pControlEp->setNextToken(TD_IN);
    rc = m_pControlEp->transfer( (byte*) buf, len);
    while(m_pControlEp->status() == USBERR_PROCESSING);
    rc = (UsbErr) MIN(0, m_pControlEp->status());
    if(rc)
      return rc;
  }
  m_pControlEp->setNextToken(TD_OUT);
  rc = m_pControlEp->transfer(NULL, 0);
  while(m_pControlEp->status() == USBERR_PROCESSING);
  rc = (UsbErr) MIN(0, m_pControlEp->status());
  if(rc)
    return rc;
  return USBERR_OK;
}

void UsbDevice::fillControlBuf(byte requestType, byte request, word value, word index, int len)
{
#ifdef __BIG_ENDIAN
  #error "Must implement BE to LE conv here"
#endif
  m_controlBuf[0] = requestType;
  m_controlBuf[1] = request;
  //We are in LE so it's fine
  *((word*)&m_controlBuf[2]) = value;
  *((word*)&m_controlBuf[4]) = index;
  *((word*)&m_controlBuf[6]) = (word) len;
}


#endif
