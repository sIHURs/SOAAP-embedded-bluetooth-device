// ----------------------------------------------------------------------------
//                              SoaapBleSlave.ino
// Beispielhafte Anwendung SOAAP / Steuerung optischer und akustischer Ausgaben
//      Kommunikation über BLE-Funkanäle mit Bewerbungstelegrammen
//                           P o l l i n g - S l a v e
// ----------------------------------------------------------------------------
// Editor:  Robert Patzke
// URI/URL: www.mfp-portal.de
//-----------------------------------------------------------------------------
// Lizenz:  CC-BY-SA  (wikipedia: Creative Commons)
// Datum:   1. November 2021
// Letzte Bearbeitung: 1. November 2021
//
#include "Arduino.h"

#include <LoopCheck.h>
#include "StateMachine.h"

#include  "nRF52840Twi.h"
#include  "SensorLSM9DS1.h"

#include "nRF52840Radio.h"
#include <BlePoll.h>
#include "Monitor.h"

#include "SoaapBleSlave.h" // Eingefügt


#define DebugTerminal
// Mit dieser Definition werden die Klasse Monitor und weitere Testmethoden
// eingebunden, womit ein anwendungsorientiertes Debugging möglich ist

#define SlaveACM2
/*
#ifdef SlaveACM1
#define SlaveADR 1
#define StartMsg "%@TestBleSlave (Adr=1, ttyACM1), Version 20211108 "
#endif
*/
#ifdef SlaveACM2
#define SlaveADR 2
#define StartMsg "%@TestBleSlave (Adr=4, ttyACM2), Version 20211108 "
#endif

#ifdef SlaveACM3
#define SlaveADR 5
#define StartMsg "%@TestBleSlave (Adr=5, ttyACM3), Version 20211108"
#endif

LoopCheck     lc;
// Eine statische Instanz der Klasse LoopCheck
// Darüber wird das Zeitverhalten gesteuert (Software-Timer) und geprüft

#ifdef DebugTerminal
Monitor       mon(modeEcho | modeNl,0,&lc);
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

#define bleCycle 150
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

nRF52840Twi twi;
// Eine statische Instanz der Klasse nRF52840Twi
// Darüber wird direkt die CPU (nRF52840) auf dem Arduino-Board zum Senden und
// Empfangen von I2C (TWI) Botschaften aufgerufen.

int durchlaufe=0; //hinzugefügt
#define SensorCycle   500
SensorLSM9DS1 sens((IntrfTw *) &twi, SensorCycle);
// Eine statische Instanz der Klasse SensorLSM9DS1 zum Zugriff
// auf den Sensor LSM9DS1 über den I2C-Bus (TWI)
// ----- Parameter ------------------------------------------------------------
// <&twi>   Die Klasse (vom angenommenen Typ IntrfTw), die die Datenüber-
//          tragung auf I2C abwickelt. Hier wird eine Instanz von nRF52840Twi
//          angebunden. Für andere Hardware kann eine entsprechende Klasse
//          verwendet werden, die von IntrfTw abgeleitet wurde.
//
// <SensorCycle>  Der Zugriff auf den Sensor erfolgt mit einer Zustands-
//                maschine, deren Takt über die Periodenzeit SensorCycle
//                in Mikrosekunden definiert ist. Siehe dazu entsprechenden
//                Timeraufruf in LOOP.

bool getValues(PlpType plpType, byte *dest);  // Vorwärtsreferenz für Datenübergabe

