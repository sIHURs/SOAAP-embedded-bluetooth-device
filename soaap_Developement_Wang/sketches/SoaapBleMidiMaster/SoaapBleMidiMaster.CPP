// ----------------------------------------------------------------------------
//                              SoaapBleMidiMaster.ino
// Beispielhafte Anwendung SOAAP / Steuerung optischer und akustischer Ausgaben
//      Kommunikation über BLE-Funkanäle mit Bewerbungstelegrammen
//                        P o l l i n g - M a s t e r
// ----------------------------------------------------------------------------
// Editor:  Robert Patzke
// URI/URL: www.hs-hannover.de
//-----------------------------------------------------------------------------
// Lizenz:  CC-BY-SA  (wikipedia: Creative Commons)
// Datum:   26. April 2022
// Letzte Bearbeitung:
//

#include  "Arduino.h"
#include  "SoaapBleMidiMaster.h"

// ----------------------------------------------------------------------------
LoopCheck     lc;
// ----------------------------------------------------------------------------
// Eine statische Instanz der Klasse LoopCheck
// Darüber wird das Zeitverhalten gesteuert (Software-Timer) und geprüft

// ----------------------------------------------------------------------------
nRF52840Radio bleCom;
// ----------------------------------------------------------------------------
// Eine statische Instanz der Klasse nRF52840Radio
// Darüber wird direkt die CPU (nRF52840) auf dem Arduino-Board zum Senden und
// Empfangen von BLE-Beacons angesprochen.


#define bleCycleTime 250
// ----------------------------------------------------------------------------
BlePoll       blePoll((IntrfRadio *) &bleCom, micros);
// ----------------------------------------------------------------------------
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
//              was hier einer Auflösung von 250 Mikrosekunden entspricht.

#define NrOfSlavesToPoll  5
// Die Anzahl der Slaves, die der Master aufrufen soll (Polling).
// Es wird grundsätzlich mit der Adresse 1 begonnen und nach dem Aufruf die
// Adresse inkrementiert, also immer die Slaves Adr = 1 bis
// Adr = NrOfSlavesToPoll+1 aufgerufen.
// ACHTUNG!
// Diese Zahl muss kleiner/gleich der in BlePoll.h definierten maximalen
// Anzahl der Slaves (MAXSLAVE) sein, weil die Ressourcen darüber statisch
// festgelegt werden.

Value2Midi    val2midArr[NrOfSlavesToPoll + 1];
Posture2Midi  post2midArr[NrOfSlavesToPoll + 1];
// Jeder Slave bekommt spezifische Value->Midi-Parameter.
// Index 0 steht für eventuelle Parameter des Masters

SerParams     ttyParams;
// ----------------------------------------------------------------------------
nRF52840Ser   tty;
// ----------------------------------------------------------------------------
// Eine statische Instanz der Klasse nRF52840Ser (UART)
// Darüber werden die seriellen Schnittstellen (UARTE0 und UARTE1) des
// nRF52840 bedient.
// Die Parameter werden in einer Struktur <SerParams> über die Funktion
// <begin(...)> gesetzt.

#define       sndBufSize  256
byte          sndBuffer[sndBufSize];
// ----------------------------------------------------------------------------
ComRingBuf    crb;
// ----------------------------------------------------------------------------
// Eine statische Instanz der Klasse <ComRingBuf>
// Damit wird ein Ringpuffer aufgebaut, der eine serielle Schnittstelle bedient.
// Der Speicher muss extra eingerichtet und mit der Funktion
//  <setWriteBuffer(sndBufSize, sndBuffer)> übergeben werden.
// Die Klasse für die serielle Schnittstelle muss von <IntrfSerial> abgeleitet
// sein, die Instanz wird mit der Funktion <begin(...)> übergeben.

#define MidiCycle     1000
MidiNotes  midi1,midi2;

