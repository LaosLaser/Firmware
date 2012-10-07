
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

#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include "mbed.h"
#include "UsbInc.h"
#include "UsbEndpoint.h"
#include "UsbHostMgr.h"

class UsbHostMgr;
class UsbEndpoint;

class UsbDevice
{
protected:
  UsbDevice( UsbHostMgr* pMgr, int hub, int port, int addr );
  ~UsbDevice();

  UsbErr enumerate();

public: 
  bool connected();
  bool enumerated();
  
  int getPid();
  int getVid();
  
  UsbErr getConfigurationDescriptor(int config, uint8_t** pBuf);
  
  UsbErr getInterfaceDescriptor(int config, int item, uint8_t** pBuf);
  
  UsbErr setConfiguration(int config);
  

  UsbErr controlSend(byte requestType, byte request, word value, word index, const byte* buf, int len);
  UsbErr controlReceive(byte requestType, byte request, word value, word index, const byte* buf, int len);

protected:
  void fillControlBuf(byte requestType, byte request, word value, word index, int len);
private:
  friend class UsbEndpoint;
  friend class UsbHostMgr;
  
  UsbEndpoint* m_pControlEp;
  
  UsbHostMgr* m_pMgr;
  
  bool m_connected;
  bool m_enumerated;
  
  int m_hub;
  int m_port;
  int m_addr;
  
  int m_refs;
  
  uint16_t m_vid;
  uint16_t m_pid;
  
  byte m_controlBuf[8];//8
  byte m_controlDataBuf[/*128*/256];
  
};

#endif