void setup()
{
  TwiParams   twiPar;       // Parameter für den I2C-Bus

#ifdef DebugTerminal
  mon.config(6);    // 6 Anzeigekanäle, die von ArduinoMonTerm aufgebaut werden

  for(int i = 0; i < 6; i++)
  {
    if(i < 3)
      mon.config(i+1,'C',16000,-16000,NULL);    // Kanalnummer, Typ, Maxwert, Minwert
    else
      mon.config(i+1,'C',4000,-4000,NULL);
  }
#endif

  bleCom.begin();           // Initialisierung der Datenübertragung
  //bleCom.setPower(0x008);   // Maximale Sendeleistung bei nRF52840
  bleCom.setPower(0x0FC);   // Reduzierte Sendeleistung beim Schreibtisch-Test

  blePoll.begin(BlePoll::ctSLAVE, SlaveADR, BlePoll::atSOAAP, 10000);
  // Initialisierung des Polling mit folgenden Parametern:
  // <BlePoll::ctSLAVE>     Es wird ein Slave eingerichtet
  // <SlaveADR>             Adresse (Nummer) des Slave
  // <BlePoll::atSOAAP>     Spezielle Anwendung SOAAP
  // <10000>                INT-Watchdog-Timeout in Mikrosekunden
  blePoll.setCbDataPtr(sendData);
  //blePoll.setCbDataPtr(getValues);
  // Callback für Datenübergabe setzen

  // Spezifische Parameter für den I2C-Bus am Arduino Nano 33 BLE
  //
  twiPar.inst     = 0;          // 1. Instanz der I2C
  twiPar.type     = TwiMaster;  // Auswahl Master/slave
  twiPar.clkPort  = 0;          // Takt über Port 0 (P0)
  twiPar.clkPin   = 15;         // Takt an Pin 15 (P0.15)
  twiPar.dataPort = 0;          // Daten über Port 0 (P0)
  twiPar.dataPin  = 14;         // Daten an Pin 14 (P0.14)
  twiPar.speed    = Twi100k;    // Taktfrequenz

  twi.begin(&twiPar); // Initialisierung des I2C-Bus

  sens.begin(FreqAG119, 12, MaxAcc4g, MaxGyro2000dps, FreqM_OFF, 0, MaxMag4G);
  // Initialisierung der Sensorabfrage mit folgenden Parametern
  //
  // <FreqAG119>  Beschleunigungssensoren und Gyroskop mit 119 Hz abgefragt
  //
  // <12>         Vor der Wertübergabe wird der Mittelwert über 12 Messungen
  //              gebildet. Die Messfrequenz beträgt daher ca. 10 Hz.
  //
  // <MaxAcc4g>   Der Maximalausschlag der Beschleunigung ist 4g
  //
  // <MaxGyro2000dps> Maximalausschlag des Gyro ist 2000 Grad/s
  //
  // <FreqM_OFF>  Der Magnetfeldsensor ist AUS (funktioniert noch nicht)
  //
  // <0>          Keine Mittelwertbildung der Magnetfeldmessung
  //
  // <MaxMag4G>   Der Maximalwert entspricht 4 Gauss

  sens.syncValuesAG();
  // Rücksetzen des Ready-Bit für Messwertübergabe
}

#ifdef DebugTerminal
char  runView[4] = {'|','/','-','\\'};
int   runViewIdx = 0;
#endif

void loop()
{
  lc.begin();
  // --------------------------------------------------------------------------

#ifdef DebugTerminal
  // Der Monitor wird ständig aufgerufen und stellt damit eine grundsätzliche
  // Belastung dar. Das ist für die Entwicklung auch vorgesehen.
  // Es entsteht dadurch eine Reserve für den produktiven Einsatz.
  //
  mon.run();
#endif

  // Alle 500 Mikrosekunden erfolgt der Aufruf des Ble-Polling
  //
  if(lc.timerMicro(lcTimer0, bleCycle, 0))
    blePoll.run();
  // ----- Parameter der Software-Timer ---------------------------------------
  // <lcTimer0>   Id/Nummer des Timer, zur Zeit werden bis 10 Timer unterstützt
  //              lcTimer0 bis lcTimer9 (einstellbar in LoopCheck.h)
  // <bleCycleTime>   Ablaufzeit in Einheit des Timer-Typ (Micro/Milli)
  //                  hier in Mikrosekunden (timerMicro)
  // <0>          Anzahl der Wiederholungen, 0 = unbegrenzt

  // Alle 500 Mikrosekunden erfolgt der Aufruf der Sensorzustandsmaschine
  //
  if(lc.timerMicro(lcTimer1, SensorCycle, 0))
    sens.run();

#ifdef DebugTerminal
  // Alle 5 Millisekunden wird die Zustandsmaschine für
  // betriebliche Abläufe aufgerufen
  //
  if(lc.timerMilli(lcTimer2, smCycleTime, 0))
  {
    sm.run();
  }

  // Jede halbe Sekunde erfolgt die Ausgabe der Version
  //
  if(lc.timerMilli(lcTimer3, 500, 0))
  {
    if(!mon.cFlag[0])
    {
      /*
      mon.print((char *) StartMsg);
      mon.cprintcr(runView[runViewIdx]);
      runViewIdx++;
      if(runViewIdx > 3) runViewIdx = 0;
      */
      smCtrlPolling();
    }
  }
#endif
  // --------------------------------------------------------------------------
  lc.end();
}

