
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

#include "EmailMessage.h"

#include <stdio.h>
#include <stdarg.h>

#define BUF_SIZE 512

EmailMessage::EmailMessage() : m_from(), m_lTo(), m_content() {
}

EmailMessage::~EmailMessage() {
}

void EmailMessage::setFrom(const char* from) {
    m_from = from;
}

void EmailMessage::addTo(const char* to) {
    m_lTo.push_back(to);
}

void EmailMessage::clearTo() {
    m_lTo.clear();
}

int EmailMessage::printf(const char* format, ... ) { // Can be called multiple times to write the message

    char buf[BUF_SIZE] = {0};
    int len = 0;

    va_list argp;

    va_start(argp, format);
    len += vsnprintf(buf, BUF_SIZE, format, argp);
    va_end(argp);

    if (len > 0)
        m_content.append(buf);

    return len;
}

void EmailMessage::clearContent() {
    m_content.clear();
}