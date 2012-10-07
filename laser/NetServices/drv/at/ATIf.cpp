
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

#include "ATIf.h"
#include "mbed.h"
#include <cstdarg>

#define READ_TIMEOUT 100
#define TMP_BUF_SIZE 128//512

#define SERIAL_BUF_LEN 512 //Huge buf needed for PPP (esp. when transferring big data chunks, using TCP)

#define BAUDRATE 9600//9600//115200// 19200

#include "netCfg.h"
#if NET_GPRS

//#define __DEBUG
#include "dbg/dbg.h"

ATIf::ATIf() : SerialBuf(SERIAL_BUF_LEN), m_isOpen(false)//, m_signalsEnable(false), m_isOpen(false), m_pCurrentSignal(NULL), m_signals() 
{
  DBG("New AT If@%p\n", this);

  m_readTimeout = READ_TIMEOUT; //default 1s
  //tmpBuf = NULL;
  m_lineMode = false;
  m_tmpBuf = new char[TMP_BUF_SIZE];
}

ATIf::~ATIf()
{
  if(m_tmpBuf)
    delete[] m_tmpBuf;
}

int ATIf::printf(const char* format, ... )
{

/*if(!m_tmpBuf)
    m_tmpBuf = new char[TMP_BUF_SIZE]; //is it really necessary ??*/
  *m_tmpBuf=0;
     
  int len = 0;
  
  //
//  flushBuffer();
//wait(1);
  //
    
  va_list argp;
  
  va_start(argp, format);
  len += vsprintf(m_tmpBuf, format, argp);
  va_end(argp);
  
  //DBG("\r\nOutBuf is : %s, mode is %d.", m_tmpBuf, m_lineMode);
  
  int err = write( m_tmpBuf, m_lineMode );
  if (err<0)
    return 0;
    
  return len;
  
}

int ATIf::scanf(const char* format, ... )
{
/*if(!m_tmpBuf)
    m_tmpBuf = new char[TMP_BUF_SIZE];*/
  int err = read( m_tmpBuf, TMP_BUF_SIZE - 1,  m_readTimeout, m_lineMode, 1/*Ensure at least one char is read*/ );
  if (err<0)
    return -1;//EOF
    
  DBG("Scanf'ing:\r\n%s\r\n",m_tmpBuf);
    
  int len = 0;
  
  if(strchr(format,'%')) //Ugly, determines wether format string is null or not
  {
    va_list argp;
    
    va_start(argp, format);
    len += vsscanf(m_tmpBuf, format, argp);
    va_end(argp);
  }
  else //No varargs, call strncmp
  {
 /*   if(strlen(m_tmpBuf) == 0 )
      return -1;*/
    if( !strncmp(m_tmpBuf, format, strlen(format)) )
    {
      return 0;
    }
    else
    {
      return -1;
    }
  }
  
  return len;
  
}

void ATIf::setTimeout(int timeout) //used by scanf
{
  m_readTimeout = timeout;
}

void ATIf::setLineMode(bool lineMode) //Switch btw line & raw fns
{
  m_lineMode = lineMode;
}

#if 0
void ATIf::setSignals(bool signalsEnable)
{
  m_signalsEnable=signalsEnable;
}
#endif

#if 0
template<class T>
void ATIf::attachSignal( const char* sigName, T* pItem, bool (T::*pMethod)(ATIf*, bool, bool*) ) //Attach Signal ("Unsollicited response code" in Telit_AT_Reference_Guide.pdf) to an handler fn
{
  ATSigHandler sig(sigName, (ATSigHandler::CDummy*)pItem, (bool (ATSigHandler::CDummy::*)(ATIf*, bool, bool*))pMethod);
  m_signals.push_back(sig);
}
#endif

#if 0
void ATIf::detachSignal( const char* sigName )
{
  list<ATSigHandler>::iterator it;
  
  for ( it = m_signals.begin(); it != m_signals.end(); it++ )
  {
    if( !strcmp((*it).m_name,sigName) )
    {
      m_signals.erase(it);
      break;
    }
  }
}
#endif

