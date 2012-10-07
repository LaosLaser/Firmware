
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

#ifndef SERIALBUF_H
#define SERIALBUF_H

#include "mbed.h"
#include "netCfg.h"
#if NET_USB_SERIAL
#include "drv/serial/usb/UsbSerial.h"
#endif

class SerialCircularBuf
{
public:
  SerialCircularBuf(int len);  
  ~SerialCircularBuf();
  
  int room();
  int len();
  
  void write(char c);
  char read();
  
  void setReadMode(bool readMode); //If true, keeps chars in buf when read, false by default
  void flushRead(); //Delete chars that have been read & return chars len (only useful with readMode = true)
  void resetRead(); //Go back to initial read position & return chars len (only useful with readMode = true)
  
private:
  char* m_buf;
  int m_len;
  
  volatile char* m_pReadStart;
  volatile char* m_pRead;
  volatile char* m_pWrite;
  volatile bool m_readMode;
};

class SerialBuf
{
public:
  SerialBuf(int len); //Buffer length
  virtual ~SerialBuf();
  
  void attach(Serial* pSerial);
  void detach();
  
  #if NET_USB_SERIAL
  void attach(UsbSerial* pUsbSerial);
  #endif
  
  //Really useful for debugging
  char getc();    
  void putc(char c);
  bool readable();
  bool writeable();
/*protected:*/
  void setReadMode(bool readMode); //If true, keeps chars in buf when read, false by default
  void flushRead(); //Delete chars that have been read & return chars len (only useful with readMode = true)
  void resetRead(); //Go back to initial read position & return chars len (only useful with readMode = true)
  
private:
  void onRxInterrupt(); //Callback from m_pSerial
  void onTxInterrupt(); //Callback from m_pSerial
    
  SerialCircularBuf m_rxBuf;
  SerialCircularBuf m_txBuf;

  Serial* m_pSerial; //Not owned

  #if NET_USB_SERIAL
  //USB Serial Impl
  UsbSerial* m_pUsbSerial; //Not owned
  //Ticker m_usbTick;
  #endif
};


#endif
