
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

#include "HTTPMap.h"
#include "../../util/url.h"

//#define __DEBUG
#include "dbg/dbg.h"

HTTPMap::HTTPMap(const string& keyValueSep /*= "="*/, const string& pairSep /*= "&"*/) : 
HTTPData(), Dictionary(), m_buf(), m_len(0), m_chunked(false), m_keyValueSep(keyValueSep), m_pairSep(pairSep)
{

}

HTTPMap::~HTTPMap()
{

}

void HTTPMap::clear()
{
  m_buf.clear();
}

int HTTPMap::read(char* buf, int len)
{
  memcpy( (void*)buf, (void*)m_buf.data(), len );
  m_buf.erase(0, len); //Erase these chars
  return len;
}

int HTTPMap::write(const char* buf, int len)
{
  m_buf.append( buf, len );
  if( ( m_chunked && !len ) || 
  ( !m_chunked && ( m_buf.length() >= m_len ) ) ) //Last chunk or EOF
  {
    parseString();
  }
  return len;
}
  
string HTTPMap::getDataType() //Internet media type for Content-Type header
{
  return "application/x-www-form-urlencoded";
}

void HTTPMap::setDataType(const string& type) //Internet media type from Content-Type header
{
  //Do not really care here
}
  
int HTTPMap::getDataLen() //For Content-Length header
{
  //This will be called before any read occurs, so let's generate our string object
  //Generate string
  generateString();
  return m_len;
}

bool HTTPMap::getIsChunked() //For Transfer-Encoding header
{
  return false; //We don't want to chunk this
}

void HTTPMap::setIsChunked(bool chunked) //From Transfer-Encoding header
{
  m_chunked = chunked;
}

void HTTPMap::setDataLen(int len) //From Content-Length header, or if the transfer is chunked, next chunk length
{
  m_len += len; //Useful so that we can parse string when last byte is written if not chunked
  m_buf.reserve( m_len );
}

void HTTPMap::generateString()
{
  Dictionary::iterator it;
  bool first = true;
  while( !Dictionary::empty() )
  {
    if(!first)
    {
      m_buf.append( m_pairSep );
    }
    else
    {
      first = false;
    }
    it = Dictionary::begin();
    m_buf.append( Url::encode( (*it).first ) );
    m_buf.append( m_keyValueSep );
    m_buf.append( Url::encode( (*it).second ) );
    Dictionary::erase(it); //Free memory as we process data
  }
  m_len = m_buf.length();
  DBG("\r\nData [len %d]:\r\n%s\r\n", m_len, m_buf.c_str());
}

void HTTPMap::parseString()
{
  string key;
  string value;
  
  int pos = 0;
  
  int key_pos;
  int key_len;
  
  int value_pos;
  int value_len;
  
  bool eof = false;
  while(!eof)
  {
    key_pos = pos;
    value_pos = m_buf.find(m_keyValueSep, pos);
    if(value_pos < 0)
    {
      //Parse error, break
      break;
    }
    
    key_len = value_pos - key_pos;
    
    value_pos+= m_keyValueSep.length();
    
    pos = m_buf.find( m_pairSep, value_pos );
    if(pos < 0) //Not found
    {
      pos = m_buf.length();
      eof = true;
    }
    
    value_len = pos - value_pos;
    
    pos += m_pairSep.length();
    
    //Append elenent
    Dictionary::operator[]( Url::decode( m_buf.substr(key_pos, key_len) ) ) = Url::decode( m_buf.substr(value_pos, value_len) );
  }
  
  m_buf.clear(); //Free data
  
}
