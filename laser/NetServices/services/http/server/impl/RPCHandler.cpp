#pragma diag_remark 1293
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

#include "RPCHandler.h"
#include "rpc.h"
#include "url.h"

//#define __DEBUG
#include "dbg/dbg.h"

#define RPC_DATA_LEN 128

RPCHandler::RPCHandler(const char* rootPath, const char* path, TCPSocket* pTCPSocket) : HTTPRequestHandler(rootPath, path, pTCPSocket)
{}

RPCHandler::~RPCHandler()
{
  DBG("\r\nHandler destroyed\r\n");
}

void RPCHandler::doGet()
{
  DBG("\r\nIn RPCHandler::doGet()\r\n");
  char resp[RPC_DATA_LEN] = {0};
  char req[RPC_DATA_LEN] = {0};
  
  DBG("\r\nPath : %s\r\n", path().c_str());
  DBG("\r\nRoot Path : %s\r\n", rootPath().c_str());
  
  //Remove path
  strncpy(req, path().c_str(), RPC_DATA_LEN-1);
  DBG("\r\nRPC req : %s\r\n", req);
  
  //Remove %20, +, from req
  cleanReq(req);
  DBG("\r\nRPC req : %s\r\n", req);
  
  //Do RPC Call
  mbed::rpc(req, resp); //FIXME: Use bool result
  
  //Response
  setContentLen( strlen(resp) );
  
  //Make sure that the browser won't cache this request
  respHeaders()["Cache-control"]="no-cache;no-store";
 // respHeaders()["Cache-control"]="no-store";
  respHeaders()["Pragma"]="no-cache";
  respHeaders()["Expires"]="0";
  
  //Write data
  respHeaders()["Connection"] = "close";
  writeData(resp, strlen(resp));
  DBG("\r\nExit RPCHandler::doGet()\r\n");
}

void RPCHandler::doPost()
{

}

void RPCHandler::doHead()
{

}

  
void RPCHandler::onReadable() //Data has been read
{

}

void RPCHandler::onWriteable() //Data has been written & buf is free
{
  DBG("\r\nRPCHandler::onWriteable() event\r\n");
  close(); //Data written, we can close the connection
}

void RPCHandler::onClose() //Connection is closing
{
  //Nothing to do
}

void RPCHandler::cleanReq(char* data)
{
    char* decoded = url_decode(data);
    strcpy(data, decoded);
    free(decoded);
/*    
  char* p;
  static const char* lGarbage[2] = {"%20", "+"};
  for(int i = 0; i < 2; i++)
  {
    while( p = strstr(data, lGarbage[i]) )
    {
      memset((void*) p, ' ', strlen(lGarbage[i]));
    }
  }
*/  
}
  
  
