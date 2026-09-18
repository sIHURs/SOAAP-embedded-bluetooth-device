//-----------------------------------------------------------------------------
// Thema:   Social Manufacturing Network / Development Environment
// Datei:   MidiControl.cpp
// Editor:  yifan wang
//-----------------------------------------------------------------------------
// Datum:   25. Dez 2022
//


#include "MidiControl.h"


// ----------------------------------------------------------------------------
// Initialisierungen
// ----------------------------------------------------------------------------
//

void MidiControl::begin(IntrfBuf *inCRB)
{
  crb = inCRB;
  stopRun = false;
  stoppedRun = false;

  setChannel(4); //test

  next(smInit);
}


// ----------------------------------------------------------------------------
// Konfiguration - set / add
// ----------------------------------------------------------------------------
//

bool MidiControl::addControlChange(ControlNr cNr, byte val)
{
  ControlChangePtr controlChangePtr;

  controlChangePtr = &controlChangeData;

  if(controlChangePtr->mode  == ControlChangeModeEmpty)
  {
    controlChangePtr->mode = ControlChangeModeRun;
    controlChangePtr->ControlNummer = cNr;
    controlChangePtr->value = val;
  }
  
  return true;
}

void MidiControl::setChannel(int chnVal)
{
  if(chnVal < 1) chnVal = 1;
  if(chnVal > 1) chnVal = 16;
  chn = chnVal - 1; 
}


// ----------------------------------------------------------------------------
// Betrieb
// ----------------------------------------------------------------------------
//

void MidiControl::setControlChange(ControlNr cNr, byte val)
{
  if(cNr >= 16 && cNr < ControllerLast)
  {
    newControlChangeData.ControlNummer = cNr;
  }
  if(val >=0 && val <= 127)
  {
    newControlChangeData.value = val;
  }
  newControlChangeData.newVal = true;
}


// ----------------------------------------------------------------------------
// Steuerung, Zustandsmaschine
// ----------------------------------------------------------------------------
//

void MidiControl::stop()
{
  stopRun = true;
}

void MidiControl::resume()
{
  stopRun = false;
  stoppedRun = false;
}

void MidiControl::run()
{
  runCounter++;
  if(cycleCnt > 0) cycleCnt--;

  if(nextState != NULL)
    (this->*nextState)();
}

void MidiControl::smInit()
{
  next(smIdle);
}

void MidiControl::smIdle()
{
  next(smControlChangeSend);
}

// ============================================================================
// MIDI Control Change 21
// ============================================================================
//

void MidiControl::smControlChangeSend()
{
  int j;

  if(stop || stoppedRun)
  {
    stoppedRun = true;
    return;
  }

  if(crb == NULL)
  {
    next(smIdle);
    return;
  }

  j = 0;
  controlChangePtr = &controlChangeData;

  if(controlChangePtr->mode == ControlChangeModeEmpty)
  {
    next(smIdle);
    return;
  }
  controlChangeSeq[j++] = 0xB0 | chn;

  // die cc im cc-speicher koennen durch aktuelle cc ersetzt werden

  if(newControlChangeData.newVal)
  {
    newControlChangeData.newVal = false;
    controlChangePtr->ControlNummer = newControlChangeData.ControlNummer;
    controlChangePtr->value = newControlChangeData.value;
  }
  controlChangeSeq[j++] = controlChangePtr->ControlNummer;

  controlChangeSeq[j++] = controlChangePtr->value;

  crb->putSeq(controlChangeSeq, j);

  next(smPause);
}

void MidiControl::smPause()
{
  next(smControlChangeSend);
}