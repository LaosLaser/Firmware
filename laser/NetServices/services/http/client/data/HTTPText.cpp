
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

#include "HTTPText.h"



#define MIN(a,b) ((a)<(b)?(a):(b))

HTTPText::HTTPText(const string& encoding /*= "text/html"*/, int maxSize /*= DEFAULT_MAX_MEM_ALLOC*/) : HTTPData(), m_buf(), m_encoding(encoding), m_maxSize(maxSize)
{

}

HTTPText::~HTTPText()
{

}

const char* HTTPText::gets() const
{
  return m_buf.c_str();
}

void HTTPText::puts(const char* str)
{
  m_buf = string(str);
}

string& HTTPText::get()
{
  return m_buf;
}

void HTTPText::set(const string& str)
{
  m_buf = str;
}

void HTTPText::clear()
{
  m_buf.clear();
}

int HTTPText::read(char* buf, int len)
{
  len = MIN( len, m_buf.length() );
  memcpy( (void*)buf, (void*)m_buf.data(), len );
  m_buf.erase(0, len); //Erase these chars
  return len;
}

int HTTPText::write(const char* buf, int len)
{
  len = MIN( m_maxSize - m_buf.length(), len ); //Do not go over m_maxSize
  m_buf.append( buf, len );
  return len;
}
  
string HTTPText::getDataType() //Internet media type for Content-Type header
{
  return m_encoding;
}

void HTTPText::setDataType(const string& type) //Internet media type from Content-Type header
{
  //Do not really care here
}

bool HTTPText::getIsChunked() //For Transfer-Encoding header
{
  return false; //We don't want to chunk this
}

void HTTPText::setIsChunked(bool chunked) //From Transfer-Encoding header
{
  //Nothing to do
}
  
int HTTPText::getDataLen() //For Content-Length header
{
  return m_buf.length();
}

void HTTPText::setDataLen(int len) //From Content-Length header, or if the transfer is chunked, next chunk length
{
  m_buf.reserve(MIN(m_buf.length() + len,m_maxSize));
}