#define appCycleTime 1000
StateMachine  ap(apInit, NULL, appCycleTime);
// Eine statische Instanz für die Zustandsmaschine, die hier für die
// Anwendung (App) von SOAAP eingesetzt wird
// ----- Parameter ------------------------------------------------------------
// <smInit>       Der zuerst aufgerufene Zustand (Funktion). Weitere Zustände
//                werden in den weiteren Zustandsfunktionen eingesetzt.
// <NULL>         Hier kann eine weitere Zustandsfunktion angegeben werden,
//                die dann grundsätzlich vor dem Verzweigen in einen Zustand
//                aufgerufen wird.
// <smCycleTime>  Die Zukluszeit (Takt) der Zustandsmaschine in Mikrosekunden


#ifdef DebugTerminal
// ----------------------------------------------------------------------------
// Zum Debuggen und weitere Analysen der Programmumgebung und Funktionstests
// Es ist ein (richtiges) Terminal erforderlich, mit dem einzelnen Zeichen
// direkt abgeschickt und die eintreffenden direkt angezeigt werden.
// ----------------------------------------------------------------------------
#define smCycleTime 5
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

char *infoThis =
{
"SOAAP BLE Master Midi Version 22.09.24-1\r\n"
"c0  Abschalten der periodischen Meldung\r\n"
"c1  Auslesen BlePoll-Debug-Register\r\n"
"c2  Steuern/Analysieren des Polling\r\n"
"c3  Werte auslesen\r\n"
"c4  Anwendung analysieren\r\n"
"c5  Meldung an Slave senden\r\n"
};


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
// ----------------------------------------------------------------------------
#endif


BlePoll::AppType  appType;

// ============================================================================
void setup()
// ============================================================================
{
  bleCom.begin();           // Initialisierung der Datenübertragung
  //bleCom.setPower(0x08);     // Maximale Sendeleistung bei nRF52840
  // TEST
  bleCom.setPower(0x0FC);   // Reduzierte Sendeleistung beim Schreibtisch-Test

  //appType = BlePoll::atDevSOAAP;
  appType = BlePoll::atSOAAP2;

  blePoll.begin(BlePoll::ctMASTER, NrOfSlavesToPoll, appType, 10000);
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
  // <1000>   Time-Out (Zeit für Slave zum Antworten) in Mikrosekunden

#ifdef DebugTerminal
  blePoll.stopEP();       // Das Polling muss extra gestartet werden
  mon.setInfo(infoThis);  // Info-Meldung für Monitor
#endif

  // Initialisierung von serieller Schnittstelle und Ringpuffer
  // --------------------------------------------------------------------------
  ttyParams.inst    = 0;    // Instanzindex der Schnittstelle (0,1)
  ttyParams.rxdPort = 1;    // Nummer des IO-Port mit RxD-Pin
  ttyParams.rxdPin  = 10;   // Nummer des RxD-Pin am Port
  ttyParams.txdPort = 1;    // Nummer des IO-Port mit TxD-Pin
  ttyParams.txdPin  = 3;    // Nummer des TxD-Pin am Port
  ttyParams.speed   = Baud31250;    // Enumerator für Bitrate
  ttyParams.type    = stCur;        // Stromschleife

  tty.begin(&ttyParams, (IntrfBuf *) &crb);
  // Übergeben von Parametern und Referenz auf Ringpufferverwaltung
  // für die Übergabe empfangener Zeichen

  tty.startSend();    // Sendebetrieb aktivieren

  crb.setWriteBuffer(sndBufSize, sndBuffer);
  // Speicher an Ringpufferverwaltung übergeben

  crb.begin((IntrfSerial *) &tty);
  // Referenz auf Schnittstelle an Ringpufferverwaltung
  // für die Übergabe zu sendender Zeichen

  setParM1();
  setParP1();
  midi1.begin(120, nd32, MidiCycle, (IntrfBuf *) &crb);
  midi1.setChannel(1);

  setParM2();
  setParP2();
  midi2.begin(120, nd32, MidiCycle, (IntrfBuf *) &crb);
  midi2.setChannel(2);
  //midi2.stop();
}

