
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
#if 0 //Dep 

#include "sioMgr.h"


u8_t SioMgr::m_count=0;
SerialBuf* SioMgr::m_pSerialBufList[MAX_SERIAL_PORTS] = {NULL};

u8_t SioMgr::registerSerialIf(SerialBuf* pIf)
{
  if(m_count==MAX_SERIAL_PORTS)
    return 0;
    
  m_pSerialBufList[m_count] = pIf;
  m_count++;
  
  return m_count; //We do not want a fd to be NULL, so return value is p+1
}
  
SerialBuf* SioMgr::getIf(u8_t item)
{
  if (!item || item > m_count)
    return NULL;
    
  return m_pSerialBufList[item-1];
}
#endif
