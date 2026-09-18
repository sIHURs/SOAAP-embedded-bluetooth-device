// ----------------------------------------------------------------------------
//                              SoaapBleMaster.h
// Beispielhafte Anwendung SOAAP / Steuerung optischer und akustischer Ausgaben
//      Kommunikation über BLE-Funkanäle mit Bewerbungstelegrammen
//                        P o l l i n g - M a s t e r
// ----------------------------------------------------------------------------
// Editor:  Robert Patzke
// URI/URL: www.mfp-portal.de
//-----------------------------------------------------------------------------
// Lizenz:  CC-BY-SA  (wikipedia: Creative Commons)
// Datum:   1. November 2021
// Letzte Bearbeitung: 15. März 2022
//

#ifndef SoaapBleMaster_h
#define SoaapBleMaster_h

// Vordefinitionen, Festlegungen zur Kompilierung
//
#define DebugTerminal
// Mit dieser Definition werden die Klasse Monitor und weitere Testmethoden
// eingebunden, womit ein anwendungsorientiertes Debugging möglich ist

//#define TEST001
// Ausgaben an serielle schnittstelle zur Prüfung der ap-Zustandsmaschine

#include  "LoopCheck.h"
#include  "StateMachine.h"
#include  "nRF52840Radio.h"
#include  "BlePoll.h"
#include  "ComRingBuf.h"
#include  "nRF52840Ser.h"
#include  "SoaapMsg.h"
#include  "Monitor.h"

// ----------------------------------------------------------------------------
// Vorwärtsreferenzen
// ----------------------------------------------------------------------------
//
void apInit();
void apWaitDE();
void apWaitMeas();
void apProcMeas();


#ifdef DebugTerminal
// ----------------------
void smInit() ;
void smCheckJobs() ;
void smDebDword() ;
void smCtrlPolling() ;
void smWaitPolling() ;
void smReadPollValues() ;
void smCheckSer();
// ----------------------
#endif

#endif /* SoaapBleMaster_h */
