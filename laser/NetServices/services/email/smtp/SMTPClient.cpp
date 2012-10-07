
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
#include "core/netservice.h"
#include "SMTPClient.h"
#include "../util/base64.h"

#define __DEBUG
#include "dbg/dbg.h"

#define CHUNK_SIZE 256 //512
#define BUF_SIZE (CHUNK_SIZE + 1)

#define SMTP_REQUEST_TIMEOUT 15000
#define SMTP_PORT 25

SMTPClient::SMTPClient() : NetService(false) /*Not owned by the pool*/,
        m_pCbItem(NULL), m_pCbMeth(NULL), m_pCb(NULL),
        m_watchdog(), m_timeout(SMTP_REQUEST_TIMEOUT), m_pDnsReq(NULL), m_server(),
        m_closed(true), m_state(SMTP_CLOSED), m_pMessage(NULL),
        m_posInMsg(0), m_blockingResult(SMTP_PROCESSING),
        m_username(""), m_password(""), m_auth(SMTP_AUTH_NONE),
        m_heloDomain("localhost") {
    DBG("New SMTPClient %p\n", this);
}

SMTPClient::SMTPClient(const Host& host, const char* heloDomain, const char* user, const char* password, SMTPAuth auth) : NetService(false),
        m_pCbItem(NULL), m_pCbMeth(NULL), m_pCb(NULL),
        m_watchdog(), m_timeout(SMTP_REQUEST_TIMEOUT), m_pDnsReq(NULL), m_server(host),
        m_closed(true), m_state(SMTP_CLOSED), m_pMessage(NULL),
        m_posInMsg(0), m_blockingResult(SMTP_PROCESSING),
        m_username(string(user)), m_password(string(password)), m_auth(auth),
        m_heloDomain(heloDomain) {
    DBG("New SMTPClient %p\n", this);
}

SMTPClient::~SMTPClient() {
    close();
}

void SMTPClient::setAuth(const char* user, const char* password) { // Plain authentication
    m_username = string(user);
    m_password = string(password);
    m_auth = SMTP_AUTH_PLAIN;
}

void SMTPClient::clearAuth() { // Clear authentication
    m_username = "";
    m_password = "";
    m_auth = SMTP_AUTH_NONE;
}

string SMTPClient::encodePlainAuth() {
    string decStr = m_username;
    decStr += '\0';
    decStr += m_username;
    decStr += '\0';
    decStr += m_password;

    string auth = "AUTH PLAIN ";
    return auth.append(Base64::encode(decStr));
}

void SMTPClient::setHeloDomain(const char* heloDomain) {
    m_heloDomain = string(heloDomain);
}

SMTPResult SMTPClient::send(EmailMessage* pMessage) { //Blocking
    doSend(pMessage);
    return blockingProcess();
}

SMTPResult SMTPClient::send(EmailMessage* pMessage, void (*pMethod)(SMTPResult)) { //Non blocking
    setOnResult(pMethod);
    doSend(pMessage);
    return SMTP_PROCESSING;
}

void SMTPClient::doSend(EmailMessage* pMessage) {
    setup(pMessage);
}

void SMTPClient::setOnResult( void (*pMethod)(SMTPResult) ) {
    m_pCb = pMethod;
    m_pCbItem = NULL;
    m_pCbMeth = NULL;
}

void SMTPClient::setTimeout(int ms) {
    m_timeout = ms;
}

void SMTPClient::poll() { //Called by NetServices
    if ( (!m_closed) && (m_watchdog.read_ms() >= m_timeout) ) {
        onTimeout();
    }
}

void SMTPClient::resetTimeout() {
    m_watchdog.reset();
    m_watchdog.start();
}

void SMTPClient::init() { //Create and setup socket if needed
    close(); //Remove previous elements
    if (!m_closed) //Already opened
        return;
    m_state = SMTP_HELLO;
    m_pTCPSocket = new TCPSocket;
    m_pTCPSocket->setOnEvent(this, &SMTPClient::onTCPSocketEvent);
    m_closed = false;
    m_posInMsg = 0;
    m_posInCRLF = 2;
    m_response = "";
}

void SMTPClient::close() {
    if (m_closed)
        return;
    m_state = SMTP_CLOSED;
    m_closed = true; //Prevent recursive calling or calling on an object being destructed by someone else
    m_watchdog.stop(); //Stop timeout
    m_watchdog.reset();
    m_pTCPSocket->resetOnEvent();
    m_pTCPSocket->close();
    delete m_pTCPSocket;
    m_pTCPSocket = NULL;
    if ( m_pDnsReq ) {
        m_pDnsReq->close();
        delete m_pDnsReq;
        m_pDnsReq = NULL;
    }
}

void SMTPClient::setServer(const Host& host) { //Setup request, make DNS Req if necessary
    m_server = host;
}

