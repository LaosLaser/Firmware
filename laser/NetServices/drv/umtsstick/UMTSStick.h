
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
UMTS Stick driver header file
*/

#ifndef UMTS_STICK_H
#define UMTS_STICK_H

#include "mbed.h"

#include "drv/usb/UsbHostMgr.h"
#include "drv/usb/UsbDevice.h"
#include "drv/usb/UsbEndpoint.h"

#include "drv/serial/usb/UsbSerial.h"

#define UMTS_SWITCHING_COUNT 2

typedef unsigned char byte;

struct UMTSSwitchingInfo
{
  uint16_t cdfsVid;
  uint16_t cdfsPid;
  uint16_t serialVid;
  uint16_t serialPidList[16];
  byte targetClass;
  bool huaweiPacket;
  byte cdfsPacket[31];
};

extern const UMTSSwitchingInfo UMTSwitchingTable[UMTS_SWITCHING_COUNT];

///UMTS Stick error codes
enum UMTSStickErr
{
  __UMTSERR_MIN = -0xFFFF,
  UMTSERR_NOTFOUND, ///<Stick was not found
  UMTSERR_NOTIMPLEMENTED, ///<This model is not implemented
  UMTSERR_USBERR, ///<USB Error
  UMTSERR_DISCONNECTED, ///<Stick disconnected
  UMTSERR_OK = 0 ///<Success
};

class UMTSStick
{
public:
  UMTSStick();
  ~UMTSStick();
  
  UMTSStickErr getSerial(UsbSerial** ppUsbSerial);
  
private:
  UMTSStickErr waitForDevice();
  UMTSStickErr checkDeviceState(const UMTSSwitchingInfo* pInfo, bool* pCdfs);

  UMTSStickErr switchMode(const UMTSSwitchingInfo* pInfo);
  UMTSStickErr findSerial(UsbSerial** ppUsbSerial);
  
  
  UsbHostMgr m_host;
  UsbDevice* m_pDev;
};

#endif
