
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
HTTP Text data source/sink header file
*/

#ifndef HTTP_TEXT_H
#define HTTP_TEXT_H

#include "../HTTPData.h"
#include "mbed.h"

#define DEFAULT_MAX_MEM_ALLOC 512 //Avoid out-of-memory problems

///HTTP Client data container for text
/**
This is a simple "Text" data repository for HTTP requests.
*/
class HTTPText : public HTTPData //Simple Text I/O
{
public:
  ///Instantiates the object.
  /**
  @param encoding encoding of the data, it defaults to text/html.
  @param maxSize defines the maximum memory size that can be allocated by the object. It defaults to 512 bytes.
  */
  HTTPText(const string& encoding = "text/html", int maxSize = DEFAULT_MAX_MEM_ALLOC);
  virtual ~HTTPText();
  
  ///Gets text
  /**
  Returns the text in the container as a zero-terminated char*.
  The array returned points to the internal buffer of the object and remains owned by the object.
  */
  const char* gets() const;
  
  //Puts text
  /**
  Sets the text in the container using a zero-terminated char*.
  */
  void puts(const char* str);
  
  ///Gets text
  /**
  Returns the text in the container as string.
  */
  string& get();
  
  ///Puts text
  /**
  Sets the text in the container as string.
  */
  void set(const string& str);
  
  ///Clears the content.
  /**
  If this container is used as a data sink, it is cleared by the HTTP Client at the beginning of the request.
  */
  virtual void clear();
  
protected:
  virtual int read(char* buf, int len);
  virtual int write(const char* buf, int len);
  
  virtual string getDataType(); //Internet media type for Content-Type header
  virtual void setDataType(const string& type); //Internet media type from Content-Type header

  virtual bool getIsChunked(); //For Transfer-Encoding header
  virtual void setIsChunked(bool chunked); //From Transfer-Encoding header
  
  virtual int getDataLen(); //For Content-Length header
  virtual void setDataLen(int len); //From Content-Length header, or if the transfer is chunked, next chunk length
  
private:
  string m_buf;
  string m_encoding;
  int m_maxSize;
};

#endif