void SMTPClient::setup(EmailMessage* pMessage) { //Setup request, make DNS Req if necessary

    init(); //Initialize client in known state, create socket
    m_pMessage = pMessage;
    resetTimeout();

    m_To = m_pMessage->m_lTo.begin(); // Point to first to recipient in TO list

    //If port set to zero then use default port
    if (!m_server.getPort()) {
        m_server.setPort(SMTP_PORT);
        DBG("Using default port %d\n", SMTP_PORT);
    }

    if (m_server.getIp().isNull()) {
        //DNS query required
        m_pDnsReq = new DNSRequest();
        DBG("DNSRequest %p\r\n", m_pDnsReq);
        m_pDnsReq->setOnReply(this, &SMTPClient::onDNSReply);
        m_pDnsReq->resolve(&m_server);
        return;
    } else
        connect();

}

void SMTPClient::connect() { //Start Connection
    resetTimeout();
    DBG("Connecting...\n");
    m_pTCPSocket->connect(m_server);
}

void SMTPClient::onTCPSocketEvent(TCPSocketEvent e) {

    DBG("Event %d in SMTPClient::onTCPSocketEvent()\n", e);

    if (m_closed) {
        DBG("WARN: Discarded\n");
        return;
    }

    switch (e) {
        case TCPSOCKET_READABLE:
            resetTimeout();
            process(false);
            break;
        case TCPSOCKET_WRITEABLE:
            resetTimeout();
            process(true);
            break;
        case TCPSOCKET_CONTIMEOUT:
        case TCPSOCKET_CONRST:
        case TCPSOCKET_CONABRT:
        case TCPSOCKET_ERROR:
            DBG("Connection error in SMTP Client.\n");
            close();
            onResult(SMTP_DISC);
            break;
        case TCPSOCKET_DISCONNECTED:
            if (m_state != SMTP_BYE) {
                DBG("Connection error in SMTP Client.\n");
                close();
                onResult(SMTP_DISC);
            }
            break;
    }
}

void SMTPClient::onDNSReply(DNSReply r) {
    if (m_closed) {
        DBG("WARN: Discarded\n");
        return;
    }

    if ( r != DNS_FOUND ) {
        DBG("Could not resolve hostname.\n");
        close();
        onResult(SMTP_DNS);
        return;
    }

    DBG("DNS Resolved to %d.%d.%d.%d\n", m_server.getIp()[0], m_server.getIp()[1], m_server.getIp()[2], m_server.getIp()[3]);
    //If no error, m_server has been updated by m_pDnsReq so we're set to go !
    m_pDnsReq->close();
    delete m_pDnsReq;
    m_pDnsReq = NULL;
    connect();
}

void SMTPClient::onResult(SMTPResult r) { //Called when exchange completed or on failure
    if (m_pCbItem && m_pCbMeth)
        (m_pCbItem->*m_pCbMeth)(r);
    else if (m_pCb)
        m_pCb(r);
    m_blockingResult = r; //Blocking mode
}

void SMTPClient::onTimeout() { //Connection has timed out
    DBG("Timed out.\n");
    close();
    onResult(SMTP_TIMEOUT);
}

SMTPResult SMTPClient::blockingProcess() { //Called in blocking mode, calls Net::poll() until return code is available
    //Disable callbacks
    m_pCb = NULL;
    m_pCbItem = NULL;
    m_pCbMeth = NULL;
    m_blockingResult = SMTP_PROCESSING;
    do {
        Net::poll();
    } while (m_blockingResult == SMTP_PROCESSING);
    Net::poll(); //Necessary for cleanup
    return m_blockingResult;
}

int SMTPClient::rc(char* buf) { //Parse return code
    int rc;
    int len = sscanf(buf, "%d %*[^\r\n]\r\n", &rc);
    if (len != 1)
        return -1;
    return rc;
}

bool SMTPClient::okPostAuthentication(int code) {
    return (code == 250) || (code == 235);
}