// ----------------------------------------------------------------------------
// Übergabe der Daten an die BLE-Kommunikation
// ----------------------------------------------------------------------------
//
RawDataAG   rawData;
bool sendData(PlpType plpType, byte *dest){
  short i;
    for (i=0;i<12;i++){
      //i%2!=0?rawData.byteArray[i]=0:rawData.byteArray[i]=i;
      if(i==0 || i==2 || i == 4 || i == 6 || i ==8 || i== 10 || i== 12){
        dest[i]=i;
      }else{
        dest[i]=i;
      }
    }
  return true;
}

bool getValues(PlpType plpType, byte *dest)
{
  bool  newData;
  short   i;

  memset(rawData.byteArray,0,12);

  newData = sens.getValuesAG(&rawData);
  //Debug: Messdaten überschreiben
  /*
  for (i=0;i<12;i++){
    i%2!=0?rawData.byteArray[i]=1:rawData.byteArray[i]=i;
  }
  */
  //Ende Lennard
  if(newData)
  {
    switch(plpType)
    {
      case plptMeas6:
        for(i = 0; i < 12; i++)
          dest[i] = rawData.byteArray[i];
        break;
    }
  }
  return(newData);

}


#ifdef DebugTerminal
// ****************************************************************************
// Z u s t a n d s m a s c h i n e
// ****************************************************************************
//

dword       debDword;
byte        tmpByteArray[256];
CalValueAG  calData;
CalValue    calValue;


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
    sm.enter(smCheckSens);
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
// Es ist sowohl die Master- als auch die Slave-Funktion vorgesehen
//

TxStatistics txStatistics;

