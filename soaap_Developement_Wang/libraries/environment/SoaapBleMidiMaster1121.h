// ----------------------------------------------------------------------------
//                              SoaapBleMidiMaster.h
// Beispielhafte Anwendung SOAAP / Steuerung optischer und akustischer Ausgaben
//      Kommunikation über BLE-Funkanäle mit Bewerbungstelegrammen
//                        P o l l i n g - M a s t e r
// ----------------------------------------------------------------------------
// Editor:  Robert Patzke
// URI/URL: www.mfp-portal.de
//-----------------------------------------------------------------------------
// Lizenz:  CC-BY-SA  (wikipedia: Creative Commons)
// Datum:   26. April 2022
// Letzte Bearbeitung: 15. März 2022
//

#ifndef SoaapBleMidiMaster1121_h
#define SoaapBleMidiMaster1121_h

// #define DebugTerminal
// #define MeasMuseDebug

// Vordefinitionen, Festlegungen zur Kompilierung
//

//#define DebugTerminal
// Mit dieser Definition werden die Klasse Monitor und weitere Testmethoden
// eingebunden, womit ein anwendungsorientiertes Debugging möglich ist
// Diese Definition wird in der Projektumgebung des Entwicklungssystems gesetzt
// (z.B. Arduino Compile Options bei Eclipse/Sloeber)

//#define TEST001
// Ausgaben an serielle schnittstelle zur Prüfung der ap-Zustandsmaschine
#define testOut(x)    smnSerial.print(x)


#include  "LoopCheck.h"
#include  "StateMachine.h"
#include  "nRF52840Radio.h"
#include  "MidiNotes.h"
#include  "MeasMuse.h"
#include  "BlePoll.h"
#include  "ComRingBuf.h"
#include  "nRF52840Ser.h"
#include  "Monitor.h"

// ----------------------------------------------------------------------------
// Vorwärtsreferenzen
// ----------------------------------------------------------------------------
//
void apInit();
void apWaitDE();
void apWaitMeas();
void apProcMeas();
void apCheckValues();
void apCalcResult();
void apSetResult();
void apTestController();

void setParM1();
void setParM2();
void setParP1(int chn);


#ifdef DebugTerminal
// ----------------------
void smInit() ;
void smCheckJobs() ;
void smDebDword() ;
void smCtrlPolling() ;
void smWaitPolling() ;
void smReadPollValues() ;
void smCheckApp();
void smMode4Slave();
void smMode4SlaveIn();
// ----------------------
#endif

#endif SoaapBleMidiMaster_h