// ============================================================================
void loop()
// ============================================================================
{
  lc.begin();     // Muss am Anfang von LOOP aufgerufen werden
  // --------------------------------------------------------------------------

#ifdef DebugTerminal
  mon.run();      // Der Monitor bekommt bei jedem Durchlauf die CPU
#endif

  // Alle 250 Mikrosekunden erfolgt der Aufruf des Ble-Polling
  //
  if(lc.timerMicro(lcTimer0, bleCycleTime, 0))
    blePoll.run();
  // ----- Parameter der Software-Timer ---------------------------------------
  // <lcTimer0>   Id/Nummer des Timer, zur Zeit werden bis 10 Timer unterstützt
  //              lcTimer0 bis lcTimer9 (einstellbar in LoopCheck.h)
  // <bleCycleTime>   Ablaufzeit in Einheit des Timer-Typ (Micro/Milli)
  //                  hier in Mikrosekunden (timerMicro)
  // <0>          Anzahl der Wiederholungen, 0 = unbegrenzt

  // Alle <appCycleTime> Mikrosekunden erfolgt der Aufruf der Anwendung
  //
  if(lc.timerMicro(lcTimer1, appCycleTime, 0, 10000))
    ap.run();

  // Alle <MidiCycle> Mikrosekunden erfolgt der Aufruf des Midi-Controller
  //
  if(lc.timerMicro(lcTimer2, MidiCycle, 0, 1000))
  {
    midi1.run();
    midi2.run();
  }

#ifdef DebugTerminal
  // Jede Sekunde erfolgt die Ausgabe einer Versionsmeldung
  // das kann über c0 am Terminal abgeschaltet werden
  //
  if(lc.timerMilli(lcTimer3, 1000, 0))
  {
    if(!mon.cFlag[0])
      mon.printcr((char *)"%@TestBleMidiMaster, Version 20221018");
  }
  // Die Zeichen %@ am Anfang steuern die Ausgabe bei AndroidMonTerm in ein
  // Textfeld (Label) statt auf das Terminal-Display

  // Jede Millisekunde erfolgt der Aufruf der Zustandsmaschine
  //
  if(lc.timerMilli(lcTimer4, smCycleTime, 0))
  {
    sm.run();
  }
#endif

  // --------------------------------------------------------------------------
  lc.end();       // Muss vor dem Ende von LOOP aufgerufen werden
}

// ****************************************************************************
// Z u s t a n d s m a s c h i n e   S O A A P - A n w e n d u n g  (ap)
// ****************************************************************************
//
byte  apTmpByteArray[256];              // Zwischenspeicher für Zeichenfolgen
int   apNrOfMeasBytes;                  // Anzahl der empfangenen Messwertbytes
byte  apMeasByteArray[32];              // Zwischenspeicher für Messwerte
byte  apSlaveList[NrOfSlavesToPoll];    // Merker für gepollte Slaves
int   apNrOfSlaves;                     // Aktuelle Anzahl von Slaves
int   curListIdx;                       // Aktueller Index für Slaveliste
int   area;                             // Area des aktuellen Slave
int   sMsgLen;                          // Länge einer SOAAP-Meldung
int   slNr;                             // Slave-Nummer (Adresse)
int   txNr;                             // Anzahl versendeter Zeichen

PlpType   pAppId;                       // Anwendungs-Id aus Polling-Sicht

int   lastNoteIdxM1 = 0;
int   lastNoteIdxM2 = 0;

#ifdef TEST001
char testMsgBuf[256];
#endif
// ----------------------------------------------------------------------------
// Initialisierungen
//
dword apInitCnt;
void apInit()
{
  apInitCnt++;

  midi1.setNoteType(MidiNotes::nti8);
  lastNoteIdxM1 = midi1.addChordNote(MidiNotes::nti8, SchlossC, 10);
  midi1.setOpMode(momSequence);
  midi2.setNoteType(MidiNotes::nti8);
  lastNoteIdxM2 = midi2.addChordNote(MidiNotes::nti8, Kammerton, 10);
  midi2.setOpMode(momSequence);
  ap.enter(apWaitDE);
}

// ----------------------------------------------------------------------------
// Warten, bis Datenaustausch Master/Slave erfolgt
//
dword apWaitDECnt;
void apWaitDE()
{
  apWaitDECnt++;

  if(!blePoll.DataExchange)
    return;   // Verbleiben in diesem Zustand bis Leerpolling beendet

  apNrOfSlaves = blePoll.getSlaveList(apSlaveList, NrOfSlavesToPoll);
  // Ermitteln der angeschlossenen Slaves

  ap.enter(apWaitMeas);
}