void SMTPClient::process(bool writeable) { //Main state-machine

    DBG("In state %d, writeable %d\n", m_state, writeable);

    // If writeable but nothing to write then return
    if (writeable && (m_state != SMTP_BODYMORE))
        return;

    // If not writeable then read
    char buf[BUF_SIZE] = {0};
    int responseCode = -1;
    if (!writeable) {
        bool firstBuf = true;
        int read;
        do { // read until nothing left but only process the response from first buffer
            read = m_pTCPSocket->recv(buf, BUF_SIZE - 1);
            if (firstBuf) {
                m_response = string(buf);
                responseCode = rc(buf);
                firstBuf = false;
                DBG("Response %d %d | %s", read, responseCode, buf);
            }
        } while (read > 0);
    }

    switch (m_state) {
        case SMTP_HELLO:
            if ( responseCode != 220 ) {
                close();
                onResult(SMTP_PRTCL);
                return;
            }
            char* helloCommand;
            if (m_auth == SMTP_AUTH_NONE) {
                helloCommand = "HELO";
                m_state = SMTP_FROM;
            } else {
                helloCommand = "EHLO";
                m_state = SMTP_AUTH;
            }
            snprintf(buf, BUF_SIZE, "%s %s\r\n", helloCommand, m_heloDomain.c_str());
            break;
        case SMTP_AUTH:
            if ( responseCode != 250 ) {
                close();
                onResult(SMTP_PRTCL);
                return;
            }
            snprintf(buf, BUF_SIZE, "%s\r\n", encodePlainAuth().c_str());
            m_state = SMTP_FROM;
            break;
        case SMTP_FROM:
            if (!okPostAuthentication(responseCode)) {
                close();
                onResult(SMTP_PRTCL);
                return;
            }
            snprintf(buf, BUF_SIZE, "MAIL FROM:<%s>\r\n", m_pMessage->m_from.c_str());
            m_state = SMTP_TO;
            break;
        case SMTP_TO:
            if ( responseCode != 250 ) {
                close();
                onResult(SMTP_PRTCL);
                return;
            }
            snprintf(buf, BUF_SIZE, "RCPT TO:<%s>\r\n", (m_To++)->c_str());
            if (m_To == m_pMessage->m_lTo.end())
                m_state = SMTP_DATA;
            break;
        case SMTP_DATA:
            if ( responseCode != 250 ) {
                close();
                onResult(SMTP_PRTCL);
                return;
            }
            snprintf(buf, BUF_SIZE, "DATA\r\n");
            m_state = SMTP_BODY;
            break;
        case SMTP_BODY:
            if ( responseCode != 354 ) {
                close();
                onResult(SMTP_PRTCL);
                return;
            }
            m_state = SMTP_BODYMORE;
            buf[0] = '\0'; // clear buffer before carrying on into next state
        case SMTP_BODYMORE:
            if (strlen(buf) > 0) { // sending interrupted by a server response
                close();
                onResult(SMTP_PRTCL);
                return;
            }

            if ( m_posInMsg < m_pMessage->m_content.length() ) { // if still something to send
                int sendLen = 0;
                while (sendLen < BUF_SIZE - 1) { // - 1 to allow room for extra dot or CR or LF
                    char c = m_pMessage->m_content.at(m_posInMsg++);
                    switch (c) { // thanks ExtraDotOutputStream.java (with extra check for naked CR)
                        case '.':
                            if (m_posInCRLF == 2) // add extra dot
                                buf[sendLen++] = '.';
                            m_posInCRLF = 0;
                            break;
                        case '\r':
                            if (m_posInCRLF == 1) // two CR in a row, so insert an LF first
                                buf[sendLen++] = '\n';
                            m_posInCRLF = 1;
                            break;
                        case '\n':
                            if (m_posInCRLF != 1) // convert naked LF to CRLF
                                buf[sendLen++] = '\r';
                            m_posInCRLF = 2;
                            break;
                        default:
                            if (m_posInCRLF == 1) { // convert naked CR to CRLF
                                buf[sendLen++] = '\n';
                                m_posInCRLF = 2;
                            } else
                                m_posInCRLF = 0; // we're  no longer at the start of a line
                            break;
                    }
                    buf[sendLen++] = c;
                    if ( m_posInMsg == m_pMessage->m_content.length() )
                        break;
                }
                m_pTCPSocket->send( buf, sendLen );
                DBG("Sending %d bytes of processed message content\n", sendLen);
            } else {
                if (m_posInCRLF == 0)
                    snprintf(buf, BUF_SIZE, "\r\n.\r\n");
                else if (m_posInCRLF == 1)
                    snprintf(buf, BUF_SIZE, "\n.\r\n");
                else
                    snprintf(buf, BUF_SIZE, ".\r\n");
                m_state = SMTP_EOF;
            }
            break;
        case SMTP_EOF:
            if ( responseCode != 250 ) {
                close();
                onResult(SMTP_PRTCL);
                return;
            }
            snprintf(buf, BUF_SIZE, "QUIT\r\n");
            m_state = SMTP_BYE;
            break;
        case SMTP_BYE:
            if ( responseCode != 221 ) {
                close();
                onResult(SMTP_PRTCL);
                return;
            }
            close();
            onResult(SMTP_OK);
            return;
    }

    if ( m_state != SMTP_BODYMORE ) {
        DBG("Sending | %s", buf);
        m_pTCPSocket->send( buf, strlen(buf) );
    }

}

string& SMTPClient::getLastResponse() { // Return last response set on result
    return m_response;
}