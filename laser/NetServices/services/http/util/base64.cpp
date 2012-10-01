
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

#include "base64.h"
//Originaly from Rolf's iputil.c

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

unsigned int base64enc_len(const char* str) {
  return (((strlen(str)-1)/3)+1)<<2;
}

void base64enc(const char *input, unsigned int length, char *output) {
  static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  unsigned int c, c1, c2, c3;
  for(unsigned int i = 0, j = 0; i<length; i+=3,j+=4) {
    c1 = ((((unsigned char)*((unsigned char *)&input[i]))));
    c2 = (length>i+1)?((((unsigned char)*((unsigned char *)&input[i+1])))):0;
    c3 = (length>i+2)?((((unsigned char)*((unsigned char *)&input[i+2])))):0;

    c = ((c1 & 0xFC) >> 2);
    output[j+0] = base64[c];
    c = ((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4);
    output[j+1] = base64[c];
    c = ((c2 & 0x0F) << 2) | ((c3 & 0xC0) >> 6);
    output[j+2] = (length>i+1)?base64[c]:'=';
    c = (c3 & 0x3F);
    output[j+3] = (length>i+2)?base64[c]:'=';
  }
  output[(((length-1)/3)+1)<<2] = '\0';
}

#ifdef __cplusplus
}
#endif

