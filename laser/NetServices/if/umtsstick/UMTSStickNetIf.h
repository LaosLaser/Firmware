
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
UMTS Stick network interface header file
*/

#ifndef UMTSSTICKNETIF_H
#define UMTSSTICKNETIF_H

#include "mbed.h"

#include "core/net.h"
#include "if/ppp/PPPNetIf.h"

#include "drv/umtsstick/UMTSStick.h"

///UMTS Stick network interface
/**
This class provides connectivity to the stack using a 3G (or LTE etc...) stick
Plug it to your USB host using two Pull-down resistors on the D+/D- lines
*/
class UMTSStickNetIf : public LwipNetIf, protected PPPNetIf
{
public:
  ///Instantiates the Interface and register it against the stack
  UMTSStickNetIf(); 
  virtual ~UMTSStickNetIf();
  
  ///Tries to connect to the stick
  /**
  This method tries to obtain a virtual serial port interface from the stick
  It waits for a stick to be connected, switches it from CDFS to virtual serial port mode if needed,
  and obtains a virtual serial port from it
  @return : A negative error code on error or 0 on success
  */
  UMTSStickErr setup(); //UMTSStickErr is from /drv/umtsstick/UMTSStick.h
  
  ///Establishes a PPP connection
  /**
  This method opens an AT interface on the serial interface, initializes and configures the stick,
  then opens a PPP connection and authenticates with the parameters
  \param apn : APN of the interface, if NULL uses the SIM default value
  \param userId : user with which to authenticate during the PPP connection, if NULL does not authenticate
  \param password : associated password
  @return : A negative error code on error or 0 on success
  */
  PPPErr connect(const char* apn = NULL, const char* userId = NULL, const char* password = NULL); //Connect using GPRS
  
  ///Disconnects the PPP connection
  PPPErr disconnect();
  
private:
  UMTSStick m_umtsStick;
  UsbSerial* m_pUsbSerial;

};

#endif

