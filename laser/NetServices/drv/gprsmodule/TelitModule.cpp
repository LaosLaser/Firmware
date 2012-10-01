
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

#include "TelitModule.h"

#include "mbed.h"

//#define __DEBUG
#include "dbg/dbg.h"

#include "netCfg.h"
#if NET_GPRS_MODULE

TelitModule::TelitModule(PinName pwrSetPin, PinName pwrMonPin) : m_pwrSetPin(pwrSetPin), m_pwrMonPin(pwrMonPin)
{
  //m_pwrSetPin.write(0);

  //m_pwrMonPin.mode(PullDown);
}

TelitModule::~TelitModule() {

}

bool TelitModule::isOn() //True if on
{ 
  return m_pwrMonPin.read();
}

bool TelitModule::on() //True if OK
{ 
  Timer tmr;
  if(!m_pwrMonPin.read()){
    //On
    DBG("Switching On...\n");
    m_pwrSetPin.write(1);
    wait(1.);
    m_pwrSetPin.write(0);
       
    tmr.start();
    while(!m_pwrMonPin.read())
    {
      wait(.001);
      if(tmr.read() > 2)
      { 
         DBG("ERROR - MUST RESET MODULE\n");
         break;
      }
    }
    tmr.stop();
  }
  wait(.1);
  return m_pwrMonPin.read();
}

bool TelitModule::off() { //True if OK
  Timer tmr;
  if(m_pwrMonPin.read()){
    //Off
    DBG("Switching Off...\n");
    m_pwrSetPin.write(1);
    wait(3.);
    m_pwrSetPin.write(0);
      DBG("Waiting....\n"); 
    tmr.start();
    while(m_pwrMonPin.read())
    {
      wait(.001);
      if(tmr.read() > 15)
      { 
         DBG("ERROR - MUST RESET MODULE\n");
         break;
      }
    }
    tmr.stop();
  }
  return !m_pwrMonPin.read();
}

#endif
