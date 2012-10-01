
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

/** \file
HTTP Map data source/sink header file
*/

#ifndef HTTP_MAP_H
#define HTTP_MAP_H

#include "../HTTPData.h"
#include "mbed.h"

#include <map>
using std::map;

typedef map<string, string> Dictionary;

///HTTP Client data container for key/value pairs
/**
This class simplifies the use of key/value pairs requests and responses used widely among web APIs.
Note that HTTPMap inherits from std::map<std::string,std::string>.
You can therefore use any public method of that class, including the square brackets operator ( [ ] ) to access a value.

The data is encoded or decoded to/from a key/value pairs-formatted string, after url-encoding/decoding.
*/
class HTTPMap : public HTTPData, public Dictionary //Key/Value pairs
{
public:
  ///Instantiates map
  /**
  @param keyValueSep Key/Value separator (defaults to "=")
  @param pairSep Pairs separator (defaults to "&")
  */
  HTTPMap(const string& keyValueSep = "=", const string& pairSep = "&");
  virtual ~HTTPMap();
  
 /* string& operator[](const string& key);
  int count();*/

  ///Clears the content
  virtual void clear();  
  
protected:
  virtual int read(char* buf, int len);
  virtual int write(const char* buf, int len);
  
  virtual string getDataType(); //Internet media type for Content-Type header
  virtual void setDataType(const string& type); //Internet media type from Content-Type header
  
  virtual bool getIsChunked(); //For Transfer-Encoding header
  virtual void setIsChunked(bool chunked); //From Transfer-Encoding header
  
  virtual int getDataLen(); //For Content-Length header
  virtual void setDataLen(int len); //From Content-Length header
  
private:
  void generateString();
  void parseString();
  //map<string, string> m_map;
  string m_buf;
  int m_len;
  bool m_chunked;
  
  string m_keyValueSep;
  string m_pairSep;
};

#endif