// ----------------------------------------------------------------------------
// Warten auf neuen Messwert von einem Slave
//
dword apWaitMeasCnt;
void apWaitMeas()
{
  apWaitMeasCnt++;

  // Ermitteln, ob einer der Slaves einen Messwert hat
  //
  for(curListIdx = 0; curListIdx < apNrOfSlaves; curListIdx++)
  {
    slNr = apSlaveList[curListIdx];
    if(blePoll.measAvail(slNr)) break;
  }
  if(curListIdx == apNrOfSlaves) return;
  // Wenn kein Slave neue Messwerte hat,
  // dann im nächsten Zustandstakt Abfrage wiederholen

  // Slave (curListIdx) hat Messwerte übermittelt
  // diese werden mit dem nächsten Takt verarbeitet
  ap.enter(apProcMeas);
}

// ----------------------------------------------------------------------------
// Verarbeiten der Daten vom Slave
//
dword apProcMeasCnt;
void apProcMeas()
{
  apProcMeasCnt++;

  // Parameter und Daten für die SOAAP-Ausgabe holen
  //
  slNr = apSlaveList[curListIdx];
  area = blePoll.getArea(slNr);
  pAppId = blePoll.getAppId(slNr);
  apNrOfMeasBytes = blePoll.getMeas(slNr, apMeasByteArray);

  // Spezifisch je Slave auswerten
  //
  if(slNr == 1 || slNr == 2)
  {
    ap.enter(apCheckValues);
    return;
  }

  // Auf nächsten Messwert von einem Slave warten
  //
  ap.enter(apWaitMeas);
}

// ----------------------------------------------------------------------------
// Daten überprüfen (auswerten)
//
short accXold, accYold, accZold;
float rollOld, pitchOld, yawOld;
dword apCheckValuesCnt;
void apCheckValues()
{
  apCheckValuesCnt++;

  switch (appType)
  {
   case BlePoll::atSOAAP1:
     //bool newData = false;

     accXold = * (short *) &apMeasByteArray[6];
     accYold = * (short *) &apMeasByteArray[8];
     accZold = * (short *) &apMeasByteArray[10];

     break;

   case BlePoll::atSOAAP2:
     rollOld  = * (float *) &apMeasByteArray[0];
     pitchOld = * (float *) &apMeasByteArray[4];
     yawOld   = * (float *) &apMeasByteArray[8];

     break;
  }

  ap.enter(apCalcResult);
}

// ----------------------------------------------------------------------------
// Daten auf Midi-Noten abbilden
//
//short borderLowAz = 100;
//short borderHighAz = 8000;
//short borderLowAy = 100;
//short borderHighAy = 8300;
//short borderLowAx = 100;
//short borderHighAx = 4000;

//byte  lowNote = 23;
//byte  highNote = 72;
//byte highNote = 96;

void setParM1()
{
  val2midArr[1].borderLowAz = 100;
  val2midArr[1].borderHighAz = 8000;
  val2midArr[1].borderLowAy = 100;
  val2midArr[1].borderHighAy = 8300;
  val2midArr[1].borderLowAx = 100;
  val2midArr[1].borderHighAx = 4000;
  val2midArr[1].lowNote = 23;
  val2midArr[1].highNote = 96;
}

void setParM2()
{
  val2midArr[2].borderLowAz = 100;
  val2midArr[2].borderHighAz = 8000;
  val2midArr[2].borderLowAy = 100;
  val2midArr[2].borderHighAy = 8300;
  val2midArr[2].borderLowAx = 100;
  val2midArr[2].borderHighAx = 4000;
  val2midArr[2].lowNote = 23;
  val2midArr[2].highNote = 96;
}

