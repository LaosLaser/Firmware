
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

#ifndef TELITMODULENETIF_H
#define TELITMODULENETIF_H

#include "mbed.h"

#include "GPRSModuleNetIf.h"

#include "drv/gprsmodule/TelitModule.h"

class TelitModuleNetIf : public GPRSModuleNetIf
{
public:
  TelitModuleNetIf(PinName tx, PinName rx, PinName pwrSetPin, PinName pwrMonPin, int baud = 115200); 
  virtual ~TelitModuleNetIf();

protected:
  virtual bool setOn(); //True on success
  virtual bool setOff(); //True on success  
  
private:
  TelitModule m_module;

};

#endif

