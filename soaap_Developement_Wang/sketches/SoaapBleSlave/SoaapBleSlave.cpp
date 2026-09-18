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

#include "SoaapBleSlave.h"

#ifdef SlaveADR1
#define SlaveADR 1
#endif

#ifdef SlaveADR2
#define SlaveADR 2
#endif

#ifdef SlaveADR3
#define SlaveADR 3
#endif

#ifdef SlaveADR4
#define SlaveADR 4
#endif

char *StartMsg =
{
    "%@TestBleSlave "
#ifdef SlaveADR1
    "(Adr=1), "
#endif
#ifdef SlaveADR2
    "(Adr=2), "
#endif
#ifdef SlaveADR3
    "(Adr=3), "
#endif
#ifdef SlaveADR4
    "(Adr=4), "
#endif
    "Version 20221024-1 "
};

LoopCheck     lc;
// Eine statische Instanz der Klasse LoopCheck
// Darüber wird das Zeitverhalten gesteuert (Software-Timer) und geprüft

#ifdef DebugTerminal
Monitor       mon(modeEcho | modeNl,0,&lc);
// Eine statische Instanz (mit Konstruktordaten) der Klasse Monitor
// Darüber wird mit (direkten) Terminals (z.B. VT100) kommuniziert
// Unter Linux werden hier GtkTerm (siehe Internet) und
// ArduinoMonTerm (eigene Entwicklung mit grafischen Wertanzeigen) eingesetzt.
// Das in den IDEs integrierte Terminal ist dafür meistens nicht geeignet,
// weil damit keine direkte Kommunikation (getipptes Zeichen sofort gesendet)
// möglich ist.
// ----- Parameter ------------------------------------------------------------
// <mode Echo>  Alle eintreffenden Zeichen werden sofort zurückgesendet
// <mode NL>    Vor der Ausgabe des Prompt (M>) erfolgt CR/LF
// <0>          Für Speicherzugriffe wird von 32 Bit ARM ausgegangen
// <&lc>        Für Zeitüberwachungen und entsprechende statisctische Daten
//              greift die Monitor-Klasse auf die LoopCheck-Klasse zu

char *infoThis =
{
"SOAAP BLE Slave Version 22.09.25-1\r\n"
"c0  Abschalten der periodischen Meldung\r\n"
"c1  Auslesen BlePoll-Debug-Register\r\n"
"c2  Steuern/Analysieren des Polling\r\n"
"c3  Steuern/Analysieren der Messwerterfassung\r\n"
"c4  Temporäre lokale Tests\r\n"
};

#endif

nRF52840Radio bleCom;
// Eine statische Instanz der Klasse nRF52840Radio
// Darüber wird direkt die CPU (nRF52840) auf dem Arduino-Board zum Senden und
// Empfangen von BLE-Beacons angesprochen.

#define bleCycle 250
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

#define ProcMeasCycle 250
ProcMeas proc((IntrfMeas *) &sens);
// Vorauswertung der Messwerte

// @yifan
// vorwaswertung der Sensdaten?
// 

nRF52840Gpio  gpio;
// Zugriff auf die Anschlüsse des Board

// @yifan, der Ziel ist die LEDs zu steuern als Anzeiger
// z.B.  ein Blick bedeutet das Programm gut laeuft, 
//       zwei Blicken bedeutet Stoerung
//       ......

bool getValues(PlpType plpType, byte *dest);
// Vorwärtsreferenz für Datenübergabe

bool xchgCtrl(PlpType plpType, byte *dest, byte *src, int sSize);
// Vorwärtsreferenz für Steuerungsaustausch

nRF52840Adc   adc;
// @yifan
// AD Wandler, der im Arduino nano integriert ist
// damit kann man die Spannung von der Batteriesversorgung messen
// der Batteriestand zu kriegen

