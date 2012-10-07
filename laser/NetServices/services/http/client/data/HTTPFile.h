
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
HTTP File data source/sink header file
*/

#ifndef HTTP_FILE_H
#define HTTP_FILE_H

#include "../HTTPData.h"
#include "mbed.h"

///HTTP Client data container for files
/**
This class provides file access/storage for HTTP requests and responses' data payloads.


*/
class HTTPFile : public HTTPData //Read or Write data from a file
{
public:
  ///Instantiates data source/sink with file in param.
  /**
  Uses file at path @a path.
  It will be opened when some data has to be read/written from/to it and closed when this operation is complete or on destruction of the instance.
  Note that the file will be opened with mode "w" for writing and mode "r" for reading, so the file will be cleared between each request if you are using it for writing.
  
  @note
  Note that to use this you must instantiate a proper file system (such as the LocalFileSystem or the SDFileSystem).
  */
  HTTPFile(const char* path);
  virtual ~HTTPFile();
  
  ///Forces file closure
  virtual void clear();

protected:
  virtual int read(char* buf, int len);
  virtual int write(const char* buf, int len);
  
  virtual string getDataType(); //Internet media type for Content-Type header
  virtual void setDataType(const string& type); //Internet media type from Content-Type header
  
  virtual bool getIsChunked(); //For Transfer-Encoding header
  virtual void setIsChunked(bool chunked); //From Transfer-Encoding header  virtual
  
  virtual int getDataLen(); //For Content-Length header
  virtual void setDataLen(int len); //From Content-Length header
  
private:
  bool openFile(const char* mode); //true on success, false otherwise
  void closeFile();

  FILE* m_fp;
  string m_path;
  int m_len;
  bool m_chunked;
};

#endif
