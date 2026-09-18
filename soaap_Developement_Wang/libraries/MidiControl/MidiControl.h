//-----------------------------------------------------------------------------
// Thema:   Social Manufacturing Network / Development Environment
// Datei:   MidiControl.h
// Editor:  yifan wang
//-----------------------------------------------------------------------------
// Datum:   25. Dez 2022
//



#ifndef MidiControl_h
#define MidiControl_h

#include "arduinoDefs.h"
#include "ComRingBuf.h"

#define MaxNrNotesSim   1 // erstmal nur ein Sensor
#define MaxMidiSeq      (2 * MaxNrNotesSim + 1)


// ----------------------------------------------------------------------------
//                            M i d i C o n t r o l
// ----------------------------------------------------------------------------
//
class MidiControl
{
#define next(x) nextState = &MidiControl::x

public:

  // -------------------------------------------------------------------------
  // Öffentliche Datentypen
  // -------------------------------------------------------------------------
  //

  typedef enum _ContollerNr
  {
    Controller16 = 0x10,    //General Purpose 1 (coarse)
    Controller17 = 0x11,    //General Purpose 2 (coarse)
    Controller18 = 0x12,    //General Purpose 3 (coarse)
    Controller19 = 0x13,    //General Purpose 4 (coarse)

    Controller20 = 0x14,
    Controller21 = 0x15,
    Controller22 = 0x16,
    ControllerLast

  } ControlNr;


private:
  // -------------------------------------------------------------------------
  // Private Datentypen
  // -------------------------------------------------------------------------
  //

  typedef void (MidiControl::*cbVector)(void);

  typedef struct _ControlChange
  {
    byte mode;
    byte ControlNummer;
    byte value;
    int state;
  } ControlChange, *ControlChangePtr;

  typedef struct _NewControlChange
  {
    bool newVal;
    byte value;
    byte ControlNummer;
  } NewControlChange, *NewControlChangePtr;


#define ControlChangeModeEmpty     0x00
#define ControlChangeModeRun       0x01
#define ControlChangeModeDoChange  0x02

  // -------------------------------------------------------------------------
  // Lokale Daten
  // -------------------------------------------------------------------------
  //

  IntrfBuf *crb;
  cbVector nextState;

  dword runCounter;
  dword cycleCnt;

  ControlChange controlChangeData;  // alle CC Message 16...19 20...22 uzw.
  byte chn;
  byte controlChangeSeq[MaxMidiSeq];

  ControlChangePtr controlChangePtr;

  dword abspause;
  NewControlChange newControlChangeData;

  bool stopRun;
  bool stoppedRun;


  // -------------------------------------------------------------------------
  // Lokale Funktion
  // -------------------------------------------------------------------------
  //

  // Zustandsmaschine
  // ----------------------

  void smInit();
  void smIdle();

  void smControlChangeSend();
  void smPause();

public:
  // --------------------------------------------------------------------------
  // Initialisierungen
  // --------------------------------------------------------------------------
  void begin(IntrfBuf *inCRB);
  
  // ----------------------------------------------------------------------------
  // Konfiguration
  // ----------------------------------------------------------------------------
  //

  bool addControlChange(ControlNr cNr, byte val);
  void setChannel(int chnVal);

  // --------------------------------------------------------------------------
  // Betrieb
  // --------------------------------------------------------------------------
  //

  void setControlChange(ControlNr cNr, byte val);

  // --------------------------------------------------------------------------
  // Steuerung, Zustandsmaschine
  // --------------------------------------------------------------------------
  //
  void run();
  void stop();
  void resume();

  // --------------------------------------------------------------------------
  // Debugging
  // --------------------------------------------------------------------------
  //

};




// ----------------------------------------------------------------------------
#endif // MidiNotes_h