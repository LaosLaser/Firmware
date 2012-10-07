
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

#ifndef HTTP_STREAM_H
#define HTTP_STREAM_H

#include "../HTTPData.h"
#include "mbed.h"

/** \file
HTTP Stream data source/sink header file
*/

typedef uint8_t byte;

///HTTP Client Streaming tool
/**
This class allows you to stream data from the web using a persisting HTTP connection.
To use it properly you must use a non-blocking HTTPClient method.
*/
class HTTPStream : public HTTPData //Streaming buf
{
public:
  ///Instantiates the object
  HTTPStream();
  virtual ~HTTPStream();
  
  ///Starts to read into buffer
  /**
  Passes a buffer of address @a buf and size @a size to the instance.
  When it receives data it will be stored in this buffer.
  When the buffer is full it throttles the client until this function is called again.
  */
  void readNext(byte* buf, int size);
  
  ///Returns whether there is data available to read
  bool readable();
  
  ///Returns the actual length of the payload written in the buffer
  int readLen();
  
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
  byte* m_buf;
  int m_size; //Capacity
  int m_len; //Length
};

#endif
