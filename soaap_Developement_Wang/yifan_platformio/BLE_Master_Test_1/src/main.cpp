// ----------------------------------------------------------------------------
//                              SoaapBleMaster.ino
// Beispielhafte Anwendung SOAAP / Steuerung optischer und akustischer Ausgaben
//      Kommunikation über BLE-Funkanäle mit Bewerbungstelegrammen
//                        P o l l i n g - M a s t e r
// ----------------------------------------------------------------------------
// Editor:  Robert Patzke
// URI/URL: www.mfp-portal.de
//-----------------------------------------------------------------------------
// Lizenz:  CC-BY-SA  (wikipedia: Creative Commons)
// Datum:   1. November 2021
// Letzte Bearbeitung: 1. November 2021
//
#include "Arduino.h"

#include "LoopCheck.h"
#include "StateMachine.h"
#include "nRF52840Radio.h"
#include "BlePoll.h"
#include "Monitor.h"
#include "SoaapBleMaster.h"  //Eingefügt

#define DebugTerminal
// Mit dieser Definition werden die Klasse Monitor und weitere Testmethoden
// eingebunden, womit ein anwendungsorientiertes Debugging möglich ist

LoopCheck     lc;
// Eine statische Instanz der Klasse LoopCheck
// Darüber wird das Zeitverhalten gesteuert (Software-Timer) und geprüft

#ifdef DebugTerminal
Monitor       mon(modeEcho | modeNl,0,&lc);
byte    tmpByteArray[256];
// Eine statische Instanz (mit Konstruktordaten) der Klasse Monitor
// Darüber wird mit (direkten) Terminals (z.B. VT100) kommuniziert
// Unter Linux werden hier GtkTerm (siehe Internet) und
// ArduinoMonTerm (eigene Entwicklung mit grafischen Wertanzeigen) eingesetzt.
// Das in den IDEs integrierte Terminal ist dafür meistens nicht geeigent,
// weil damit keine direkte Kommunikation (getipptes Zeichen sofort gesendet)
// möglich ist.
// ----- Parameter ------------------------------------------------------------
// <mode Echo>  Alle eintreffenden Zeichen werden sofort zurückgesendet
// <mode NL>    Vor der Ausgabe des Prompt (M>) erfolgt CR/LF
// <0>          Für Speicherzugriffe wird von 32 Bit ARM ausgegangen
// <&lc>        Für Zeitüberwachungen und entsprechende statisctische Daten
//              greift die Monitor-Klasse auf die LoopCheck-Klasse zu
#endif

nRF52840Radio bleCom;
// Eine statische Instanz der Klasse nRF52840Radio
// Darüber wird direkt die CPU (nRF52840) auf dem Arduino-Board zum Senden und
// Empfangen von BLE-Beacons angesprochen.


#define bleCycleTime 150
BlePoll       blePoll((IntrfRadio *) &bleCom, micros);
// Eine statische Instanz der Klasse BlePoll
// Darüber werden das Polling des Masters als auch die Antworten der Slaves
// gesteuert. Es können Geräte entwickelt werden, die nur Master oder Slave
// sind und solche mit Doppelfunktion, wenn kein expliziter Master
// eingesetzt und das Netzwerk über Spontan-Master quasi dezentral
// betrieben werden soll.
// ----- Parameter ------------------------------------------------------------
// <&bleCom>    Die Klasse (vom angenommenen Typ IntrfRadio), die die Daten-
//              übertragung abwickelt. Hier wird eine Instanz von nRF52840Radio
//              angebunden. Für andere Hardware kann eine entsprechende Klasse
//              verwendet werden, die von IntrfRadio abgeleitet wurde.
// <micros>     Eine Funktion, die die verstrichene Zeit in Mikrosekunden gibt.
//              Damit werden Zeiten (z.B. Time-Out) berechnet.
//              Wird hier der Wert NULL übergeben, dann werden die Zeiten aus
//              dem Aufrufzyklus (bleCycleTime) in Mikrosekunden berechnet,
//              was hier einer Auflösung von 500 Mikrosekunden entspricht.

