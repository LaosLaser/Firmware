
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

#include "SerialBuf.h"
#include "mbed.h"

//#define __DEBUG
#include "dbg/dbg.h"

#include "netCfg.h"
#if NET_GPRS

#if NET_USB_SERIAL
#define m_pStream( a ) (m_pSerial?m_pSerial->a:m_pUsbSerial->a)
#else
#define m_pStream( a ) (m_pSerial->a)
#endif

//Circular buf

SerialCircularBuf::SerialCircularBuf(int len) : m_readMode(false)
{
  m_buf = new char[len];
  m_len = len;
  m_pReadStart = m_pRead = m_buf;
  m_pWrite = m_buf;
} 

SerialCircularBuf::~SerialCircularBuf()
{
  if(m_buf)
    delete[] m_buf;
}

int SerialCircularBuf::room() //Return room available in buf
{
  //return m_len - len() - 1; //-1 is to avoid loop
  if ( m_pReadStart > m_pWrite )
    return ( m_pReadStart - m_pWrite - 1 );
  else
    return m_len - ( m_pWrite - m_pReadStart ) - 1;
}

int SerialCircularBuf::len() //Return chars length in buf
{
  if ( m_pWrite >= m_pRead )
    return ( m_pWrite - m_pRead );
  else
    return m_len - ( m_pRead - m_pWrite ); // = ( ( m_buf + m_len) - m_pRead ) + ( m_pWrite - m_buf )
}

void SerialCircularBuf::write(char c)
{
#if 0
  if(!room())
    return;
#endif
  //WARN: Must call room() before
  *m_pWrite = c;
  m_pWrite++;
  if(m_pWrite>=m_buf+m_len)
    m_pWrite=m_buf;
}
char SerialCircularBuf::read()
{
#if 0
  if(!len())
    return 0;
#endif
  //WARN: Must call len() before
  char c = *m_pRead;
  m_pRead++;
  
  if(m_pRead>=m_buf+m_len)
    m_pRead=m_buf;
    
  if(!m_readMode) //If readmode=false, trash this char
    m_pReadStart=m_pRead;
    
  return c;
}

void SerialCircularBuf::setReadMode(bool readMode) //If true, keeps chars in buf when read, false by default
{
  if(m_readMode == true && readMode == false)
  {
    //Trash bytes that have been read
    flushRead();
  }
  m_readMode = readMode;
}

void SerialCircularBuf::flushRead() //Delete chars that have been read & return chars len (only useful with readMode = true)
{
  m_pReadStart = m_pRead;
}

void SerialCircularBuf::resetRead() //Go back to initial read position & return chars len (only useful with readMode = true)
{
  m_pRead = m_pReadStart;
}

//SerialBuf

SerialBuf::SerialBuf(int len) : m_rxBuf(len), m_txBuf(len), m_pSerial(NULL) //Buffer length
#if NET_USB_SERIAL
, m_pUsbSerial(NULL) 
#endif
{
  DBG("New Serial buf@%p\n", this);
}

SerialBuf::~SerialBuf()
{

}

void SerialBuf::attach(Serial* pSerial)
{
  DBG("Serial buf@%p in attach\n", this);
  m_pSerial = pSerial;
  m_pSerial->attach<SerialBuf>(this, &SerialBuf::onRxInterrupt, Serial::RxIrq);
  m_pSerial->attach<SerialBuf>(this, &SerialBuf::onTxInterrupt, Serial::TxIrq);
  onRxInterrupt(); //Read data
}

void SerialBuf::detach()
{
  if(m_pSerial)
  {
    m_pSerial->attach<SerialBuf>(NULL, NULL, Serial::RxIrq);
    m_pSerial->attach<SerialBuf>(NULL, NULL, Serial::TxIrq);
    m_pSerial = NULL;
  }
  #if NET_USB_SERIAL
  else if(m_pUsbSerial)
  {
    m_pUsbSerial->attach<SerialBuf>(NULL, NULL, UsbSerial::RxIrq);
    m_pUsbSerial->attach<SerialBuf>(NULL, NULL, UsbSerial::TxIrq);
    m_pUsbSerial = NULL;
  }
  #endif
}

#if NET_USB_SERIAL
void SerialBuf::attach(UsbSerial* pUsbSerial)
{
  m_pUsbSerial = pUsbSerial;
  m_pUsbSerial->attach<SerialBuf>(this, &SerialBuf::onRxInterrupt, UsbSerial::RxIrq);
  m_pUsbSerial->attach<SerialBuf>(this, &SerialBuf::onTxInterrupt, UsbSerial::TxIrq);
  onRxInterrupt(); //Read data
}
#endif

char SerialBuf::getc()
{
  while(!readable());
  char c = m_rxBuf.read();
  return c;
}

void SerialBuf::putc(char c)
{
  while(!writeable());
  m_txBuf.write(c);
  onTxInterrupt();
}

bool SerialBuf::readable()
{
  if( !m_rxBuf.len() ) //Fill buf if possible
    onRxInterrupt();
  return (m_rxBuf.len() > 0);
}

bool SerialBuf::writeable()
{
  if( !m_txBuf.room() ) //Free buf is possible
    onTxInterrupt();
  return (m_txBuf.room() > 0);
}

void SerialBuf::setReadMode(bool readMode) //If true, keeps chars in buf when read, false by default
{
  m_rxBuf.setReadMode(readMode);
}

void SerialBuf::flushRead() //Delete chars that have been read & return chars len (only useful with readMode = true)
{
  m_rxBuf.flushRead();
}

void SerialBuf::resetRead() //Go back to initial read position & return chars len (only useful with readMode = true)
{
  m_rxBuf.resetRead();
}

void SerialBuf::onRxInterrupt() //Callback from m_pSerial
{
  while( m_rxBuf.room() && m_pStream(readable()) )
  {
    m_rxBuf.write(m_pStream(getc()));
  }
}

void SerialBuf::onTxInterrupt() //Callback from m_pSerial
{
  while( m_txBuf.len() && m_pStream(writeable()) )
  {
    m_pStream(putc(m_txBuf.read()));
  }
}


#endif
