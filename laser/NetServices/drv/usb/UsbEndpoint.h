
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

#ifndef USB_ENDPOINT_H
#define USB_ENDPOINT_H

#include "mbed.h"
#include "UsbInc.h"

class UsbDevice;

enum UsbEndpointType
{
  USB_CONTROL,
  USB_BULK
//  USB_INT,
//  USB_ISO
};

class UsbEndpoint
{
public:
  UsbEndpoint( UsbDevice* pDevice, uint8_t ep, bool dir, UsbEndpointType type, uint16_t size, int addr = -1 );
  ~UsbEndpoint();

  void setNextToken(uint32_t token); //Only for control Eps
  
  UsbErr transfer(volatile uint8_t* buf, uint32_t len); 
  
  int status(); //return UsbErr or transfered len
  
  void updateAddr(int addr);
  void updateSize(uint16_t size);
  
  //void setOnCompletion( void(*pCb)completed() );
  class CDummy;
  template <class T>
  void setOnCompletion( T* pCbItem, void (T::*pCbMeth)() )
  {
    m_pCbItem = (CDummy*) pCbItem;
    m_pCbMeth = (void (CDummy::*)()) pCbMeth;
  }
   
//static void completed(){}

protected:
  void onCompletion();
public:
  static void sOnCompletion(uint32_t pTd);
  
private:
  friend class UsbDevice;

  UsbDevice* m_pDevice;
  
  bool m_dir;
  bool m_setup;
  UsbEndpointType m_type;
  
  //bool m_done;
  volatile bool m_result;
  volatile int m_status;
  
  volatile uint32_t m_len;
  
  volatile uint8_t* m_pBufStartPtr;

  volatile HCED* m_pEd; //Ep descriptor
  volatile HCTD* m_pTdHead; //Head trf descriptor
  volatile HCTD* m_pTdTail; //Tail trf descriptor
  
  //static volatile HCED* m_pNextEd;
  
  CDummy* m_pCbItem;
  void (CDummy::*m_pCbMeth)();
  
  
  static UsbEndpoint* m_pHeadEp;
  UsbEndpoint* m_pNextEp;
  
};

#endif
