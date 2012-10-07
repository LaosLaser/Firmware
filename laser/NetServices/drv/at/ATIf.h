
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

#ifndef ATIF_H
#define ATIF_H

#include "netCfg.h"

#include "mbed.h"
#include "drv/serial/buf/SerialBuf.h"
#include <list>
using std::list;

//class Serial; //Pb w forward decl

enum ATErr
{
  __AT_MIN = -0xFFFF,
  AT_CLOSED,
  AT_NOANSWER,
  AT_ERROR,
  AT_TIMEOUT, 
  AT_BUSY,
  AT_PARSE,
  AT_INCOMPLETE,
  AT_OK = 0
};

class ATIf : public SerialBuf
{
  public:
    
    ATIf();
    virtual ~ATIf();

#if 0    
    template<class T> 
    //void attachSignal( const char* sigName, T* pItem, bool (T::*pMethod)(ATIf*, bool, bool*) ); //Attach Signal ("Unsollicited response code" in Telit_AT_Reference_Guide.pdf) to an handler fn
    //Linker bug : Must be defined here :(
    void attachSignal( const char* sigName, T* pItem, bool (T::*pMethod)(ATIf*, bool, bool*) ) //Attach Signal ("Unsollicited response code" in Telit_AT_Reference_Guide.pdf) to an handler fn
    {
      ATSigHandler sig(sigName, (ATSigHandler::CDummy*)pItem, (bool (ATSigHandler::CDummy::*)(ATIf*, bool, bool*))pMethod);
      m_signals.push_back(sig);
    }
    void detachSignal( const char* sigName );    
#endif
  
    ATErr open(Serial* pSerial); //Deactivate echo, etc
    #if NET_USB_SERIAL
    ATErr open(UsbSerial* pUsbSerial); //Deactivate echo, etc
    #endif
    ATErr close(); //Release port
    
    int printf(const char* format, ... );
    int scanf(const char* format, ... );
    void setTimeout(int timeout); //used by scanf
    void setLineMode(bool lineMode); //Switch btw line & raw fns
    //void setSignals(bool signalsEnable);
    ATErr flushBuffer(); //Discard input buffer
    ATErr flushLine(int timeout); //Discard input buffer until CRLF is encountered
    
  protected:
    //virtual bool onRead(); //Inherited from SerialBuf, return true if data is incomplete
    ATErr rawOpen(Serial* pSerial, int baudrate); //Simple open function for similar non-conforming protocols

  public:
/*    ATErr command(const char* cmd, char* result, int resultLen, int timeout); */ //Kinda useless
    ATErr write(const char* cmd, bool lineMode = false);
    ATErr read(char* result, int resultMaxLen, int timeout, bool lineMode = false, int resultMinLen = 0);
    bool isOpen();
    ATErr checkOK(); //Helper fn to quickly check that OK has been returned
  
  private:  
    int readLine(char* line, int maxLen, int timeout); //Read a single line from serial port
    int writeLine(const char* line); //Write a single line to serial port
    
    int readRaw(char* str, int maxLen, int timeout = 0, int minLen = 0); //Read from serial port in buf
    int writeRaw(const char* str); //Write directly to serial port
      
    volatile int m_readTimeout;
    volatile bool m_lineMode;
    //bool m_signalsEnable;
    bool m_isOpen;
    
    char* m_tmpBuf;
 
#if 0 
    class ATSigHandler
    {
      class CDummy;
      public:
        ATSigHandler(const char* name, CDummy* cbObj, bool (CDummy::*cbMeth)(ATIf* pIf, bool beg, bool* pMoreData)) : m_cbObj(cbObj), m_cbMeth(cbMeth), m_name(name) 
        {}
      protected:
        CDummy* m_cbObj;
        bool (CDummy::*m_cbMeth)(ATIf* pIf, bool beg, bool* pMoreData); //*pMoreData set to true if needs to read next line, beg = true if beginning of new code
        const char* m_name;
     
      friend class ATIf;
    };
    
    volatile ATSigHandler* m_pCurrentSignal; //Signal that asked more data
    
    bool handleSignal(); //Returns true if signal has been handled
    list<ATSigHandler> m_signals;
#endif
    
};

#endif
