
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

#include "UMTSStick.h"

#define __DEBUG
#include "dbg/dbg.h"

UMTSStick::UMTSStick() : m_host(), m_pDev(NULL)
{

}

UMTSStick::~UMTSStick()
{

}


UMTSStickErr UMTSStick::getSerial(UsbSerial** ppUsbSerial)
{
  m_host.init();
  
  UMTSStickErr rc;
  
  rc = waitForDevice();
  if(rc)
    return rc;
   
  //Device is now enumerated, read table
  
  uint16_t vid = m_pDev->getVid();
  uint16_t pid = m_pDev->getPid();
  
  DBG("Configuration set: vid:%04x pid:%04x\n", vid, pid);
    
  bool handled = false;
  bool cdfs = false;
  const UMTSSwitchingInfo* pInfo;
  for(int i = 0; i < UMTS_SWITCHING_COUNT; i++)
  {
    pInfo = &UMTSwitchingTable[i];
    if( !checkDeviceState(pInfo, &cdfs) )
    {
      handled = true;
      break;
    }
    
  } //for(int i = 0; i < UMTS_SWITCHING_COUNT; i++)
  
  if(!handled)
  {
    DBG("Don't know this device!\n");
    return UMTSERR_NOTIMPLEMENTED;
  }
  
  //Check if the device is in CDFS mode, in this case switch
  if(cdfs)
  {
    DBG("Switching the device by sending a magic packet\n");
  
    rc = switchMode(pInfo);
    if(rc)
      return rc;
      
    DBG("Now wait for device to reconnect\n");
    
    m_host.releaseDevice(m_pDev);
      
    //Wait for device to reconnect
    wait(3);
    rc = waitForDevice();
    if(rc)
      return rc;
  }
  
  rc = findSerial(ppUsbSerial);
  if(rc)
    return rc;
  
  return UMTSERR_OK;
}

UMTSStickErr UMTSStick::waitForDevice()
{
  bool ready = false;
  while(!ready)
  {
    while(!m_host.devicesCount())
    {}
    wait(1);
    if(m_host.devicesCount())
      ready = true;
  }
  
  wait(2); //Wait for device to be initialized
  
  if(!m_host.devicesCount())
    return UMTSERR_DISCONNECTED;
  
  m_pDev = m_host.getDevice(0);
  
  while(!m_pDev->enumerated())
  {
    m_host.poll();
    if(!m_host.devicesCount())
      return UMTSERR_DISCONNECTED;
  }
   
  return UMTSERR_OK;
}

UMTSStickErr UMTSStick::checkDeviceState(const UMTSSwitchingInfo* pInfo, bool* pCdfs)
{
  uint16_t vid = m_pDev->getVid();
  uint16_t pid = m_pDev->getPid();
  bool handled = false;
  if( (vid == pInfo->cdfsVid) && (pid == pInfo->cdfsPid) )
  {
    DBG("Match on dongles list\n");
    if( !pInfo->targetClass ) //No specific interface to check, vid/pid couple is specific to CDFS mode
    {
      DBG("Found device in CDFS mode\n");
      handled = true;
      *pCdfs = true;
    }
    else //if( !pInfo->targetClass )
    {
      //Has to check if there is an interface of class targetClass
      byte* desc = NULL;
      int c = 0;
             
      while( !m_pDev->getInterfaceDescriptor(1, c++, &desc) )
      {
        if( desc[5] == pInfo->targetClass )
        {
          DBG("Found device in Serial mode\n");
          handled = true;
          *pCdfs = false;
          break;
        }
      }        
      
      if(!handled)
      {
        //All interfaces were tried, so we are in CDFS mode
        DBG("Found device in CDFS mode\n");
        handled = true;
        *pCdfs = true;
      }
    } //if( !pInfo->targetClass )
  } //if( (vid == pInfo->cdfsVid) && (pid == pInfo->cdfsPid) )
  else
  {
    //Try every vid/pid couple of the serial list
    for( int i = 0; i < 16 ; i++)
    {
      if(!pInfo->serialPidList[i])
        break;
      if( (pInfo->serialVid == vid) && (pInfo->serialPidList[i] == pid) )
      {
        DBG("Found device in Serial mode\n");
        handled = true;
        *pCdfs = false;
        break;
      }
    }
  } //if( (vid == pInfo->cdfsVid) && (pid == pInfo->cdfsPid) )
  
  if(!handled)
    return UMTSERR_NOTFOUND;
    
  return UMTSERR_OK;
}