ATErr ATIf::open(Serial* pSerial) //Deactivate echo, etc
{
  DBG("Opening...\n");
  m_isOpen = true; //Must be set so that the serial port-related fns work
  //Setup options  
//  pSerial->baud(BAUDRATE); //FIXME
  SerialBuf::attach(pSerial);

  setReadMode(false); //Discard chars
  setTimeout(1000);
  setLineMode(true); //Line Mode
  
  DBG("Trmt...\n");
 // printf("AT+IPR=%d", BAUDRATE); //FIXME
  printf("ATZ"); //Reset
  wait(.100);
  printf("ATE"); //Deactivate echo
  wait(.500);
  flushBuffer();
  
  DBG("ATZ ATE...\n");
  
  int len = writeLine("ATV1");
  ATErr err = AT_OK;
  if(len<0)
    err=(ATErr)len;
    
  if(!err)
  {
    err = checkOK();
    if (err) //No ACK from module
    {
      DBG("\r\nOpening port, error %d.", err);
      if(err==AT_TIMEOUT)
        err = AT_NOANSWER;
    }
  }
  
  if(err)
  {
    SerialBuf::detach();
    m_isOpen = false;
    return err;
  }
  
  DBG("\r\nNo error.");
  #if 0//FIXME
  m_signalsEnable = true;
  #endif
  //FIXME:
//  m_pSerial->attach<ATIf>(this, &ATIf::onSerialInterrupt);
  
  return AT_OK;
}

#if NET_USB_SERIAL
ATErr ATIf::open(UsbSerial* pUsbSerial) //Deactivate echo, etc
{
  DBG("Opening...\n");
  m_isOpen = true; //Must be set so that the serial port-related fns work
  //Setup options  
  SerialBuf::attach(pUsbSerial);

  setReadMode(false); //Discard chars
  setTimeout(1000);
  setLineMode(true); //Line Mode

  printf("ATZ"); //Reinit
  wait(.500);  
  //flushBuffer();
//  printf("ATE0 ^CURC=0"); //Deactivate echo & notif
  printf("ATE0"); //Deactivate echo & notif
  wait(.500);
  flushBuffer();
  
  DBG("ATZ ATE...\n");
  
  int len = writeLine("ATQ0 V1 S0=0 &C1 &D2 +FCLASS=0");//writeLine("ATQ0 V1 S0=0 &C1 &D2 +FCLASS=0");
  ATErr err = AT_OK;
  if(len<0)
    err=(ATErr)len;
    
  if(!err)
  {
    err = checkOK();
    if (err) //No ACK from module
    {
      DBG("Opening port, error %d.\n", err);
      if(err==AT_TIMEOUT)
        err = AT_NOANSWER;
    }
  }
  
  if(err)
  {
    SerialBuf::detach();
    m_isOpen = false;
    return err;
  }
  
  DBG("No error.\n");
  #if 0//FIXME
  m_signalsEnable = true;
  #endif
  //FIXME:
//  m_pSerial->attach<ATIf>(this, &ATIf::onSerialInterrupt);
  
  return AT_OK;
}
#endif

ATErr ATIf::close() //Release port
{
  SerialBuf::detach(); //Detach serial buf
  m_isOpen = false;
  //m_signalsEnable = false;
  return AT_OK;
}

ATErr ATIf::flushBuffer()
{
  if(!m_isOpen)
    return AT_CLOSED;
  
  int len=0;
  //char c;
  while(readable())
  {
    //DBG("Readable\n"); 
    /*c =*/ getc();
    //DBG("\r\n[%c] discarded.", c);    
 //   wait(0.01);
    len++;
  }
  
  DBG("\r\n%d chars discarded.", len);
  
  return AT_OK;
}

ATErr ATIf::flushLine(int timeout)
{
  if(!m_isOpen)
    return AT_CLOSED;
    
  Timer timer;
  
  timer.start();
  
  int len=0;
  char c=0;
  while(true)
  {
    while(!readable())
    {      if(timer.read_ms()>timeout)
      {
 //       DBG("Timeout!!0");
        return AT_TIMEOUT;
      }
    }
    if(c=='\x0D')
    {
      c = getc();
      len++;
      if(c=='\x0A')
        break;
    }
    else
    {
      c = getc();
      len++;
    }
  }
  
//  DBG("\r\n%d chars discarded.", len);
  
  return AT_OK;
}

#if 0
bool ATIf::onRead()
{
  if(!m_signalsEnable)
    return false;

  //Save Usermode params
  volatile int u_readTimeout = m_readTimeout;
  volatile bool u_lineMode = m_lineMode;
//  bool u_isOpen = m_isOpen;
  SerialBuf::setReadMode(true);
  
  m_readTimeout = 0; //No timeout in an interrupt fn!
  
  bool handled;
  if(!!flushLine(0))
  {
    SerialBuf::resetRead();
    //Not a complete line here, wait...
    handled = false;
  }
  else
  {
   SerialBuf::resetRead();
   handled = true; 
   if( handleSignal() ) //Was that a signal ?
    {
      //OK, discard data since it has been processed
      SerialBuf::flushRead();
    }
    else
    {
      //Keep data since it has not been processed yet
      //Have to be processed in usermode
      SerialBuf::resetRead();
//      handled = false; 
    }
  }  
  //Restore Usermode params
  m_readTimeout = u_readTimeout;
  m_lineMode = u_lineMode;
  //m_isOpen = u_isOpen;
  return handled;
}
#endif

