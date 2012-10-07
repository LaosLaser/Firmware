
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

#include "netCfg.h"
#if NET_GPRS_MODULE

#include "HuaweiGTM900NetIf.h"

#define __DEBUG
#include "dbg/dbg.h"

HuaweiGTM900NetIf::HuaweiGTM900NetIf(PinName tx, PinName rx, int baud /*= 115200*/) : GPRSModuleNetIf(tx, rx, baud)
{

}

HuaweiGTM900NetIf::~HuaweiGTM900NetIf()
{

}
  
bool HuaweiGTM900NetIf::setOn()
{  
  wait(5); //Wait for module to init.

  return true;
}

bool HuaweiGTM900NetIf::setOff()
{
  return true;
}
  

#endif