UMTSStickErr UMTSStick::switchMode(const UMTSSwitchingInfo* pInfo)
{
  if(!pInfo->huaweiPacket) //Send SCSI packet on first bulk ep
  {
    //Find first bulk ep           
    byte* desc = NULL;
    int c = 0;
    
    UsbEndpoint *pEpOut = NULL;
       
    while( !m_pDev->getInterfaceDescriptor(1, c++, &desc) )
    {
      byte* p = desc;
      int epNum = 0;
      p = p + p[0]; //Move to next descriptor (which should be an ep descriptor)
      while (epNum < desc[4]) //Eps count in this if
      {
        if (p[1] != USB_DESCRIPTOR_TYPE_ENDPOINT)
          break;
        
        if( (p[3] == 0x02) && !(p[2] & 0x80) ) //Bulk endpoint, out
        {
          DBG("Found bulk ep %02x\n", p[2]);
          pEpOut = new UsbEndpoint( m_pDev, p[2], false, USB_BULK, *((uint16_t*)&p[4]) );
          break;
        }
        
        p = p + p[0]; //Move to next ep desc
        epNum++;
      }
      if(pEpOut)
        break;
    }        
    
    if(!pEpOut)
      return UMTSERR_NOTFOUND;
      
    //Send SCSI packet
    
    DBG("Sending SCSI Packet to switch\n");
    byte ramCdfsBuf[31];
    memcpy(ramCdfsBuf, pInfo->cdfsPacket, 31);
    pEpOut->transfer((volatile byte*)ramCdfsBuf, 31);
    while(pEpOut->status() == USBERR_PROCESSING);
    int ret = pEpOut->status();
    if((ret < 0) && (ret !=USBERR_DISCONNECTED)) //Packet was not transfered
    {
      DBG("Usb error %d\n", ret);
      delete pEpOut;
      return UMTSERR_USBERR;
    }
    
    delete pEpOut;
  }  
  else
  {
    UsbErr usbErr;
    //Send the Huawei-specific control packet
    usbErr = m_pDev->controlSend(0, 0x03, 1, 0, NULL, 0);
    if(usbErr && (usbErr != USBERR_DISCONNECTED))
      return UMTSERR_USBERR;  
  }
  
  DBG("The stick should be switching in serial mode now\n");
  
  return UMTSERR_OK;
}

UMTSStickErr UMTSStick::findSerial(UsbSerial** ppUsbSerial)
{
  byte* desc = NULL;
  int c = 0;

  int epOut = 0;
  int epIn = 0;
      
  while( !m_pDev->getInterfaceDescriptor(1, c++, &desc) )
  {
    byte* p = desc;
    int epNum = 0;
    
    DBG("Interface of type %02x\n", desc[5]);
    
    if(desc[5] != 0xFF) //Not a serial-like if
      continue;
    
    p = p + p[0]; //Move to next descriptor (which should be an ep descriptor)
    while (epNum < desc[4]) //Eps count in this if
    {
      if (p[1] == USB_DESCRIPTOR_TYPE_ENDPOINT)
      {
        if( (p[3] == 0x02) && !(p[2] & 0x80) && !epOut ) //Bulk endpoint, out
        {
          DBG("Found bulk out ep %02x of payload size %04x\n", p[2], *((uint16_t*)&p[4]));
          epOut = p[2] & 0x7F;
        }
      
        if( (p[3] == 0x02) && (p[2] & 0x80) && !epIn ) //Bulk endpoint, in
        {
          DBG("Found bulk in ep %02x of payload size %04x\n", p[2], *((uint16_t*)&p[4]));
          epIn = p[2] & 0x7F;
        }
       
        if(epOut && epIn)
          break;
      }
      
      p = p + p[0]; //Move to next ep desc
      epNum++;
    }
    
    if(epOut && epIn)
      break;
  }
  
  if(!epOut || !epIn)
    return UMTSERR_NOTFOUND;
    
  DBG("Endpoints found, create serial object\n");
    
  *ppUsbSerial = new UsbSerial(m_pDev, epIn, epOut);
  
  DBG("UsbSerial object created\n");
    
  return UMTSERR_OK;
}

#endif