ATErr ATIf::rawOpen(Serial* pSerial, int baudrate) //Simple open function for similar non-conforming protocols
{
  DBG("\r\nOpening...");
  m_isOpen = true; //Must be set so that the serial port-related fns work
  //Setup options  
  pSerial->baud(baudrate);
  SerialBuf::attach(pSerial);

  return AT_OK;
}

#if 0
ATErr ATIf::command(const char* cmd, char* result, int resultLen, int timeout) ////WARN/FIXME: result has to be long enough!!!
{  
  if(!m_isOpen)
    return AT_CLOSED;
  
  flushBuffer();
  
  int err;
  err = writeLine(cmd);
  
  if(err<0)
    { m_receiveStatus = AT_READY; return (ATErr)err; }
    
  err = readLine(result, resultLen, timeout);
  
  if(err<0)
    { m_receiveStatus = AT_READY; return (ATErr)err; }
      
  m_receiveStatus = AT_READY;
    
  return AT_OK;
  
}
#endif

ATErr ATIf::write(const char* cmd, bool lineMode /*= false*/)
{
  if(!m_isOpen)
    return AT_CLOSED;

  int err;  
  err = lineMode ? writeLine(cmd) : writeRaw(cmd);

  if(err<0)
    return (ATErr)err;
      
  return AT_OK;
}


ATErr ATIf::read(char* result, int resultMaxLen, int timeout, bool lineMode /*= false*/, int resultMinLen/* = 0*/)
{
  if(!m_isOpen)
    return AT_CLOSED;
  
  int err;  
  err = lineMode ? readLine(result, resultMaxLen, timeout) :  readRaw(result, resultMaxLen, timeout, resultMinLen);
  
  if(err<0)
    return (ATErr)err;
    
  return AT_OK;
}

bool ATIf::isOpen()
{
  return m_isOpen;
}

ATErr ATIf::checkOK() //Helper fn to quickly check that OK has been returned
{
  char ret[16] = {0};
  int err = readLine(ret,16,m_readTimeout);
  
  if(err<0)
  {
    DBG("\r\nError in check (%s).\r\n", ret);
    flushBuffer(); //Discard anything in buf to avoid misparsing in the following calls
    return (ATErr)err;
  }
  
  if(!!strcmp("OK",ret))
  {
    DBG("\r\nNot an OK <%s>.\r\n", ret);
    flushBuffer();
    return AT_ERROR;
  }
  
  DBG("\r\nCHECK OK\r\n");
  
  return AT_OK;
}

#if 0
void ATIf::onSerialInterrupt() //Callback from m_pSerial
{
return;//FIXME

  if(m_receiveStatus == AT_READING)
    return;

  if( m_cbObj && m_cbMeth )
    return (m_cbObj->*m_cbMeth)();
}
#endif

