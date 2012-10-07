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

//Assigns addresses to connected devices...

#ifndef USB_HOSTMGR_H
#define USB_HOSTMGR_H

#include "mbed.h"
#include "UsbInc.h"
#include "UsbDevice.h"

#define USB_HOSTMGR_MAX_DEVS 2 //2 devices max for now... will be more when hubs are supported

class UsbDevice;

class UsbHostMgr //[0-1] inst
{
public:
  UsbHostMgr();
  ~UsbHostMgr();
  
  UsbErr init(); //Initialize host
  
  void poll(); //Enumerate connected devices, etc
  
  int devicesCount();
  
  UsbDevice* getDevice(int item);
  void releaseDevice(UsbDevice* pDev);
  

  void UsbIrqhandler();

protected:  
  void onUsbDeviceConnected(int hub, int port);
  void onUsbDeviceDisconnected(int hub, int port);
  
  friend class UsbDevice;
  void resetPort(int hub, int port);
  
private:
/*  __align(8)*/ volatile HCCA* m_pHcca;
  
  UsbDevice* m_lpDevices[USB_HOSTMGR_MAX_DEVS]; 
};

#endif
