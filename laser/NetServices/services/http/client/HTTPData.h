
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

#ifndef HTTP_DATA_H
#define HTTP_DATA_H

#include "core/net.h"

#include <string>
using std::string;

class HTTPData //This is a simple interface for HTTP data storage (impl examples are Key/Value Pairs, File, etc...)
{
public:
  HTTPData();
  virtual ~HTTPData();
  
  virtual void clear() = 0;

protected:
  friend class HTTPClient;
  virtual int read(char* buf, int len) = 0;
  virtual int write(const char* buf, int len) = 0;
  
  virtual string getDataType() = 0; //Internet media type for Content-Type header
  virtual void setDataType(const string& type) = 0; //Internet media type from Content-Type header
  
  virtual bool getIsChunked() = 0; //For Transfer-Encoding header
  virtual void setIsChunked(bool chunked) = 0; //From Transfer-Encoding header
  
  virtual int getDataLen() = 0; //For Content-Length header
  virtual void setDataLen(int len) = 0; //From Content-Length header, or if the transfer is chunked, next chunk length
  
private:

};

#endif