int ATIf::readLine(char* line, int maxLen, int timeout) //Read a single line from serial port, return length or ATErr(<0)
{
#ifdef OLDREADLINE
  if(!m_isOpen)
    return AT_CLOSED;
    
  int len = 0;
  
  Timer timer;
  
  timer.start();
#ifdef __START_CLRF_MANDAT
  for( int i=0; i<2; i++ )
  {
    while(!readable())
    {
      if(timer.read_ms()>timeout)
      {
//        DBG("Timeout!!0");
        return AT_TIMEOUT;
      }
      wait_ms(10); //Wait 10ms
    }
    *line = getc();
  //  DBG("In readLine(), read : %c", *line);
    if( ( (i == 0) && (*line!='\x0D') )
     || ( (i == 1) && (*line!='\x0A') ) )
     return AT_PARSE;
  }
#else
  
#endif

  for( ; len < maxLen ; len++ )
  {
    timer.reset();
    while(!readable())
    {
      if(timer.read_ms()>timeout)
      {
//        DBG("Timeout!!1");
        return AT_TIMEOUT;
      }
      wait_ms(10); //Wait 10ms
    }
    *line = getc();
    //DBG("In readLine(), read : %c", *line);

    if(*line=='\x0D')
    {
      timer.reset();
      while(!readable())
      {
        if(timer.read_ms()>timeout)
        {
          return AT_TIMEOUT;
        }
        wait_ms(10); //Wait 1ms
      }
      *line = getc();
    //  DBG("In readLine(), read : %c", *line);
      if(*line=='\x0A')
      {
        if(len==0)
        {
          //Start of line
          len--;
          continue;
        }
        else
        {
          *line=0; //End of line
          break;
        }
      }
      else
      {
        //Should not happen, must have lost some bytes somewhere or non AT protocol
        return AT_PARSE;
      }
    }
    line++;
  }
  
  if(len==maxLen)
    return AT_INCOMPLETE; //Buffer full, must call this method again to get end of line
  
  return len;
#else
 if(!m_isOpen)
    return AT_CLOSED;
  
  Timer timer;  
  timer.start();

  int len = 0;
  while( len < maxLen )
  {
    timer.reset();
    while(!readable())
    {
      if(timer.read_ms()>timeout)
      {
        return AT_TIMEOUT;
      }
      wait_ms(10); //Wait 10ms
    }
    *line = getc();

    if( (*line=='\x0D') || (*line=='\x0A') )
    {
    
      if(len==0)
      {
        //Start of line
        continue;
      }
      else
      {
        *line=0; //End of line
        break;
      }
    }
    len++;  
    line++;
  }
  
  if(len==maxLen)
    return AT_INCOMPLETE; //Buffer full, must call this method again to get end of line
  
  return len;
#endif
}

int ATIf::writeLine(const char* line) //Write a single line to serial port
{
//  char* line = (char*) _line;
  if(!m_isOpen)
    return AT_CLOSED;
  
//  DBG("\n\rIn writeline.");
  
  int len = 0;

  while(*line)
  {
    putc(*line);
    line++;
    len++;
  }
  
 /* putc('\r');
  
    putc('\n');*/
  
  putc('\x0D');
//  putc('\x0A');

// DBG("\n\rWritten %d + 1", len);
  
  return len;
 
}


    
int ATIf::readRaw(char* str, int maxLen, int timeout /*= 0*/, int minLen /*= 0*/) //Read from serial port in buf
{
  if(!m_isOpen)
    return AT_CLOSED;
    
  int len = 0;
  
  Timer timer;
  
  timer.start();

  for( ; len < maxLen ; len++ )
  {
    while( (len < minLen) && !readable())
    {
      if(timer.read_ms()>timeout)
      {
        return AT_TIMEOUT;
      }
      wait(.01); //Wait 10ms
    }
    
    if(!readable()) //Buffer read entirely
      break;
      
    *str = getc();
    str++;
    len++;
  }
  
  *str = 0; //End char
  
  return len;
  
}

int ATIf::writeRaw(const char* str) //Write directly to serial port    
{
  if(!m_isOpen)
    return AT_CLOSED;
    
  int len = 0;

  while(*str)
  {
    putc(*str);
    str++;
    len++;
  }
  
  return len;
}    

#if 0
bool ATIf::handleSignal()
{
  bool beg = false;
  
//  SerialBuf::setReadMode(true); //Keep chars in buf when read  
//  SerialBuf::resetRead();
   
  //if( !m_pCurrentSignal ) //If no signal asked for this line
  if(true) //Check anyway, could have been some parsing error before
  {
    //Extract Signal Name
    char sigName[32]; //Should not be longer than that
    setLineMode(true); //Read one line  

    int len = scanf("%[^:]:%*[^\n]", sigName);
    if(len != 1)
      return false; //This is not a signal      
 //   DBG("\r\nGot signal %s\r\n", sigName);
  
    list<ATSigHandler>::iterator it;

    for ( it = m_signals.begin(); it != m_signals.end(); it++ )
    {
      if( !strcmp((*it).m_name, sigName) )
      {
  //      DBG("\r\nFound signal %s\r\n", sigName);
        m_pCurrentSignal = &(*it);
        beg = true;
        break;
      }
    }
    
    
  }
  
  if( !m_pCurrentSignal ) 
    return false; //This is not a signal or it cannot be handled
    
  bool moreData = false;
  //Call signal handling routine
  SerialBuf::resetRead(); //Rollback so that the handling fn can call scanf properly
  bool result = ((m_pCurrentSignal->m_cbObj)->*(m_pCurrentSignal->m_cbMeth))(this, beg, &moreData);
  
  if( !moreData ) //Processing completed
  {
    m_pCurrentSignal = NULL;
  }
  
  return result;
}
#endif

#endif