#ifdef DebugTerminal
#define smCycleTime 5
void smInit();  // Vorwärtsreferenz auf die weiter unten definierte Funktion
StateMachine  sm(smInit, NULL, smCycleTime);
// Eine statische Instanz für die Zustandsmaschine, die hier für allgemeine
// Steuerungen, Überwachungen und zum Debugging verwendet wird
// ----- Parameter ------------------------------------------------------------
// <smInit>       Der zuerst aufgerufene Zustand (Funktion). Weitere Zustände
//                werden in den weiteren Zustandsfunktionen eingesetzt.
// <NULL>         Hier kann eine weitere Zustandsfunktion angegeben werden,
//                die dann grundsätzlich vor dem Verzweigen in einen Zustand
//                aufgerufen wird.
// <smCycleTime>  Die Zukluszeit (Takt) der Zustandsmaschine in Millisekunden
#endif

#define NrOfSlavesToPoll  5
// Die Anzahl der Slaves, die der Master aufrufen soll (Polling).
// Es wird grundsätzlich mit der Adresse 1 begonnen und nach dem Aufruf die
// Adresse inkrementiert, also immer die Slaves Adr = 1 bis
// Adr = NrOfSlavesToPoll+1 aufgerufen.
// ACHTUNG!
// Diese Zahl muss kleiner/gleich der in BlePoll.h definierten maximalen
// Anzahl der Slaves (MAXSLAVE) sein, weil die Ressourcen darüber statisch
// festgelegt werden.

// ============================================================================
int durchlaufe=0; //hinzugefügt
void setup()
{
  
  pinMode(LED_BUILTIN,OUTPUT);
  bleCom.begin();           // Initialisierung der Datenübertragung
  //bleCom.setPower(0x08);     // Maximale Sendeleistung bei nRF52840
  // TEST
  bleCom.setPower(0x0FC);   // Reduzierte Sendeleistung beim Schreibtisch-Test

  blePoll.begin(BlePoll::ctMASTER, NrOfSlavesToPoll, BlePoll::atSOAAP, 1000000);
  // Initialisierung des Polling mit folgenden Parametern:
  // <BlePoll::ctMASTER>    Es wird ein Master eingerichtet
  // <NrOfSlavesToPoll>     Anzahl gepollter Slaves (s.o.)
  // <BlePoll::atSOAAP>     Spezielle Anwendung SOAAP
  // <10000>                INT-Watchdog-Timeout beim Lesen in Mikrosekunden

  blePoll.setEmptyPollParams(2000, 500, 2000);
  // Setzen der Parameter für das leere Polling zur Feststellung der
  // vorhandenen Slaves und Aufbau einer Poll-Liste für den Datenaustausch
  // ----- Parameter ----------------------------------------------------------
  // <2000>   Anzahl der Poll-Durchläufe, solange kein Slave gefunden wird
  // <500>    Anzahl weiterer Durchläufe, nachdem wenigstens ein Slave gefunden ist
  // <2000>   Time-Out (Zeit für Slave zum Antworten) in Mikrosekunden

  for(int i = 1; i <= NrOfSlavesToPoll; i++)
    blePoll.setDataPollParams(i, 1, 10, 1000);
  // Setzen der Parameter beim Datenpolling, für Slaves individuell
  // ----- Parameter ----------------------------------------------------------
  // <i>      Adresse des Slave (1-..)
  // <1>      Priorität beim Aufruf, 0 = immer bis max 65535 = sehr selten
  // <10>     minimale Priorität bei automatischer Prioritätsreduzierung
  //          im Fall von Störungen (Time-Out)
  // <1500>   Time-Out (Zeit für Slave zum Antworten) in Mikrosekunden

  // TEST
  blePoll.stopEP();   // Das Polling muss extra gestartet werden
}

