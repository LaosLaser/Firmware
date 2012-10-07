
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

#define __LWIP_DEBUG
#define __DEBUG
#include "dbg.h"
#include "mbed.h"
#include <cstdarg>

void DebugStream::debug(const char* format, ...)
{ 
  va_list argp;
  
  va_start(argp, format);
  vprintf(format, argp);
  va_end(argp);
}

void DebugStream::release()
{

}

void DebugStream::breakPoint(const char* file, int line)
{
  printf("\r\nBREAK in %s at line %d\r\n", file, line);
  fflush(stdout);
  getchar();
  fflush(stdin);
}

/*
int snprintf(char *str, int size, const char *format, ...)
{
  va_list argp;
  
  va_start(argp, format);
  vsprintf(str, format, argp);
  va_end(argp);
  
  return strlen(str);
}
*/