void setParP1()
{
  Posture2MidiPtr pmp = &post2midArr[1];

  pmp->offsetRoll               = 90;
  pmp->borderLowRoll            = 45;
  pmp->borderHighRoll           = 170;
  pmp->aimRoll                  = NoteType;

  pmp->offsetPitch              = 90;
  pmp->borderLowPitch           = 45;
  pmp->borderHighPitch          = 170;
  pmp->aimPitch                 = NoteVal;

  pmp->midiBords[NoteVal].low   = 23;
  pmp->midiBords[NoteVal].high  = 96;
  pmp->midiBords[NoteType].low  = 2;
  pmp->midiBords[NoteType].high = 12;

  pmp->koeffRoll =
      (pmp->midiBords[pmp->aimRoll].high - pmp->midiBords[pmp->aimRoll].low) /
      (pmp->borderHighRoll - pmp->borderLowRoll);

  pmp->koeffPitch =
      (pmp->midiBords[pmp->aimPitch].high - pmp->midiBords[pmp->aimPitch].low) /
      (pmp->borderHighPitch - pmp->borderLowPitch);
}

void setParP2()
{
  Posture2MidiPtr pmp = &post2midArr[2];

  pmp->offsetRoll               = 90;
  pmp->borderLowRoll            = 45;
  pmp->borderHighRoll           = 170;
  pmp->aimRoll                  = NoteType;

  pmp->offsetPitch              = 90;
  pmp->borderLowPitch           = 45;
  pmp->borderHighPitch          = 170;
  pmp->aimPitch                 = NoteVal;

  pmp->midiBords[NoteVal].low   = 23;
  pmp->midiBords[NoteVal].high  = 96;
  pmp->midiBords[NoteType].low  = 2;
  pmp->midiBords[NoteType].high = 12;

  pmp->koeffRoll =
      (pmp->midiBords[pmp->aimRoll].high - pmp->midiBords[pmp->aimRoll].low) /
      (pmp->borderHighRoll - pmp->borderLowRoll);

  pmp->koeffPitch =
      (pmp->midiBords[pmp->aimPitch].high - pmp->midiBords[pmp->aimPitch].low) /
      (pmp->borderHighPitch - pmp->borderLowPitch);
}


byte  resultAz;
byte  resultAy;
byte  resultAx;

byte  resultNote[3];

dword apCalcResultCnt;
void apCalcResult()
{
  short     testY;
  int       result;
  float     rollVal, pitchVal, yawVal;
  Meas2Midi meas2midi;

  apCalcResultCnt++;

  switch (appType)
  {
    case BlePoll::atSOAAP1:
      if((accZold < val2midArr[slNr].borderLowAz) && (accXold < val2midArr[slNr].borderLowAx))
      {
        ap.enter(apWaitMeas);
        return;
      }

      testY = accZold + accYold / 4;
      result = val2midArr[slNr].lowNote
          + (val2midArr[slNr].highNote * testY) / val2midArr[slNr].borderHighAz;
      if(result > val2midArr[slNr].highNote)
        resultAz = val2midArr[slNr].highNote;
      else if (result < val2midArr[slNr].lowNote)
        resultAz = val2midArr[slNr].lowNote;
      else
        resultAz = result;

      testY = accXold + accYold / 4;
      result = (MidiNotes::nti32 * testY) / val2midArr[slNr].borderHighAz;
      if(result < MidiNotes::nti2) result = MidiNotes::nti2;
      if(result > MidiNotes::nti32) result = MidiNotes::nti32;
      resultAx = result;
      break;

    case BlePoll::atSOAAP2:
      rollVal = rollOld + post2midArr[slNr].offsetRoll; // Rollwinkel in positiven Bereich verschieben
      if(rollVal < post2midArr[slNr].borderLowRoll)
      {
        rollVal = post2midArr[slNr].borderLowRoll;      // Inhalt ggf. auf Minimalwert begrenzen
      }
      else if(rollVal > post2midArr[slNr].borderHighRoll)
      {
        rollVal = post2midArr[slNr].borderHighRoll;     // Inhalt ggf. auf Maximalwert begrenzen
      }
      meas2midi = post2midArr[slNr].aimRoll;
      resultNote[meas2midi] =
          post2midArr[slNr].midiBords[meas2midi].low +
          (byte) ((rollVal - post2midArr[slNr].borderLowRoll) * post2midArr[slNr].koeffRoll);

      pitchVal = pitchOld + post2midArr[slNr].offsetPitch; // Pitchwinkel in positiven Bereich verschieben
      if(pitchVal < post2midArr[slNr].borderLowPitch)
      {
        pitchVal = post2midArr[slNr].borderLowPitch;    // Inhalt ggf. auf Minimalwert begrenzen
      }
      else if(pitchVal > post2midArr[slNr].borderHighPitch)
      {
        pitchVal = post2midArr[slNr].borderHighPitch;   // Inhalt ggf. auf Maximalwert begrenzen
      }
      meas2midi = post2midArr[slNr].aimPitch;
      resultNote[meas2midi] =
          post2midArr[slNr].midiBords[meas2midi].low +
          (byte) ((pitchVal - post2midArr[slNr].borderLowPitch) * post2midArr[slNr].koeffPitch);
      resultNote[NoteVel] = 100;
      break;
  }

  ap.enter(apSetResult);
}