void setup()
{
  TwiParams   twiPar;       // Parameter für den I2C-Bus

  gpio.configArd(ArdA0A3, IfDrvInput | IfDrvPullUp);

#ifdef DebugTerminal
  mon.config(6);    // 6 Anzeigekanäle, die von ArduinoMonTerm aufgebaut werden

  for(int i = 0; i < 6; i++)
  {
    if(i < 3)
      mon.config(i+1,'C',16000,-16000,NULL);    // Kanalnummer, Typ, Maxwert, Minwert
    else
      mon.config(i+1,'C',4000,-4000,NULL);
  }

  mon.setInfo(infoThis);

  // @yifan
  // einige Debug/config an Linux
#endif

  bleCom.begin();           // Initialisierung der Datenübertragung
  //bleCom.setPower(0x008);   // Maximale Sendeleistung bei nRF52840
  bleCom.setPower(0x0FC);   // Reduzierte Sendeleistung beim Schreibtisch-Test

  // @yifan
  // ble power, physicalische Ebene, der Kommunikationsabstand

  //blePoll.begin(BlePoll::ctSLAVE, SlaveADR, BlePoll::atDevSOAAP, 10000);
  blePoll.begin(BlePoll::ctSLAVE, SlaveADR, BlePoll::atSOAAP2, 50000);
  // Initialisierung des Polling mit folgenden Parametern:
  // <BlePoll::ctSLAVE>     Es wird ein Slave eingerichtet
  // <SlaveADR>             Adresse (Nummer) des Slave
  // <BlePoll::atSOAAP>     Spezielle Anwendung SOAAP
  // <10000>                INT-Watchdog-Timeout in Mikrosekunden

  // @yifan
  // wo der nano Board als Slave definiert, wenn der Master stoerungen hat, kann der slave 
  // als Master weiter laufen

  blePoll.setCbDataPtr(getValues);
  blePoll.setCbCtrlPtr(xchgCtrl);
  // Callbacks für Datenübergabe setzen

  //blePoll.stopSR();
  // Kein Empfangsbetrieb

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

  sens.begin(FreqAG119, 12, MaxAcc4g, MaxGyro2000dps, FreqM20, 10, MaxMag16G);
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
  // <FreqM40>    Der Magnetfeldsensor wird mit 20 Hz abgetastet
  //
  // <8>          Mittelwertbildung über 10 Messwerte, also 2 Hz
  //
  // <MaxMag16G>  Der Maximalwert entspricht 16 Gauss

  sens.syncValuesAG();
  sens.syncValuesM();
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

  // Alle 250 Mikrosekunden erfolgt der Aufruf des Ble-Polling
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

  // Alle 250 Mikrosekunden erfolgt der Aufruf der Messwertvorverarbeitung
  if(lc.timerMicro(lcTimer2, ProcMeasCycle, 0))
    proc.run();

#ifdef DebugTerminal
  // Alle 5 Millisekunden wird die Zustandsmaschine für
  // betriebliche Abläufe aufgerufen
  //
  if(lc.timerMilli(lcTimer3, smCycleTime, 0))
  {
    sm.run();
  }

  // Jede halbe Sekunde erfolgt die Ausgabe der Version
  //
  if(lc.timerMilli(lcTimer4, 500, 0))
  {
    if(!mon.cFlag[0])
    {
      mon.print(StartMsg);
      mon.cprintcr(runView[runViewIdx]);
      runViewIdx++;
      if(runViewIdx > 3) runViewIdx = 0;
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
RawDataAG   rawDataAG;
RawDataM    rawDataM;
CalValueAG  calDataAG;
byte        deviceState[4];

bool getValues(PlpType plpType, byte *dest)
{
  bool  retv;
  int   si,di;
  union {float tmpFloat; byte tmpByteArr[4]; } transData;


  si = di = 0;

  switch(plpType)
  {
    case plptMeas6:
      if(!sens.availValuesAG())
      {
        retv = false;
        break;
      }
      memset(rawDataAG.byteArray,0,12);
      sens.getValuesAG(&rawDataAG);
      for(si = 0; si < 12; si++)
        dest[di++] = rawDataAG.byteArray[si];
      retv = true;
      break;

    case plptMeas9Ctrl4:
      if(!sens.availValuesAG())
      {
        retv = false;
        break;
      }
      memset(rawDataAG.byteArray,0,12);
      sens.getValuesAG(&rawDataAG);
      sens.getValuesM(&rawDataM);

      for(si = 0; si < 12; si++)
        dest[di++] = rawDataAG.byteArray[si];
      for(si = 0; si < 6; si++)
        dest[di++] = rawDataM.byteArray[si];
      break;

    // ----------------------------------------------------------------------------------
    case plptIMU3F4Ctrl4:     // Roll-, Nick- und Gierwinkel Float + 4 Byte Status
    // ----------------------------------------------------------------------------------
      if(!proc.availAngles(true))
      {
        retv = false;
        break;
      }

      transData.tmpFloat = proc.getRollValue();
      for(si = 0; si < 4; si++)
        dest[di++] = transData.tmpByteArr[si];

      transData.tmpFloat = proc.getPitchValue();
      for(si = 0; si < 4; si++)
        dest[di++] = transData.tmpByteArr[si];

      transData.tmpFloat = proc.getYawValue();
      for(si = 0; si < 4; si++)
        dest[di++] = transData.tmpByteArr[si];

      deviceState[0] = proc.getGravSigns();

      for(si = 0; si < 4; si++)
        dest[di++] = deviceState[si];

      retv = true;
      break;
  }

  return(retv);
}

union
{
byte procData[32];
PlpCtrl25 pdu;
} inCtrl;

int  procSize     = 0;
byte procPath     = 0;
byte procCnt      = 0;
byte ctrlCnt      = 0;


bool xchgCtrl(PlpType plpType, byte *dest, byte *src, int sSize)
{
  int   si,di;
  bool  retv;

  PlpCtrl2Ptr   recPtr  = (PlpCtrl2Ptr) src;

  if(recPtr->ctrlCnt == ctrlCnt) return(false); // Nichts Neues
  ctrlCnt = recPtr->ctrlCnt;

  si = di = 0;
  retv = false;

  switch(plpType)
  {
    case plptMeas9Ctrl4:
    case plptIMU3F4Ctrl4:
      // Lokale Kopie der Steuerdaten vom Master anlegen
      //
      for(si = 0; si < sSize; si++)
        inCtrl.procData[si] = src[si];
      procSize = sSize;

      dest[di++] = procPath;
      dest[di++] = ++procCnt;
      // Test
      dest[di++] = (inCtrl.pdu.counter & 0x3F) | 0x40;
      dest[di++] = 'O';
      retv = true;
      break;
  }

  return(retv);
}

#ifdef DebugTerminal
// ****************************************************************************
// Z u s t a n d s m a s c h i n e
// ****************************************************************************
//

dword       debDword;
byte        tmpByteArray[256];
int         tmpInt;
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
  else if(mon.cFlag[4] && !mon.busy)
    sm.enter(smTempTesting);
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
char *smPollHelp =
{
    "B   Prozessdaten anzeigen\r\n"
    "C   Steuerempfang auslesen\r\n"
    "E   Empfangspuffer auslesen (16 Zeichen)\r\n"
    "P   Polling starten/stoppen/fortsetzen\r\n"
    "R   Radiopuffer auslesen (16 Zeichen)\r\n"
    "S   Sendepuffer auslesen (30 Zeichen)\r\n"
    "T   Übertragungsstatistik und Daten anzeigen\r\n"
    "0...9  Slave-Umgebung\r\n"
};

TxStatistics txStatistics;

void smCtrlPolling()
{
  if(sm.firstEnter())
  {
    mon.print((char *) "polling ");
    mon.lastKeyIn = ':';
  }

  if(mon.lastKeyIn == ':') return;

  if(mon.lastKeyIn == 'H' || mon.lastKeyIn == 'h')
  {
    mon.println();
    mon.print(smPollHelp);
    sm.resetEnter();
  }


  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'P' || mon.lastKeyIn == 'p')
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
  else if(mon.lastKeyIn == 'B' || mon.lastKeyIn == 'b')
  {
    mon.print((char *) "Prozessdaten = ");
    mon.println(inCtrl.procData, 16, ' ');
    sm.resetEnter();
  }

  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'C' || mon.lastKeyIn == 'c')
  {
    mon.print((char *) "Steuerempfang[");
    tmpInt = blePoll.getCtrlData(tmpByteArray);
    mon.print(tmpInt);
    mon.print("] = ");
    mon.println(tmpByteArray, tmpInt, ' ');
    sm.resetEnter();
  }

  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'E' || mon.lastKeyIn == 'e')
  {
    mon.print((char *) "Empfangspuffer = ");
    bleCom.getPduRec(tmpByteArray, 0, 16);
    mon.println(tmpByteArray, 16, ' ');
    sm.resetEnter();
  }

  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'S' || mon.lastKeyIn == 's')
  {
    mon.print((char *) "Sendepuffer = ");
    bleCom.getPduSentS(tmpByteArray, 0, 32);
    mon.println(tmpByteArray, 32, ' ');
    sm.resetEnter();
  }

  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'R' || mon.lastKeyIn == 'r')
  {
    mon.print((char *) "Radiopuffer = ");
    bleCom.getPduMem(tmpByteArray, 0, 16);
    mon.println(tmpByteArray, 16, ' ');
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
    mon.print(txStatistics.memDumpRec,14,' '); mon.print("]  s[ ");
    mon.print(txStatistics.memDumpSnd,24,' '); mon.cprintln(']');
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
// (3) Testen der Sensorzugriffe
// ----------------------------------------------------------------------------
//

char *chkSensHelp =
{
"3 Beschleunigung Rohwerte (short)\r\n"
"4 Beschleunigung kalibrierte Werte (float)\r\n"
"5 Beschleunigung kalibrierte Werte zyklisch (float)\r\n"
"6 Beschleunigung & Gyroskop kalibrierte Werte zyklisch (float)\r\n"
#ifdef ProcMeasDebug
"p Debug <ProcMeas>\r\n"
#endif
};

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
  else if(mon.lastKeyIn == 'H' || mon.lastKeyIn == 'h')
  {
    mon.println();
    sm.enter(smSensHelp);
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
  else if(mon.lastKeyIn == 'a')
    sm.enter(smSensGetAngleValues);
  else if(mon.lastKeyIn == 'b')
    sm.enter(smSensCheckAngleValues);
  else if(mon.lastKeyIn == 'c')
    sm.enter(smSensDebugValues);
  else if(mon.lastKeyIn == 'e')
    sm.enter(smSensGetErrors);
#ifdef ProcMeasDebug
  else if(mon.lastKeyIn == 'p')
    sm.enter(smDebugProcMeas);
#endif
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

void smSensHelp()
{
  mon.println();
  mon.println(chkSensHelp);
  sm.enter(smCheckSens);
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

  if(!sens.getValuesAG(&rawDataAG)) return;

  mon.print((char *) "ValueA = ");
  mon.print(rawDataAG.valueAG.A.x);
  mon.print((char *) " ");
  mon.print(rawDataAG.valueAG.A.y);
  mon.print((char *) " ");
  mon.println(rawDataAG.valueAG.A.z);
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
  sm.setDelay(500);

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
  sm.setDelay(500);

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

  if(!sens.getValuesAG(&rawDataAG)) return;
  sm.setTimeOut(1000);

  mon.print((char *) "Values AGM = ");

  sprintf(outValue,"%4X ",(unsigned short) rawDataAG.valueAG.A.x);
  mon.print(outValue);
  sprintf(outValue,"%4X ",(unsigned short) rawDataAG.valueAG.A.y);
  mon.print(outValue);
  sprintf(outValue,"%4X   ",(unsigned short) rawDataAG.valueAG.A.z);
  mon.print(outValue);

  sprintf(outValue,"%4X ",(unsigned short) rawDataAG.valueAG.G.x);
  mon.print(outValue);
  sprintf(outValue,"%4X ",(unsigned short) rawDataAG.valueAG.G.y);
  mon.print(outValue);
  sprintf(outValue,"%4X   ",(unsigned short) rawDataAG.valueAG.G.z);
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

  if(!sens.getValuesAG(&rawDataAG)) return;
  sm.setTimeOut(1000);

  mon.print((char *) "%~Values AGM = $");

  sprintf(outValue,"#@%04X$",(unsigned short) rawDataAG.valueAG.A.x);
  mon.print(outValue);
  sprintf(outValue,"#A%04X$",(unsigned short) rawDataAG.valueAG.A.y);
  mon.print(outValue);
  sprintf(outValue,"#B%04X$",(unsigned short) rawDataAG.valueAG.A.z);
  mon.print(outValue);

  sprintf(outValue,"#C%04X$",(unsigned short) rawDataAG.valueAG.G.x);
  mon.print(outValue);
  sprintf(outValue,"#D%04X$",(unsigned short) rawDataAG.valueAG.G.y);
  mon.print(outValue);
  sprintf(outValue,"#E%04X$",(unsigned short) rawDataAG.valueAG.G.z);
  mon.print(outValue);

  if(mon.lastKeyIn == ' ')
    sm.enter(smCheckSens);
}


void smSensGetAngleValues()
{
  float roll, pitch;

  if(sm.firstEnter())
  {
    sens.syncValuesAG();
    sm.setTimeOut(1000);
  }

  if(!proc.availAngles(true))
  {
    if(sm.timeOut())
    {
      mon.println("Time-Out Warten auf Messwerte");
      sm.enter(smCheckSens);
    }
    return;
  }

  roll = proc.getRollValue();
  pitch = proc.getPitchValue();

  mon.print((char *) "Angles Roll = ");
  sprintf(outValue,"%f  Pitch = ",roll);
  mon.print(outValue);
  sprintf(outValue,"%f ",pitch);
  mon.println(outValue);
  sm.enter(smCheckSens);
}


void smSensCheckAngleValues()
{
  float roll, pitch;
  dword cntMics;

  if(sm.firstEnter())
  {
    sens.syncValuesAG();
    sm.setTimeOut(1000);
  }

  if(!proc.availAngles(true))
  {
    if(sm.timeOut())
    {
      mon.println("Time-Out Warten auf Messwerte");
      sm.enter(smCheckSens);
    }
    return;
  }

  cntMics = micros();
  roll = proc.getRollValue();
  pitch = proc.getPitchValue();
  cntMics = micros() - cntMics;

  mon.print(cntMics);
  mon.print((char *) " ValueA = ");
  sprintf(outValue,"%f ",calData.A.x);
  mon.print(outValue);
  sprintf(outValue,"%f ",calData.A.y);
  mon.print(outValue);
  sprintf(outValue,"%f ",calData.A.z);
  mon.println(outValue);

  mon.print((char *) "Angles Roll = ");
  sprintf(outValue,"%f  Pitch = ",roll);
  mon.print(outValue);
  sprintf(outValue,"%f ",pitch);
  mon.println(outValue);
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

#ifdef ProcMeasDebug
void smDebugProcMeas()
{
  mon.print((char *) "ProcMeas: Init = ");
  mon.print(proc.statistics.pmInitCnt);
  mon.print((char *) " Wait = ");
  mon.print(proc.statistics.pmWaitCnt);
  mon.print((char *) " Calc = ");
  mon.println(proc.statistics.pmCalcCnt);

  sm.enter(smCheckSens);
}
#endif

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

// ----------------------------------------------------------------------------
// (4) Temporäres Testen lokaler Speicher und Funktionen
// ----------------------------------------------------------------------------
//

void smTempTesting()
{
  if(sm.firstEnter())
  {
    mon.print((char *) "Temp. Testen ");
    mon.lastKeyIn = ':';
  }

  if(mon.lastKeyIn == ':') return;

  charOut[0] = mon.lastKeyIn;
  charOut[1] = '\0';
  mon.println(charOut);

  if(mon.lastKeyIn == '0' || mon.lastKeyIn == ' ')
  {
    mon.cFlag[4] = false;
    mon.print((char *) "-- Schleifenabbruch - drücke Enter");
    sm.enter(smCheckJobs);
  }
  else if(mon.lastKeyIn == 'M' || mon.lastKeyIn == 'm')
  {
    mon.println();
    sm.enter(smTestAdcMem);
  }
  mon.lastKeyIn = ':';
  sm.resetEnter();
}

char  tmpOut[256];

void smTestAdcMem()
{
  nrfAdcPtr perMem = NULL;
  volatile ChnConfigs  *chnCfgPtr;

  dword res1 = (dword) &perMem->EVENTS_STARTED;
  dword res2 = (dword) &perMem->INTEN;
  dword res3 = (dword) &perMem->STATUS;
  dword res4 = (dword) &perMem->ENABLE;
  //dword res5 = (dword) &perMem->CH[0];
  chnCfgPtr = &(NrfAdcPtr->CH[0]);
  volatile dword *res5Ptr = &(chnCfgPtr->CONFIG);
  dword res5 = (dword) res5Ptr;
  dword res6 = (dword) &perMem->RESOLUTION;
  dword res7 = (dword) &perMem->RESULT_PTR;

  sprintf(tmpOut,"%X %X %X %X %X %X %X",res1,res2,res3,res4,res5,res6,res7);
  mon.println(tmpOut);
  sm.enter(smTempTesting);
}


#endif // DebugTerminal

