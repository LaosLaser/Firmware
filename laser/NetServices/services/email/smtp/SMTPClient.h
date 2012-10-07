
/*
Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com) y Segundo Equipo

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
SMTP Client header file
*/

#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H

class EmailMessage;

#include "core/net.h"
#include "api/TCPSocket.h"
#include "api/DNSRequest.h"
#include "EmailMessage.h"
#include "mbed.h"

///SMTP client results
enum SMTPResult {
    SMTP_OK, ///<Success
    SMTP_PROCESSING, ///<Processing
    SMTP_DNS, ///<Could not resolve name
    SMTP_PRTCL, ///<Protocol error
    SMTP_TIMEOUT, ///<Connection timeout
    SMTP_DISC ///<Disconnected
};

///SMTP authentication
enum SMTPAuth {
    SMTP_AUTH_NONE, ///<No authentication
    SMTP_AUTH_PLAIN ///<AUTH PLAIN authentication
};
#include "core/netservice.h"

///A simple SMTP Client
/**
The SMTPClient is composed of:
- The actual client (SMTPClient)
- A class (EmailMessage) to hold the message addresses and content for sending
*/
class SMTPClient : protected NetService {
public:
    ///Instantiates the SMTP client
    SMTPClient();

    ///Destructor for the SMTP client
    virtual ~SMTPClient();

    ///Full constructor for the SMTP client
    /**
    @param host : SMTP server host
    @param heloDomain : domain name of client
    @param user : username
    @param password : password
    @param auth : authentication type
    */
    SMTPClient(const Host& host, const char* heloDomain, const char* user, const char* password, SMTPAuth auth);

    ///Set server host
    /**
    @param host : SMTP server host
    */
    void setServer(const Host& host);

    ///Provides a plain authentication feature (Base64 encoded username and password)
    /**
    @param user : username
    @param password : password
    */
    void setAuth(const char* user, const char* password); // Plain authentication

    ///Turns off authentication
    void clearAuth(); // Clear authentication

    ///Set HELO domain (defaults to localhost if not set)
    /**
    @param heloDomain : domain name of client (strictly should be fully qualified domain name)
    */
    void setHeloDomain(const char* heloDomain);

    //High Level setup functions
    ///Sends the message (blocking)
    /**
    @param pMessage : pointer to a message

    Blocks until completion
    */
    SMTPResult send(EmailMessage* pMessage); //Blocking

    ///Sends the message (non blocking)
    /**
    @param pMessage : pointer to a message
    @param pMethod : callback function

    The function returns immediately and calls the callback on completion or error
    */
    SMTPResult send(EmailMessage* pMessage, void (*pMethod)(SMTPResult)); //Non blocking

    ///Sends the message (non blocking)
    /**
    @param pMessage : pointer to a message
    @param pItem : instance of class on which to execute the callback method
    @param pMethod : callback method

    The function returns immediately and calls the callback on completion or error
    */
    template<class T>
    SMTPResult send(EmailMessage* pMessage, T* pItem, void (T::*pMethod)(SMTPResult)) { //Non blocking
        setOnResult(pItem, pMethod);
        doSend(pMessage);
        return SMTP_PROCESSING;
    }

    ///Sends the message (non blocking)
    /**
    @param pMessage : pointer to a message

    The function returns immediately and calls the previously set callback on completion or error
    */
    void doSend(EmailMessage* pMessage);

    ///Setup the result callback
    /**
    @param pMethod : callback function
    */
    void setOnResult( void (*pMethod)(SMTPResult) );

    ///Setup the result callback
    /**
    @param pItem : instance of class on which to execute the callback method
    @param pMethod : callback method
    */
    class CDummy;
    template<class T>
    void setOnResult( T* pItem, void (T::*pMethod)(SMTPResult) ) {
        m_pCb = NULL;
        m_pCbItem = (CDummy*) pItem;
        m_pCbMeth = (void (CDummy::*)(SMTPResult)) pMethod;
    }

    ///Setup timeout
    /**
    @param ms : time of connection inactivity in ms after which the request should timeout
    */
    void setTimeout(int ms);

    ///Gets the last response from the server
    string& getLastResponse();

    virtual void poll(); //Called by NetServices

protected:
    int rc(char* buf); //Return code
    void process(bool writeable); //Main state-machine

    void resetTimeout();

    void init();
    void close();

    void setup(EmailMessage* pMessage); //Setup request, make DNS Req if necessary
    void connect(); //Start Connection

    int  tryRead(); //Read data and try to feed output
    void readData(); //Data has been read
    void writeData(); //Data has been written & buf is free

    void onTCPSocketEvent(TCPSocketEvent e);
    void onDNSReply(DNSReply r);
    void onResult(SMTPResult r); //Called when exchange completed or on failure
    void onTimeout(); //Connection has timed out

    string encodePlainAuth(); // Encode plain authentication (username and password in based 64)
    bool okPostAuthentication(int code); // True if server response is ok following authentication

private:
    SMTPResult blockingProcess(); //Called in blocking mode, calls Net::poll() until return code is available

    CDummy* m_pCbItem;
    void (CDummy::*m_pCbMeth)(SMTPResult);
    void (*m_pCb)(SMTPResult);

    TCPSocket* m_pTCPSocket;

    Timer m_watchdog;
    int m_timeout;

    DNSRequest* m_pDnsReq;
    Host m_server;

    bool m_closed;

    enum SMTPStep {
        SMTP_HELLO,
        SMTP_AUTH,
        SMTP_FROM,
        SMTP_TO,
        SMTP_DATA,
        SMTP_BODY,
        SMTP_BODYMORE,
        SMTP_EOF,
        SMTP_BYE,
        SMTP_CLOSED
    };

    SMTPStep m_state;

    EmailMessage* m_pMessage;

    int m_posInMsg;
    int m_posInCRLF;

    SMTPResult m_blockingResult; //Result if blocking mode
    string m_response;
    int m_responseCode;
    string m_username;
    string m_password;
    SMTPAuth m_auth;
    string m_heloDomain;

    vector<string>::iterator m_To;
};

#endif // SMTP_CLIENT_H