void smCtrlPolling()
{
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
  else if(mon.lastKeyIn == 'S' || mon.lastKeyIn == 's')
  {
    mon.print((char *) "Sendepuffer = ");
    bleCom.getPduSent(tmpByteArray, 0, 10);
    mon.println(tmpByteArray, 10, ' ');
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
    mon.print(txStatistics.crcErrors); mon.print("  r[ ");
    mon.print(txStatistics.memDumpRec,8,' '); mon.print("]  s[ ");
    mon.print(txStatistics.memDumpSnd,16,' '); mon.cprintln(']');
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
// (4) Testen der Sensorzugriffe
// ----------------------------------------------------------------------------
//

char charOut[2];

void smCheckSens()
{
  if(sm.firstEnter())
  {
    mon.print((char *) "Sensorzugriff ");
    mon.lastKeyIn = ':';
  }

  if(mon.lastKeyIn == ':') return;

  charOut[0] = mon.lastKeyIn;
  charOut[1] = '\0';
  mon.println(charOut);

  if(mon.lastKeyIn == '0' || mon.lastKeyIn == ' ')
  {
    mon.cFlag[3] = false;
    mon.print((char *) "-- Schleifenabbruch - drücke Enter");
    sm.enter(smCheckJobs);
  }
  else if(mon.lastKeyIn == '1')
    sm.enter(smSensReset1);
  else if(mon.lastKeyIn == '2')
    sm.enter(smSensReset2);
  else if(mon.lastKeyIn == '3')
    sm.enter(smSensGetValues1);
  else if(mon.lastKeyIn == '4')
    sm.enter(smSensGetValues2);
  else if(mon.lastKeyIn == '5')
    sm.enter(smSensGetValues3);
  else if(mon.lastKeyIn == '6')
    sm.enter(smSensGetValues4);
  else if(mon.lastKeyIn == '7')
    sm.enter(smSensGetValues5);
  else if(mon.lastKeyIn == '8')
    sm.enter(smSensGetValues6);
  else if(mon.lastKeyIn == 'c')
    sm.enter(smSensDebugValues);
  else if(mon.lastKeyIn == 'e')
    sm.enter(smSensGetErrors);
  else if(mon.lastKeyIn == 'r')
    sm.enter(smSensGetRunCounts);
  else if(mon.lastKeyIn == 's')
    sm.enter(smSensGetSoaapValues);
  else if(mon.lastKeyIn == 't')
    sm.enter(smSensGetTimeOuts);
  else
    sm.resetEnter();
  mon.lastKeyIn = ':';
}

void smSensReset1()
{
  int retv = sens.resetAG();
  //int retv = twi.writeByteReg(0x6B, 0x22, 0x05);
  mon.print((char *) "resetAG = ");
  mon.println(retv);
  sm.enter(smCheckSens);
}

void smSensReset2()
{
  int retv = sens.resetM();
  //int retv = twi.writeByteReg(0x1E, 0x21, 0x0C);
  mon.print((char *) "resetM = ");
  mon.println(retv);
  sm.enter(smCheckSens);
}

void smSensGetValues1()
{
  if(sm.firstEnter())
    sens.syncValuesAG();

  if(!sens.getValuesAG(&rawData)) return;

  mon.print((char *) "ValueA = ");
  mon.print(rawData.valueAG.A.x);
  mon.print((char *) " ");
  mon.print(rawData.valueAG.A.y);
  mon.print((char *) " ");
  mon.println(rawData.valueAG.A.z);
  sm.enter(smCheckSens);
}

char  outValue[64];

void smSensGetValues2()
{
  if(sm.firstEnter())
    sens.syncValuesAG();

  if(!sens.getValuesAG(&calData)) return;

  mon.print((char *) "ValueA = ");
  sprintf(outValue,"%f ",calData.A.x);
  mon.print(outValue);
  sprintf(outValue,"%f ",calData.A.y);
  mon.print(outValue);
  sprintf(outValue,"%f ",calData.A.z);
  mon.println(outValue);
  sm.enter(smCheckSens);
}

void smSensGetValues3()
{
  if(sm.firstEnter())
  {
    sens.syncValuesAG();
    mon.lastKeyIn = ':';
  }

  if(!sens.getValuesAG(&calData)) return;

  mon.print((char *) "Values A = ");
  sprintf(outValue,"%+5.3f ",calData.A.x);
  mon.print(outValue);
  sprintf(outValue,"%+5.3f ",calData.A.y);
  mon.print(outValue);
  sprintf(outValue,"%+5.3f ",calData.A.z);
  mon.println(outValue);

  if(mon.lastKeyIn == ' ')
    sm.enter(smCheckSens);
}

void smSensGetValues4()
{
  if(sm.firstEnter())
  {
    sens.syncValuesAG();
    mon.lastKeyIn = ':';
  }

  if(!sens.getValuesAG(&calData)) return;

  mon.print((char *) "Values AG = ");
  sprintf(outValue,"%+5.3f ",calData.A.x);
  mon.print(outValue);
  sprintf(outValue,"%+5.3f ",calData.A.y);
  mon.print(outValue);
  sprintf(outValue,"%+5.3f   ",calData.A.z);
  mon.print(outValue);

  sprintf(outValue,"%+5.1f ",calData.G.x);
  mon.print(outValue);
  sprintf(outValue,"%+5.1f ",calData.G.y);
  mon.print(outValue);
  sprintf(outValue,"%+5.1f ",calData.G.z);
  mon.println(outValue);


  if(mon.lastKeyIn == ' ')
    sm.enter(smCheckSens);
}

void smSensGetValues5()
{
  if(sm.firstEnter())
  {
    sens.syncValuesAG();
    mon.lastKeyIn = ':';
    sm.setTimeOut(1000);
  }

  if(sm.timeOut())
  {
    mon.println((char *) " Time Out");
    sm.enter(smCheckSens);
    return;
  }

  if(!sens.getValuesAG(&calData)) return;
  sm.setTimeOut(1000);

  mon.print((char *) "Values AGM = ");

  sprintf(outValue,"%+5.3f ",calData.A.x);
  mon.print(outValue);
  sprintf(outValue,"%+5.3f ",calData.A.y);
  mon.print(outValue);
  sprintf(outValue,"%+5.3f   ",calData.A.z);
  mon.print(outValue);

  sprintf(outValue,"%+5.1f ",calData.G.x);
  mon.print(outValue);
  sprintf(outValue,"%+5.1f ",calData.G.y);
  mon.print(outValue);
  sprintf(outValue,"%+5.1f   ",calData.G.z);
  mon.print(outValue);

  sens.getValuesM(&calValue);

  sprintf(outValue,"%+5.3f ",calValue.x);
  mon.print(outValue);
  sprintf(outValue,"%+5.3f ",calValue.y);
  mon.print(outValue);
  sprintf(outValue,"%+5.3f ",calValue.z);
  mon.println(outValue);

  if(mon.lastKeyIn == ' ')
    sm.enter(smCheckSens);
}

void smSensGetValues6()
{
  if(sm.firstEnter())
  {
    sens.syncValuesAG();
    mon.lastKeyIn = ':';
    sm.setTimeOut(1000);
  }

  if(sm.timeOut())
  {
    mon.println((char *) " Time Out");
    sm.enter(smCheckSens);
    return;
  }

  if(!sens.getValuesAG(&rawData)) return;
  sm.setTimeOut(1000);

  mon.print((char *) "Values AGM = ");

  sprintf(outValue,"%4X ",(unsigned short) rawData.valueAG.A.x);
  mon.print(outValue);
  sprintf(outValue,"%4X ",(unsigned short) rawData.valueAG.A.y);
  mon.print(outValue);
  sprintf(outValue,"%4X   ",(unsigned short) rawData.valueAG.A.z);
  mon.print(outValue);

  sprintf(outValue,"%4X ",(unsigned short) rawData.valueAG.G.x);
  mon.print(outValue);
  sprintf(outValue,"%4X ",(unsigned short) rawData.valueAG.G.y);
  mon.print(outValue);
  sprintf(outValue,"%4X   ",(unsigned short) rawData.valueAG.G.z);
  mon.println(outValue);

  /*
  sens.getValuesM(&calValue);

  sprintf(outValue,"%+5.3f ",calValue.x);
  mon.print(outValue);
  sprintf(outValue,"%+5.3f ",calValue.y);
  mon.print(outValue);
  sprintf(outValue,"%+5.3f ",calValue.z);
  mon.println(outValue);
  */
  if(mon.lastKeyIn == ' ')
    sm.enter(smCheckSens);
}


void smSensGetValues7()
{
  if(sm.firstEnter())
  {
    sens.syncValuesAG();
    mon.lastKeyIn = ':';
    sm.setTimeOut(1000);
  }

  if(sm.timeOut())
  {
    mon.println((char *) " Time Out");
    sm.enter(smCheckSens);
    return;
  }

  if(!sens.getValuesAG(&rawData)) return;
  sm.setTimeOut(1000);

  mon.print((char *) "%~Values AGM = $");

  sprintf(outValue,"#@%04X$",(unsigned short) rawData.valueAG.A.x);
  mon.print(outValue);
  sprintf(outValue,"#A%04X$",(unsigned short) rawData.valueAG.A.y);
  mon.print(outValue);
  sprintf(outValue,"#B%04X$",(unsigned short) rawData.valueAG.A.z);
  mon.print(outValue);

  sprintf(outValue,"#C%04X$",(unsigned short) rawData.valueAG.G.x);
  mon.print(outValue);
  sprintf(outValue,"#D%04X$",(unsigned short) rawData.valueAG.G.y);
  mon.print(outValue);
  sprintf(outValue,"#E%04X$",(unsigned short) rawData.valueAG.G.z);
  mon.print(outValue);

  if(mon.lastKeyIn == ' ')
    sm.enter(smCheckSens);
}


void smSensGetSoaapValues()
{
  sm.enter(smSensGetValues7);
}

void smSensDebugValues()
{
  mon.print((char *) "Werte: toValueStatusAG=");
  mon.print(sens.debGetDword(1));
  mon.print((char *) "  toValueStatusM=");
  mon.println(sens.debGetDword(2));
  sm.enter(smCheckSens);
}


void smSensGetErrors()
{
  mon.print((char *) "Errors AG: AdrNak=");
  mon.print(sens.errorCntAdrNakAG);
  mon.print((char *) "  DataNak=");
  mon.print(sens.errorCntDataNakAG);
  mon.print((char *) "  Overrun=");
  mon.print(sens.errorCntOverAG);

  mon.print((char *) "  M: AdrNak=");
  mon.print(sens.errorCntAdrNakM);
  mon.print((char *) "  DataNak=");
  mon.print(sens.errorCntDataNakM);
  mon.print((char *) "  Overrun=");
  mon.println(sens.errorCntOverM);
  sm.enter(smCheckSens);
}

void smSensGetTimeOuts()
{
  mon.print((char *) "TimeOuts AG: TwiStat=");
  mon.print(sens.toCntTwiStatusAG);
  mon.print((char *) "  TwiData=");
  mon.print(sens.toCntTwiDataAG);
  mon.print((char *) "  Status=");
  mon.print(sens.toCntStatusAG);

  mon.print((char *) "  M: TwiStat=");
  mon.print(sens.toCntTwiStatusM);
  mon.print((char *) "  TwiData=");
  mon.print(sens.toCntTwiDataM);
  mon.print((char *) "  Status=");
  mon.println(sens.toCntStatusM);
  sm.enter(smCheckSens);
}

void smSensGetRunCounts()
{
  mon.print((char *) "RunCounts: ");
  for(int i = 0; i < NrOfRunStates; i++)
  {
    mon.print((char *) " ");
    mon.print(sens.runStateCntArray[i]);
  }
  mon.print((char *) "  Total=");
  mon.println(sens.runStateCntTotal);
  sm.enter(smCheckSens);
}
#endif // DebugTerminal