// ----------------------------------------------------------------------------
// Bedienen des Midi-Controller
//
dword apSetResultCnt;
void apSetResult()
{
  apSetResultCnt++;

  switch (appType)
  {
    case BlePoll::atSOAAP1:
      if(slNr == 1)
        midi1.setChordNote(lastNoteIdxM1, (MidiNotes::NoteTypeIdx) resultAx, resultAz, 100);
      else if(slNr == 2)
        midi2.setChordNote(lastNoteIdxM2, (MidiNotes::NoteTypeIdx) resultAx, resultAz, 100);
      break;

    case BlePoll::atSOAAP2:
      if(slNr == 1)
        midi1.setChordNote
        (
          lastNoteIdxM1,
          (MidiNotes::NoteTypeIdx) resultNote[NoteType],
          resultNote[NoteVal],
          resultNote[NoteVel]
        );
      else if(slNr == 2)
        midi2.setChordNote
        (
          lastNoteIdxM1,
          (MidiNotes::NoteTypeIdx) resultNote[NoteType],
          resultNote[NoteVal],
          resultNote[NoteVel]
        );
      break;
  }

  ap.enter(apWaitMeas);
}

byte  testValue = 22;

void apTestController()
{
  midi1.setChordNote(lastNoteIdxM1, MidiNotes::nti4, testValue, 60);
  testValue++;
  if(testValue > 96)
    testValue = 22;
  ap.setDelay(100);
}



#ifdef DebugTerminal
// ****************************************************************************
// Z u s t a n d s m a s c h i n e   z u m   D e b u g g e n   (sm)
// ****************************************************************************
//
dword   debDword;
byte    tmpByteArray[256];


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
  else if(mon.cFlag[4] && !mon.busy)
    sm.enter(smCheckApp);
  else if(mon.cFlag[5] && !mon.busy)
    sm.enter(smMode4Slave);
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

char *smPollHelp =
{
    "C   Pollzähler zurücksetzen\r\n"
    "L   Liste der Slaves anzeigen\r\n"
    "P   Polling starten/stoppen/fortsetzen\r\n"
    "R   Radiopuffer auslesen (16 Zeichen)\r\n"
    "S   Sendepuffer auslesen (16 Zeichen)\r\n"
    "T   Übertragungsstatistik und Daten anzeigen\r\n"
    "0...9  Slave-Umgebung\r\n"
};

TxStatistics txStatistics;

