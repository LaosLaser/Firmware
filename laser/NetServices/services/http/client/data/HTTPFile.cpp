
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

#include "HTTPFile.h"

//#define __DEBUG
#include "dbg/dbg.h"

HTTPFile::HTTPFile(const char* path) : HTTPData(), m_fp(NULL), m_path(path), m_len(0), m_chunked(false)
{

}

HTTPFile::~HTTPFile()
{
  closeFile();
}

void HTTPFile::clear()
{
  closeFile();
  //Force reopening
}

int HTTPFile::read(char* buf, int len)
{
  if(!openFile("r")) //File does not exist, or I/O error...
    return 0;
  len = fread(buf, 1, len, m_fp);
  if( feof(m_fp) )
  {
    //File read completely, we can close it
    closeFile();
  }
  return len;
}

int HTTPFile::write(const char* buf, int len)
{
  if(!openFile("w")) //File does not exist, or I/O error...
    return 0;
  len = fwrite(buf, 1, len, m_fp);
  DBG("Written %d bytes in %d\n", len, m_fp);
  if( (!m_chunked && (ftell(m_fp) >= m_len)) ||
      (m_chunked && !len) )
  {
    //File received completely, we can close it
    closeFile();
  }
  return len;
}
  
string HTTPFile::getDataType() //Internet media type for Content-Type header
{
  return ""; //Unknown
}

void HTTPFile::setDataType(const string& type) //Internet media type from Content-Type header
{
  //Do not really care here
}

bool HTTPFile::getIsChunked() //For Transfer-Encoding header
{
  return false;
}

void HTTPFile::setIsChunked(bool chunked) //From Transfer-Encoding header
{
  m_chunked = chunked;
}
  
int HTTPFile::getDataLen() //For Content-Length header
{
  return m_len;
}

void HTTPFile::setDataLen(int len) //From Content-Length header, or if the transfer is chunked, next chunk length
{
  if(!m_chunked)
    m_len = len; //Useful so that we can close file when last byte is written
}

bool HTTPFile::openFile(const char* mode) //true on success, false otherwise
{
  if(m_fp) 
    return true;
  
  m_fp = fopen(m_path.c_str(), mode);
  if(m_fp && mode[0]=='r')
  {
    //Seek EOF to get length
    fseek(m_fp, 0, SEEK_END);
    m_len = ftell(m_fp);
    fseek(m_fp, 0, SEEK_SET); //Goto SOF
  }
  
  DBG("fd = %d\n", m_fp);
  
  if(!m_fp) 
    return false;
    
  return true;
}

void HTTPFile::closeFile()
{
  if(m_fp)
    fclose(m_fp);
}
