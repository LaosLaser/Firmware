
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

#ifndef USB_SERIAL_H
#define USB_SERIAL_H

//DG 2010
//Essentially a clone of Serial if
#include "Stream.h"
#include "mbed.h"

#include "drv/usb/UsbDevice.h"
#include "drv/usb/UsbEndpoint.h"

namespace mbed {

class UsbSerial : public Stream {

public:

    UsbSerial(UsbDevice* pDevice, int epIn, int epOut, const char* name = NULL);
    virtual ~UsbSerial();
    //Apart from the ctor/dtor, exactly the same protos as Serial

    void baud(int baudrate);

    enum Parity {
        None = 0,
        Odd = 1,
        Even = 2,
        Forced1 = 3,
        Forced0 = 4
    };
    
    enum IrqType {
        RxIrq = 0
        , TxIrq
    };

    void format(int bits, int parity, int stop); 
    
    class CDummy;
    template <class T>
    void attach(T* pCbItem, void (T::*pCbMeth)(), IrqType type = RxIrq)
    {
      if(type == RxIrq)
      {
        m_pInCbItem = (CDummy*) pCbItem;
        m_pInCbMeth = (void (CDummy::*)()) pCbMeth;
      }
      else
      {
        m_pOutCbItem = (CDummy*) pCbItem;
        m_pOutCbMeth = (void (CDummy::*)()) pCbMeth;
      }
    }
    
#if 0 // Inhereted from Stream, for documentation only

    /* Function: putc
     *  Write a character
     *
     * Variables:
     *  c - The character to write to the serial port
     */
    int putc(int c);

    /* Function: getc
     *  Read a character
     *
     * Variables:
     *  returns - The character read from the serial port
     */
    int getc();
        
    /* Function: printf
     *  Write a formated string
     *
     * Variables:
     *  format - A printf-style format string, followed by the 
     *      variables to use in formating the string.
     */
    int printf(const char* format, ...);

    /* Function: scanf
     *  Read a formated string 
     *
     * Variables:
     *  format - A scanf-style format string,
     *      followed by the pointers to variables to store the results. 
     */
    int scanf(const char* format, ...);
         
#endif
               
    /* Function: readable
     *  Determine if there is a character available to read
     *
     * Variables:
     *  returns - 1 if there is a character available to read, else 0
     */
    int readable();

    /* Function: writeable
     *  Determine if there is space available to write a character
     * 
     * Variables:
     *  returns - 1 if there is space to write a character, else 0
     */
    int writeable();    

#ifdef MBED_RPC
    virtual const struct rpc_method *get_rpc_methods();
    static struct rpc_class *get_rpc_class();
#endif
    
protected:

    virtual int _getc();    
    virtual int _putc(int c);
    
    void onReadable();
    void onWriteable();
    
    void onEpInTransfer();
    void onEpOutTransfer();
    
private:

    UsbEndpoint m_epIn;
    UsbEndpoint m_epOut;
    
    CDummy* m_pInCbItem;
    void (CDummy::*m_pInCbMeth)();

    CDummy* m_pOutCbItem;
    void (CDummy::*m_pOutCbMeth)();

    void startTx();
    void startRx();
    
    Timeout m_txTimeout;
    volatile bool m_timeout;

    volatile char* m_inBufEven;    
    volatile char* m_inBufOdd;
    volatile char* m_inBufUsr;
    volatile char* m_inBufTrmt;
    
    volatile char* m_outBufEven;
    volatile char* m_outBufOdd;
    volatile char* m_outBufUsr;
    volatile char* m_outBufTrmt;
    
    volatile int m_inBufLen;
    volatile int m_outBufLen;
    
    volatile char* m_pInBufPos;
    volatile char* m_pOutBufPos;
    
    
    
};

}



#endif
