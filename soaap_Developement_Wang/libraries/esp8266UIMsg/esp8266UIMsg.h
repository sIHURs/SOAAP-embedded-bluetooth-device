//-----------------------------------------------------------------------------
// Thema:   Social Manufacturing Network / Development Environment
// Datei:   esp8266UIMsg.h
// Editor:  Yifan Wang 
// Datum:   14. Dez 2022
//-----------------------------------------------------------------------------


#ifndef _esp8266UIMsg_h
#define _esp8266UIMsg_h



#include "arduinoDefs.h"
#include "MeasMuse.h"
#include "ComRingBuf.h"

#define NrOfChannelsMM  16




class esp8266UIMsg
{

  #define next(x) nextState = &esp8266UIMsg::x


  // -------------------------------------------------------------------------
  // Serial Test
  // -------------------------------------------------------------------------
  //
  // Lokale Variablen 
  private:

    typedef void (esp8266UIMsg::*cbVector)(void);


  private:

    IntrfBuf *crb;
    cbVector nextState;

    dword runCounter;
    dword cycleCnt;

    byte *readBuffer[256];

    bool stopRun;
    bool stoppedRun;
  
  // -------------------------------------------------------------------------
  // Lokale Funktionen
  
  // Zustandsmaschine

    void smInit(); // default Konfig
    void smWait(); // weiss nocht nicht
    void smReceive(); // emgfaengen
    void smParse(); 
    void smError(); // 

public:

  void begin(IntrfBuf *inCRB);
  void run();
  void stop();
  void resume();






















    
public:
  // -------------------------------------------------------------------------
  // Globale Datentypen
  // -------------------------------------------------------------------------
  //

    

private:

  // -------------------------------------------------------------------------
  // Private Datentypen
  // -------------------------------------------------------------------------
  //

  typedef void (esp8266UIMsg::*cbVector)(void);

  typedef struct KonfigParameter
  { 
    int UIimportchannel;

    float UIimportoffset;
    float UIimportmin;
    float UIimportmax;

    byte UIimportlow;
    byte UIimporthigh;
    
    Meas2Midi UIimportaimRoll;
    Meas2Midi UIimportaimPitch;
    Meas2Midi UIimportaimYaw;
    Meas2Midi UIimportmidi;

    MeasMap UIimportmapRoll;
    MeasMap UIimportmapPitch;
    MeasMap UIimportmapYaw;

    //MidiResultPtr UIimportrefResult;
  } KonfigParameter, *KonfigParameterPtr;


  KonfigParameter konfigParameter;
  

  // --------------------------------------------------------------------------
  // Lokale Funktionen
  // --------------------------------------------------------------------------
  //

  void getValue();

public:
  // Anwendungsfunktion... 

  int getKonfig(int slNr, byte *dest);


};

#endif 