// ============================================================================
void loop()
{
  lc.begin();     // Muss am Anfang von LOOP aufgerufen werden
  // --------------------------------------------------------------------------

#ifdef DebugTerminal
  mon.run();      // Der Monitor bekommt bei jedem Durchlauf die CPU

#endif

  // Alle 500 Mikrosekunden erfolgt der Aufruf des Ble-Polling
  //
  if(lc.timerMicro(lcTimer0, bleCycleTime, 0)){
       blePoll.run();      
  }
 
  // ----- Parameter der Software-Timer ---------------------------------------
  // <lcTimer0>   Id/Nummer des Timer, zur Zeit werden bis 10 Timer unterstützt
  //              lcTimer0 bis lcTimer9 (einstellbar in LoopCheck.h)
  // <bleCycleTime>   Ablaufzeit in Einheit des Timer-Typ (Micro/Milli)
  //                  hier in Mikrosekunden (timerMicro)
  // <0>          Anzahl der Wiederholungen, 0 = unbegrenzt

#ifdef DebugTerminal
  // Jede Sekunde erfolgt die Ausgabe einer Versionsmeldung
  // das kann über c0 am Terminal abgeschaltet werden
  //
  if(lc.timerMilli(lcTimer1, 1000, 0))
  {
    if(!mon.cFlag[0]){
       durchlaufe++; //hinzugefügt
       //mon.printcr((char *)"%@TestBleMaster (ttyACM0), Version 20211104\n");
       //mon.println(durchlaufe); //hinzugefügt
       smWaitPolling();
    }
     
  }
  // Die Zeichen %@ am Anfang steuern die Ausgabe bei AndroidMonTerm in ein
  // Textfeld (Label) statt auf das Terminal-Display

  // Alle 5 Millisekunden erfolgt der Aufruf der Zustandsmaschine
  //
  if(lc.timerMilli(lcTimer2, smCycleTime, 0))
  {
    sm.run();
  }
#endif


  // --------------------------------------------------------------------------
  lc.end();       // Muss vor dem Ende von LOOP aufgerufen werden
}

#ifdef DebugTerminal
// ****************************************************************************
// Z u s t a n d s m a s c h i n e
// ****************************************************************************
//
dword   debDword;



void smInit()
{
  sm.enter(smCheckJobs);
}

// ----------------------------------------------------------------------------
// Abfrage der Monitorschalter
// ----------------------------------------------------------------------------
//

void smCheckJobs()
{
  if(mon.cFlag[1] && !mon.busy)
    sm.enter(smDebDword);
  else if(mon.cFlag[2] && !mon.busy)
    sm.enter(smCtrlPolling);
  else if(mon.cFlag[3] && !mon.busy)
    sm.enter(smReadPollValues);
  /*
  else if(mon.cFlag[4] && !mon.busy)
    sm.enter(smCheckSens);
  */
}

// ----------------------------------------------------------------------------
// Debug-Informationen
// ----------------------------------------------------------------------------
//

void smDebDword()
{
  int idx;

  if(sm.firstEnter())
  {
    mon.print((char *) "DebDword[");
    mon.lastKeyIn = ':';
  }

  if(mon.lastKeyIn == ':') return;

  if(mon.lastKeyIn >= 0x30 && mon.lastKeyIn <= 0x39)
  {
    idx = mon.lastKeyIn & 0x0F;
    mon.print(idx);
    mon.print((char *) "]=");
    debDword = blePoll.debGetDword(idx);
    mon.println(debDword);
    sm.resetEnter();
  }
  else
  {
    if(mon.lastKeyIn == ' ')
    {
      mon.cFlag[1] = false;
      mon.print((char *) "-- Schleifenabbruch - drücke Enter");
      sm.enter(smCheckJobs);
    }
  }
}

// ----------------------------------------------------------------------------
// Steuern des Polling-Prozesses
// ----------------------------------------------------------------------------
//

TxStatistics txStatistics;