void smCtrlPolling()
{
  dword   tmpDw;
  short   tmpShort;
  int     i;

  PlpMeas6Ptr resPtr;

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
    bleCom.getPduMem(tmpByteArray, 0, 16);
    mon.println(tmpByteArray, 16, ' ');
    sm.resetEnter();
  }


  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'S' || mon.lastKeyIn == 's')
  {
    mon.print((char *) "Sendepuffer = ");
    bleCom.getPduSentS(tmpByteArray, 0, 16);
    mon.println(tmpByteArray, 16, ' ');
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

    resPtr = (PlpMeas6Ptr) &slPtr->result;

    mon.print(slPtr->cntLostPdu); mon.cprint('|');
    mon.print(slPtr->cntErrCrc); mon.cprint('|');
    //mon.print(slPtr->result.measCnt); mon.cprint('|');
    mon.print(resPtr->measCnt); mon.cprint('|');
    mon.print(slPtr->cntLostMeas); mon.cprint(' ');

    for(i = 0; i < 6; i++)
    {
      //tmpShort = (short) slPtr->result.meas[i];
      tmpShort = (short) resPtr->meas[i];
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
// Zugriff auf die Sensordaten
// ----------------------------------------------------------------------------
//

char *rpValHelp =
{
"0-9  Sensordaten\r\n"
"Leerzeichen für Abbruch\r\n"
};

void smReadPollValues()
{
  PlPduMeasPtr  resultPtr;
  PlPduMeasPtr  ctrlPtr;
  byte          appId;
  float         tmpFloat;
  char          conv[32];

  if(sm.firstEnter())
  {
    mon.print((char *) "Werte vom Sensor ");
    mon.lastKeyIn = ':';
  }

  if(mon.lastKeyIn == ':') return;

  if(mon.lastKeyIn == 'H' || mon.lastKeyIn == 'h')
  {
    mon.println();
    mon.println(rpValHelp);
    sm.resetEnter();
  }
  // --------------------------------------------------------------------------
  else if(mon.lastKeyIn >= '0' && mon.lastKeyIn <= '9')
  {
    int idx = mon.lastKeyIn & 0x0F;
    SlavePtr slPtr = blePoll.getSlavePtr(idx);
    PollStatePtr pPtr = blePoll.getPollPtr(slPtr->pIdx);
    resultPtr = (PlPduMeasPtr) &slPtr->result;
    ctrlPtr = (PlPduMeasPtr) &slPtr->control;
    appId = resultPtr->appId;

    mon.print((char *) "Slave[");
    mon.print(idx);
    mon.print((char *) "] pduCnt=");
    mon.print(resultPtr->counter);
    mon.print((char *) " pduType=");
    mon.print(resultPtr->type);
    mon.print((char *) " appId=");
    mon.print(resultPtr->appId);
    mon.print((char *) " measCnt=");
    mon.println(resultPtr->measCnt);

    mon.println(resultPtr->plData,27,' ');

    switch (appId)
    {
      case plptIMU3F4Ctrl4:
        mon.print("AppId: plptIMU3F4Ctrl4");
        mon.print(" Roll=");
        tmpFloat = ((PlpI3S4C4Ptr) resultPtr)->meas[0];
        sprintf(conv,"%f",tmpFloat);
        mon.print(conv);
        mon.print(" Pitch=");
        tmpFloat = ((PlpI3S4C4Ptr) resultPtr)->meas[1];
        sprintf(conv,"%f",tmpFloat);
        mon.print(conv);
        break;

      default:
        mon.print(" Unbekannte AppId");
        break;
    }
    mon.println();
    sm.resetEnter();
  }
  else
  {
    if(mon.lastKeyIn == ' ')
    {
      mon.cFlag[3] = false;
      mon.print((char *) "-- Schleifenabbruch - drücke Enter");
      sm.enter(smCheckJobs);
    }
  }

}

// ----------------------------------------------------------------------------
// Analysieren der Anwendung
// ----------------------------------------------------------------------------
//
char *caHelp =
{
"c Zyklen der Zustandsmaschine\r\n"
"m Ausgabe Midiwerte\r\n"
"Leerzeichen für Abbruch\r\n"
};

void smCheckApp()
{
  if(sm.firstEnter())
  {
    mon.print((char *) "Check App ");
    mon.lastKeyIn = ':';
  }

  if(mon.lastKeyIn == ':') return;

  if(mon.lastKeyIn == 'H' || mon.lastKeyIn == 'h')
  {
    mon.println();
    mon.println(caHelp);
    sm.resetEnter();
  }
  else if(mon.lastKeyIn == 'C' || mon.lastKeyIn == 'c')
  {
    mon.cprintln('c');
    mon.print(apInitCnt); mon.cprint(' ');
    mon.print(apWaitDECnt); mon.cprint(' ');
    mon.print(apWaitMeasCnt); mon.cprint(' ');
    mon.print(apProcMeasCnt); mon.cprint(' ');
    mon.print(apCheckValuesCnt); mon.cprint(' ');
    mon.print(apCalcResultCnt); mon.cprint(' ');
    mon.println(apSetResultCnt);
    sm.resetEnter();
  }
  else if(mon.lastKeyIn == 'M' || mon.lastKeyIn == 'm')
  {
    mon.cprintln('m');
    mon.print(slNr); mon.cprint(' ');
    mon.print(resultNote[NoteType]); mon.cprint(' ');
    mon.print(resultNote[NoteVal]); mon.cprint(' ');
    mon.println(resultNote[NoteVel]);
    sm.resetEnter();
  }

  else if(mon.lastKeyIn == ' ')
  {
    mon.cFlag[4] = false;
    mon.print((char *) "-- Schleifenabbruch - drücke Enter");
    sm.enter(smCheckJobs);
  }
}

// ----------------------------------------------------------------------------
// Modus (Polling-Info, Steuerung) an einen Slave
// ----------------------------------------------------------------------------
// ACHTUNG!
// Diese Testfunktion ist allgemein gehalten. Tatsächlich wird zur Zeit noch
// nicht der Datentyp ausgewertet und immer die ersten zwei Zeichen Inhalt
// zum Slave übertragen
//
char *msHelp =
{
"1. 1..5 Slaveadresse\r\n"
"2. 0..9 Modustyp oder A für Antwort\r\n"
"   xyz  Modus, Senden mit TAB\r\n"
"Leerzeichen für Abbruch\r\n"
};

int   inIdx = 0;
int   msSlaveNr = 0;
int   msModeType = 0;

CtrlResp2 ctrlResp;
byte      oldProcCnt = 0;

void smMode4Slave()
{
  //CtrlData2Ptr ctrlDataPtr;

  if(sm.firstEnter())
  {
    mon.print((char *) "Meldung an Slave[");
    mon.lastKeyIn = ':';
    inIdx = 0;
  }

  if(mon.lastKeyIn == ':') return;

  if(mon.lastKeyIn == 'H' || mon.lastKeyIn == 'h')
  {
    mon.println();
    mon.println(msHelp);
    sm.resetEnter();
  }
  else if(mon.lastKeyIn >= '1' && mon.lastKeyIn <= '5' && inIdx == 0)
  {
    mon.cprint(mon.lastKeyIn);
    msSlaveNr = mon.lastKeyIn & 0x0F;
    mon.print("] Typ = ");
    inIdx = 1;
    mon.lastKeyIn = ':';
  }
  else if(mon.lastKeyIn >= '0' && mon.lastKeyIn <= '9' && inIdx == 1)
  {
    mon.cprint(mon.lastKeyIn);
    msModeType = mon.lastKeyIn & 0x0F;
    sm.enter(smMode4SlaveIn);
  }
  else if((mon.lastKeyIn == 'A' || mon.lastKeyIn == 'a') && inIdx == 1)
  {
    mon.print("Antwort: {");
    blePoll.getCtrlResp(msSlaveNr, &ctrlResp);

    if(ctrlResp.procCnt != oldProcCnt)
    {
      oldProcCnt = ctrlResp.procCnt;
      mon.cprint(ctrlResp.ctrl[0]);
      mon.cprint(ctrlResp.ctrl[1]);
    }
    mon.cprintln('}');
    sm.resetEnter();
  }

  else if(mon.lastKeyIn == ' ')
  {
    mon.cFlag[5] = false;
    mon.print((char *) "-- Schleifenabbruch - drücke Enter");
    sm.enter(smCheckJobs);
  }
}

char mode2Snd[8];

void smMode4SlaveIn()
{
  if(sm.firstEnter())
  {
    mon.print((char *) " {");
    mon.lastKeyIn = ':';
    inIdx = 0;
  }

  if(mon.lastKeyIn == ':') return;

  if(mon.lastKeyIn == ' ')
  {
    mon.cFlag[5] = false;
    mon.print((char *) "-- Schleifenabbruch - drücke Enter");
    sm.enter(smCheckJobs);
  }

  else if(mon.lastKeyIn == '\t' || inIdx > 5)
  {
    blePoll.updControl(msSlaveNr, (byte *) mode2Snd, 2);
    mon.println("} gesendet");
    sm.enter(smMode4Slave);
  }

  else
  {
    mode2Snd[inIdx] = mon.lastKeyIn;
    mon.cprint(mon.lastKeyIn);
    mon.lastKeyIn = ':';
    inIdx++;
  }

}

// ----------------------------------------------------------------------------
// Debug-Informationen
// ----------------------------------------------------------------------------
//

#endif // DebugTerminal

