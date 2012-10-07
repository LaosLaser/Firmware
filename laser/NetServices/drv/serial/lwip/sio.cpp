
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

/*

This is a wrapper around Serial if for lwIP (acts as a serial driver)

See sio.h for functions to be implemented

sio_fd_t is a void* defined type, we use it as a SerialBuf ptr



*/


#include "netCfg.h"
#if NET_PPP

//#define MAX_SERIAL_PORTS 8

#include "lwip/sio.h"
#include "mbed.h"
//#include "sioMgr.h"
#include "drv/serial/buf/SerialBuf.h"

//#define __DEBUG
#include "dbg/dbg.h"

//extern "C" {

/**
 * Opens a serial device for communication.
 * 
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum)
{
#if 0
  SerialBuf* pIf = SioMgr::getIf(devnum);
  if(pIf == NULL)
    return NULL;
    
  //Got a SerialBuf* object
  //WARN: It HAS to be initialised (instanciated + attached to a Serial obj)
  
  return (sio_fd_t) pIf;
  #endif
  return NULL;
}

/**
 * Sends a single character to the serial device.
 * 
 * @param c character to send
 * @param fd serial device handle
 * 
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd)
{
  SerialBuf* pIf = (SerialBuf*) fd;
  //while(!pIf->writeable());
  pIf->putc( (char) c );
}

/**
 * Receives a single character from the serial device.
 * 
 * @param fd serial device handle
 * 
 * @note This function will block until a character is received.
 */
u8_t sio_recv(sio_fd_t fd)
{
  SerialBuf* pIf = (SerialBuf*) fd;
  pIf->setReadMode(false);
  while(!pIf->readable());
  return (u8_t) pIf->getc();
}

/**
 * Reads from the serial device.
 * 
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 * 
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
static volatile bool m_abort = false;
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recvd = 0; //bytes received
  SerialBuf* pIf = (SerialBuf*) fd;
  pIf->setReadMode(false);
  while(!m_abort && len)
  {
    while(!pIf->readable());
    *data = (u8_t) pIf->getc();
    data++;
    len--;
    recvd++;
  }
  m_abort = false;
  return recvd;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 * 
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recvd = 0; //bytes received
  SerialBuf* pIf = (SerialBuf*) fd;
  pIf->setReadMode(false);
  while(len)
  {
   /* if(!pIf->readable())
    {
      wait_ms(4);
    }*/
    if(!pIf->readable())
    {
      return recvd;
    }
    *data = (u8_t) pIf->getc();
    data++;
    len--;
    recvd++;
  }
  return recvd;
}

/**
 * Writes to the serial device.
 * 
 * @param fd serial device handle
 * @param data pointer to data to send
 * @param len length (in bytes) of data to send
 * @return number of bytes actually sent
 * 
 * @note This function will block until all data can be sent.
 */
u32_t sio_write(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t sent = 0; //bytes sent
  SerialBuf* pIf = (SerialBuf*) fd;
  while(len)
  {
    while(!pIf->writeable());
    pIf->putc(*data);
    data++;
    len--;
    sent++;
  }
  return sent; //Well, this is bound to be len if no interrupt mechanism
}

/**
 * Aborts a blocking sio_read() call.
 * 
 * @param fd serial device handle
 */
void sio_read_abort(sio_fd_t fd)
{
  m_abort = true;
}

//}

#endif

