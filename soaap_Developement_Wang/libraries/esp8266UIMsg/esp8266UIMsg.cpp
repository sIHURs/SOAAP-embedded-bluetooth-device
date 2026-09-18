//-----------------------------------------------------------------------------
// Thema:   Social Manufacturing Network / Development Environment
// Datei:   esp8266UIMsg.h
// Editor:  Yifan Wang 
// Datum:   16. Dez 2022
//-----------------------------------------------------------------------------

#include "esp8266UIMsg.h"

// ----------------------------------------------------------------------------
// Initialisierungen
// ----------------------------------------------------------------------------
//

void esp8266UIMsg::begin(IntrfBuf *inCRB)
{
    crb = inCRB;

    stopRun = false;
    stoppedRun = false;


    for(int i; i<256; i++)
    {
        *readBuffer[i] = 0x00;
    }

    next(smInit);
}

// ----------------------------------------------------------------------------
// Steuerung, Zustandsmaschine
// ----------------------------------------------------------------------------
//

void esp8266UIMsg::stop()
{
    stopRun = true;
}

void esp8266UIMsg::resume()
{
    stopRun = false;
    stoppedRun = false;
}

void esp8266UIMsg::run()
{
    runCounter++;
    if(cycleCnt > 0) cycleCnt --;

    if(nextState != NULL)
       (this->*nextState)();
}

void esp8266UIMsg::smInit()
{
    next(smIdle);
}

void esp8266UIMsg::smWait()
{
    next(smReceiver);
}


void smReceive()
{
    if(stopRun || stoppedRun)
    {
        stoppedRun = true;
        return;
    }

    if(crb == NULL)
    {
        next(smWait);
        return;
    }

    
    next(smParse);
}
void smParse()
{

    next(smWait);
}
void smError(); // 
