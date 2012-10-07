
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
Email message header file
*/

#ifndef EMAIL_MESSAGE_H
#define EMAIL_MESSAGE_H

#include <vector>
using std::vector;

#include <string>
using std::string;

///A simple email message
/**
A class to hold the message addresses and content for sending (with SMTPClient).
*/
class EmailMessage {
public:
    ///Instantiates the email message
    EmailMessage();

    ///Destructor for the email message
    ~EmailMessage();

    ///Set FROM address
    /**
    @param from : email from address
    */
    void setFrom(const char* from);

    ///Add TO address to list of recipient addresses
    /**
    @param host : SMTP server host
    */
    void addTo(const char* to);

    ///Clear TO addresses
    void clearTo();
    
    ///Append text to content of message using printf
    /**
    @param format : printf format followed by ... variables
    
    Can be called multiple times to write the message
    */
    int printf(const char* format, ... );

    ///Clear content previously appended by printf
    void clearContent();

private:
    friend class SMTPClient;
    string m_from;
    vector<string> m_lTo;
    string m_content;

};

#endif