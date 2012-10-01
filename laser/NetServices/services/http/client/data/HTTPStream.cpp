
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

#include "HTTPStream.h"

#define MIN(a,b) ((a)<(b)?(a):(b))

HTTPStream::HTTPStream() : HTTPData(), m_buf(NULL), m_size(0), m_len(0)
{

}

HTTPStream::~HTTPStream()
{

}


void HTTPStream::readNext(byte* buf, int size)
{
  m_buf = buf;
  m_size = size;
  m_len = 0;
}
  
bool HTTPStream::readable()
{
  return (m_len>0);
}
  
int HTTPStream::readLen()
{
  return m_len;
}

void HTTPStream::clear()
{
  //Do nothing here since it is a stream and not a buffer
}

int HTTPStream::read(char* buf, int len)
{
  return 0; //Unsupported yet
}

int HTTPStream::write(const char* buf, int len)
{
  if(!m_buf)
    return 0;
  len = MIN( m_size - m_len, len );
  memcpy(m_buf + m_len, buf, len);
  m_len += len;
  return len;
}
  
string HTTPStream::getDataType() //Internet media type for Content-Type header
{
  return "";
}

void HTTPStream::setDataType(const string& type) //Internet media type from Content-Type header
{
  //Do not really care here
}

bool HTTPStream::getIsChunked() //For Transfer-Encoding header
{
  return false; //We don't want to chunk this
}

void HTTPStream::setIsChunked(bool chunked) //From Transfer-Encoding header
{
  //Nothing to do
}
  
int HTTPStream::getDataLen() //For Content-Length header
{
  return 0;
}

void HTTPStream::setDataLen(int len) //From Content-Length header, or if the transfer is chunked, next chunk length
{

}
