
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
MySQL Client header file
*/

#ifndef MYSQL_CLIENT_H
#define MYSQL_CLIENT_H

#include "core/net.h"
#include "core/netservice.h"
#include "api/TCPSocket.h"
#include "api/DNSRequest.h"
#include "mbed.h"

#include <string>
using std::string;

#include <map>
using std::map;

typedef unsigned char byte;

///MySQL client results
enum MySQLResult
{
  MYSQL_OK, ///<Success
  MYSQL_PROCESSING, ///<Processing
  MYSQL_PRTCL, ///<Protocol error
  MYSQL_SETUP, ///<Not properly configured
  MYSQL_DNS, ///<Could not resolve name
  MYSQL_AUTHFAILED, ///<Auth failure
  MYSQL_READY, ///<Ready to send commands
  MYSQL_SQL, ///<SQL Error
  MYSQL_TIMEOUT, ///<Connection timeout
  MYSQL_CONN ///<Connection error
};

///A MySQL Client
/**
This MySQL client implements a limited subset of the MySQL internal client/server protocol (including authentication), for server versions 4.1 and newer.
*/
class MySQLClient : protected NetService
{
public:
  ///Instantiates the MySQL client
  MySQLClient();
  virtual ~MySQLClient();
  
  //High Level setup functions
  
  ///Opens a connection to a server
  /**
  Opens a connection to the server host using the provided username, password passowrd and selecting database
  On completion of this call (and any further one), the callback set in parameter is fired with the result of that command in parameter
  @param host : server
  @param user : username
  @param db : database to use
  @param pMethod : callback to call on each request completion
  */
  MySQLResult open(Host& host, const string& user, const string& password, const string& db, void (*pMethod)(MySQLResult)); //Non blocking
  
  ///Opens a connection to a server
  /**
  Opens a connection to the server host using the provided username, password passowrd and selecting database
  On completion of this call (and any further one), the callback set in parameter is fired with the result of that command in parameter
  @param host : server
  @param user : username
  @param db : database to use
  @param pItem : callback's class instance
  @param pMethod : callback's method to call on each request completion
  */
  template<class T> 
  MySQLResult open(Host& host, const string& user, const string& password, const string& db, T* pItem, void (T::*pMethod)(MySQLResult)) //Non blocking
  {
    setOnResult(pItem, pMethod);
    setup(host, user, password, db);
    return MYSQL_PROCESSING;
  }
  
  
  ///Executes an SQL command
  /**
  Executes an SQL request on the SQL server
  This is a non-blocking function
  On completion, the callback set in the open function is fired with the result of the command in parameter
  @param sqlCommand SQL request to execute
  */
  MySQLResult sql(string& sqlCommand);
  
  ///Closes the connection to the server
  MySQLResult exit();
      
  void setOnResult( void (*pMethod)(MySQLResult) );
  class CDummy;
  template<class T> 
  void setOnResult( T* pItem, void (T::*pMethod)(MySQLResult) )
  {
    m_pCb = NULL;
    m_pCbItem = (CDummy*) pItem;
    m_pCbMeth = (void (CDummy::*)(MySQLResult)) pMethod;
  }
  
  ///Setups timeout
  /**
  @param ms : time of connection inactivity in ms after which the request should timeout
  */
  void setTimeout(int ms);
  
  virtual void poll(); //Called by NetServices
  
protected:
  void resetTimeout();
  
  void init();
  void close();
  
  void setup(Host& host, const string& user, const string& password, const string& db); //Setup connection, make DNS Req if necessary
  void connect(); //Start Connection

  void handleHandshake();
  void sendAuth();
  
  void handleAuthResult();
  void sendAuth323();
  
  void sendCommand(byte command, byte* arg, int len);
  void handleCommandResult();
  
  void readData(); //Copy to buf
  void writeData(); //Copy from buf

  void onTCPSocketEvent(TCPSocketEvent e);
  void onDNSReply(DNSReply r);
  void onResult(MySQLResult r); //Called when exchange completed or on failure
  void onTimeout(); //Connection has timed out
  
private:
  CDummy* m_pCbItem;
  void (CDummy::*m_pCbMeth)(MySQLResult);
  
  void (*m_pCb)(MySQLResult);
  
  TCPSocket* m_pTCPSocket;

  Timer m_watchdog;
  int m_timeout;
  
  DNSRequest* m_pDnsReq;
  
  bool m_closed;
  
  enum MySQLStep
  {
   // MYSQL_INIT,
    MYSQL_HANDSHAKE,
    MYSQL_AUTH,
    MYSQL_COMMANDS,
    MYSQL_CLOSED
  };
  
  //Parameters
  Host m_host;
  
  string m_user;
  string m_password;
  string m_db;

  //Low-level buffers & state-machine
  MySQLStep m_state;
  
  byte* m_buf;
  byte* m_pPos;
  int m_len;
  int m_size;
  
  int m_packetId;
 
};

#endif
