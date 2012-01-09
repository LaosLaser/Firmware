#include "LastScreen.h"

LastScreen::LastScreen()
{
    prevptr = 0;
    name= 1;
}

LastScreen::~LastScreen()
{

}

void LastScreen::set(int myname)
{
    LastScreen* newscreen = new LastScreen;
    newscreen->name = myname;
    newscreen->prevptr = this->prevptr;
    this->prevptr = newscreen;
}

int LastScreen::prev()
{
    LastScreen *old;
    int myname;
    if (this->prevptr != 0) {
        old = this->prevptr;
        this->prevptr = old->prevptr;
        myname = old->name;
        delete old;
        return myname;
     } 
    else 
     {
        return 1;
     }
}
    