void smCtrlPolling()
{
  dword   tmpDw;
  short   tmpShort;
  int     i;

  if(sm.firstEnter())
  {
    mon.print((char *) "polling ");
    mon.lastKeyIn = ':';
  }

  if(mon.lastKeyIn == ':') return;

  // --------------------------------------------------------------------------
  if(mon.lastKeyIn == 'P' || mon.lastKeyIn == 'p')
  {
    if(blePoll.stoppedEP())
    {
      blePoll.resumeEP();
      mon.println((char *) "fortgesetzt");
      sm.resetEnter();
    }
    else
    {
    blePoll.stopEP();
    sm.enter(smWaitPolling);
    }
  }

  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'C' || mon.lastKeyIn == 'c')
  {
    blePoll.resetPollCounters();
    mon.println((char *) "Zähler zurückgesetzt");
    sm.resetEnter();
  }


  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'R' || mon.lastKeyIn == 'r')
  {
    mon.print((char *) "Radiopuffer = ");
    bleCom.getPduMem(tmpByteArray, 0, 10);
    mon.println(tmpByteArray, 10, ' ');
    sm.resetEnter();
  }


  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'S' || mon.lastKeyIn == 's')
  {
    mon.print((char *) "Sendepuffer = ");
    bleCom.getPduSent(tmpByteArray, 0, 10);
    mon.println(tmpByteArray, 10, ' ');
    sm.resetEnter();
  }

  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'L' || mon.lastKeyIn == 'l')
  {
    mon.print((char *) "Slave-Liste: ");
    int nrOfSlaves = blePoll.getSlaveList(tmpByteArray, 255);
    mon.println(tmpByteArray, nrOfSlaves, ',');
    sm.resetEnter();
  }

  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'T' || mon.lastKeyIn == 't')
  {
    mon.print((char *) "TxStat [");
    dword bleStat = blePoll.getStatistics(&txStatistics);
    mon.print(bleStat);
    mon.print((char *) "] ");
    mon.print(txStatistics.mode); mon.cprint(' ');
    mon.print(txStatistics.interrupts); mon.cprint(' ');
    mon.print(txStatistics.recs); mon.cprint(' ');
    mon.print(txStatistics.sendings); mon.cprint(' ');
    mon.print(txStatistics.aliens); mon.cprint(' ');
    mon.print(txStatistics.wrongs); mon.cprint(' ');
    mon.print(txStatistics.pollAcks); mon.cprint(' ');
    mon.print(txStatistics.pollNaks); mon.cprint(' ');
    mon.print(txStatistics.crcErrors); mon.print("  s[ ");
    mon.print(txStatistics.memDumpSnd,8,' '); mon.print("]  r[ ");
    mon.print(txStatistics.memDumpRec,16,' '); mon.cprintln(']');
    sm.resetEnter();
  }

  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn >= '0' && mon.lastKeyIn <= '9')
  {
    int idx = mon.lastKeyIn & 0x0F;
    SlavePtr slPtr = blePoll.getSlavePtr(idx);
    PollStatePtr pPtr = blePoll.getPollPtr(slPtr->pIdx);

    mon.print((char *) "Slave[");
    mon.print(idx);
    mon.print((char *) "] ");
    mon.print(slPtr->cntTo); mon.cprint(' ');
    mon.print(slPtr->cntNakEP); mon.cprint(' ');
    mon.print(slPtr->cntAckDP); mon.cprint(' ');

    if(slPtr->cntAckDP == 0)
    {
      if(slPtr->cntTo == 0)
        tmpDw = slPtr->cntNakEP;
      else
        tmpDw = slPtr->cntNakEP / slPtr->cntTo;
    }
    else
    {
      if(slPtr->cntTo == 0)
        tmpDw = slPtr->cntAckDP;
      else
        tmpDw = slPtr->cntAckDP / slPtr->cntTo;
    }
    mon.print(tmpDw); mon.cprint(' ');

    mon.print(slPtr->cntLostPdu); mon.cprint('|');
    mon.print(slPtr->cntErrCrc); mon.cprint('|');
    mon.print(slPtr->result.measCnt); mon.cprint('|');
    mon.print(slPtr->cntLostMeas); mon.cprint(' ');

    for(i = 0; i < 6; i++)
    {
      tmpShort = (short) slPtr->result.meas[i];
      mon.prints(tmpShort); mon.cprint(' ');
    }
    mon.print((char *) "  Poll[");
    mon.print(slPtr->pIdx);
    mon.print((char *) "] ");
    mon.print(pPtr->status); mon.cprint(' ');
    mon.println(pPtr->slIdx);
    sm.resetEnter();
  }



  else
  {
    if(mon.lastKeyIn == ' ')
    {
      mon.cFlag[2] = false;
      mon.print((char *) "-- Schleifenabbruch - drücke Enter");
      sm.enter(smCheckJobs);
    }
  }
}

void smWaitPolling()
{
  if(!blePoll.stoppedEP()) return;

  mon.println((char *) "angehalten");
  sm.enter(smCtrlPolling);
}

// ----------------------------------------------------------------------------
// Debug-Informationen
// ----------------------------------------------------------------------------
//

void smReadPollValues()
{

}

#endif // DebugTerminal

