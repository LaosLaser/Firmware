
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

#include "usb_mem.h"

#include "string.h" //For memcpy, memmove, memset

#include "netCfg.h"
#if NET_USB

#define EDS_COUNT 4
#define TDS_COUNT 8

#define HCCA_SIZE 0x100
#define ED_SIZE 0x10
#define TD_SIZE 0x10

#define TOTAL_SIZE (HCCA_SIZE + (EDS_COUNT*ED_SIZE) + (TDS_COUNT*TD_SIZE))


static volatile __align(256) byte usb_buf[TOTAL_SIZE] __attribute((section("AHBSRAM1"),aligned));  //256 bytes aligned!

static volatile byte* usb_hcca;  //256 bytes aligned!

static volatile byte* usb_edBuf;  //4 bytes aligned!
static volatile byte* usb_tdBuf;  //4 bytes aligned!

static byte usb_edBufAlloc[EDS_COUNT] __attribute((section("AHBSRAM1"),aligned));
static byte usb_tdBufAlloc[TDS_COUNT] __attribute((section("AHBSRAM1"),aligned));

void usb_mem_init()
{
  usb_hcca = usb_buf;
  usb_edBuf = usb_buf + HCCA_SIZE;
  usb_tdBuf = usb_buf + HCCA_SIZE + (EDS_COUNT*ED_SIZE);
  memset(usb_edBufAlloc, 0, EDS_COUNT);
  memset(usb_tdBufAlloc, 0, TDS_COUNT);
}

volatile byte* usb_get_hcca()
{
  return usb_hcca;
}

volatile byte* usb_get_ed()
{
  int i;
  for(i = 0; i < EDS_COUNT; i++)
  {
    if( !usb_edBufAlloc[i] )
    {
      usb_edBufAlloc[i] = 1;
      return usb_edBuf + i*ED_SIZE;
    }
  }
  return NULL; //Could not alloc ED
}

volatile byte* usb_get_td()
{
  int i;
  for(i = 0; i < TDS_COUNT; i++)
  {
    if( !usb_tdBufAlloc[i] )
    {
      usb_tdBufAlloc[i] = 1;
      return usb_tdBuf + i*TD_SIZE;
    }
  }
  return NULL; //Could not alloc TD
}

void usb_free_ed(volatile byte* ed)
{
  int i;
  i = (ed - usb_edBuf) / ED_SIZE;
  usb_edBufAlloc[i] = 0;
}

void usb_free_td(volatile byte* td)
{
  int i;
  i = (td - usb_tdBuf) / TD_SIZE;
  usb_tdBufAlloc[i] = 0;
}


#endif
