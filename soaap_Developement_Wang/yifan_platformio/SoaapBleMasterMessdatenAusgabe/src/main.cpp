// ----------------------------------------------------------------------------
//                              SoaapBleMaster.ino
// Beispielhafte Anwendung SOAAP / Steuerung optischer und akustischer Ausgaben
//      Kommunikation über BLE-Funkanäle mit Bewerbungstelegrammen
//                        P o l l i n g - M a s t e r
// ----------------------------------------------------------------------------
// Editor:  Robert Patzke
// URI/URL: www.hs-hannover.de
//-----------------------------------------------------------------------------
// Lizenz:  CC-BY-SA  (wikipedia: Creative Commons)
// Datum:   1. November 2021
// Letzte Bearbeitung: 4. Februar 2022
//

#include  "Arduino.h"
#include  "SoaapBleMaster.h"


void devSendDataML(void);
// ----------------------------------------------------------------------------
LoopCheck     lc;
// ----------------------------------------------------------------------------
// Eine statische Instanz der Klasse LoopCheck
// Darüber wird das Zeitverhalten gesteuert (Software-Timer) und geprüft

// @yifan 
// soergt sich fuer die Echtzeitsystem, in loop werden 5 timer starten

// ----------------------------------------------------------------------------
nRF52840Radio bleCom;
// ----------------------------------------------------------------------------
// Eine statische Instanz der Klasse nRF52840Radio
// Darüber wird direkt die CPU (nRF52840) auf dem Arduino-Board zum Senden und
// Empfangen von BLE-Beacons angesprochen.

// @yifan
// physikalische Ebene


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

SerParams     ttyParams; 
// ----------------------------------------------------------------------------
nRF52840Ser   tty;
// ----------------------------------------------------------------------------
// Eine statische Instanz der Klasse nRF52840Ser (UART)
// Darüber werden die seriellen Schnittstellen (UARTE0 und UARTE1) des
// nRF52840 bedient.
// Die Parameter werden in einer Struktur <SerParams> über die Funktion
// <begin(...)> gesetzt.

// @yifan
// UART Kommunikation zwischen Master-nano und PC
// ist dazu noch nicht klar 

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

// @yifan
// um Datensuebertragung zu beschleunigen, Die Serial-Bib von Arduino stoppt Daten 
// zu senden, wenn der Buf voll ist, dann wartet darauf, der Buf leer ist, zu langsam

#define appCycleTime 500
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


// ----------------------------------------------------------------------------
SoaapMsg  sMsg;
// ----------------------------------------------------------------------------
// Eine statische Instanz der Klasse <SoaapMsg>
// Damit werden SOAAP-spezifische Meldungen/Telegramme generiert und ausgewertet
// und SOAAP-spezifische Datentypen und Parameter definiert.

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


// ============================================================================
void setup()
// ============================================================================
{
  bleCom.begin();           // Initialisierung der Datenübertragung
  //bleCom.setPower(0x08);     // Maximale Sendeleistung bei nRF52840
  // TEST
  bleCom.setPower(0x0FC);   // Reduzierte Sendeleistung beim Schreibtisch-Test

  blePoll.begin(BlePoll::ctMASTER, NrOfSlavesToPoll, BlePoll::atDevSOAAP, 10000);
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
  //blePoll.stopEP();   // Das Polling muss extra gestartet werden
    if(blePoll.stoppedEP())
    {
      blePoll.resumeEP();}
#endif

  // Initialisierung von serieller Schnittstelle und Ringpuffer
  // --------------------------------------------------------------------------
  ttyParams.inst    = 0;    // Instanzindex der Schnittstelle (0,1)
  ttyParams.rxdPort = 1;    // Nummer des IO-Port mit RxD-Pin
  ttyParams.rxdPin  = 10;   // Nummer des RxD-Pin am Port
  ttyParams.txdPort = 1;    // Nummer des IO-Port mit TxD-Pin
  ttyParams.txdPin  = 3;    // Nummer des TxD-Pin am Port
  ttyParams.speed   = Baud115200;   // Enumerator für Bitrate
  ttyParams.type    = stStd;        // Spannungsausgang

  tty.begin(&ttyParams, (IntrfBuf *) &crb);
  // Übergeben von Parametern und Referenz auf Ringpufferverwaltung
  // für die Übergabe empfangener Zeichen

  tty.startSend();    // Sendebetrieb aktivieren

  crb.setWriteBuffer(sndBufSize, sndBuffer);
  // Speicher an Ringpufferverwaltung übergeben

  crb.begin((IntrfSerial *) &tty);
  // Referenz auf Schnittstelle an Ringpufferverwaltung
  // für die Übergabe zu sendender Zeichen
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

  // Alle 500 Mikrosekunden erfolgt der Aufruf der Anwendung
  //
  if(lc.timerMicro(lcTimer1, appCycleTime, 0, 10000))
    ap.run();


#ifdef DebugTerminal
  // Jede Sekunde erfolgt die Ausgabe einer Versionsmeldung
  // das kann über c0 am Terminal abgeschaltet werden
  //
  if(lc.timerMilli(lcTimer2, 1000, 0))
  {
    //if(!mon.cFlag[0])
      //mon.printcr((char *)"%@TestBleMaster (ttyACM0), Version 20220303");
      //smWaitPolling();
  }
  // Die Zeichen %@ am Anfang steuern die Ausgabe bei AndroidMonTerm in ein
  // Textfeld (Label) statt auf das Terminal-Display

  // Alle 5 Millisekunden erfolgt der Aufruf der Zustandsmaschine
  //


  //


  //Debug



  /// 
  if(lc.timerMilli(lcTimer3, smCycleTime, 0))
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
int   apNrOfCtrlBytes;                  // Anzahl der empfangenen Steuerbytes inkl count und path
byte  apMeasByteArray[32];              // Zwischenspeicher für Messwerte
byte  apCtrlByteArray[32];              // Zwischenspeicher für Steuerbytes
byte  apSlaveList[NrOfSlavesToPoll];    // Merker für gepollte Slaves
int   apNrOfSlaves;                     // Aktuelle Anzahl von Slaves
int   curListIdx;                       // Aktueller Index für Slaveliste
int   area;                             // Area des aktuellen Slave
int   sMsgLen;                          // Länge einer SOAAP-Meldung
int   slNr;                             // Slave-Nummer (Adresse)
int   txNr;                             // Anzahl versendeter Zeichen

SoaapApId sAppId;                       // Anwendungs-Id aus Sicht SOAAP
PlpType   pAppId;                       // Anwendungs-Id aus Polling-Sicht

#ifdef TEST001
char testMsgBuf[256];
#endif
// ----------------------------------------------------------------------------
// Initialisierungen
//
void apInit()
{
#ifdef TEST001
  crb.putLine("Initialisierung");
#endif

  // Eventuelle Initialisierung
  ap.enter(apWaitDE);     // in nächsten Zustand schalten
}

// ----------------------------------------------------------------------------
// Warten, bis Datenaustausch Master/Slave erfolgt
//
void apWaitDE()
{
  if(!blePoll.DataExchange)
    return;   // Verbleiben in diesem Zustand bis Leerpolling beendet

  apNrOfSlaves = blePoll.getSlaveList(apSlaveList, NrOfSlavesToPoll);
  // Ermitteln der angeschlossenen Slaves

#ifdef TEST001
  crb.putStr("Slaveliste\r\n");
#endif

  ap.enter(apWaitMeas);
}

// ----------------------------------------------------------------------------
// Warten auf neuen Messwert von einem Slave
//
void apWaitMeas()
{
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

#ifdef TEST001
  crb.putStr("Messwerte\r\n");
#endif

  // Slave (curListIdx) hat Messwerte übermittelt
  // diese werden mit dem nächsten Takt verarbeitet
  ap.enter(apProcMeas);
}

// ----------------------------------------------------------------------------
// Verarbeiten der Daten vom Slave
//
void apProcMeas()
{
#ifdef TEST001
  if(ap.firstEnter())
  {
    slNr = apSlaveList[curListIdx];
    sprintf(testMsgBuf,"Slave-Nr = %d\r\n",slNr);
    crb.putStr(testMsgBuf);
  }
  //ap.enter(apWaitMeas);
  //return;
#endif

  // Parameter und Daten für die SOAAP-Meldung holen
  //
  slNr = apSlaveList[curListIdx];
  area = blePoll.getArea(slNr);
  pAppId = blePoll.getAppId(slNr);
  apNrOfMeasBytes = blePoll.getMeas(slNr, apMeasByteArray);
  apNrOfCtrlBytes = blePoll.getCtrlM(slNr, apCtrlByteArray);

#ifdef TEST001
  ap.enter(apWaitMeas);
  return;
#endif

  // Abbildung des Polling-Telegramm-Typs auf die SOAAP-Anwendungs-Id
  //
  switch(pAppId)
  {
    case plptMeas9:
      sAppId = saiDefaultMeas;
      break;

    case plptMeas13:
      sAppId = saiMaximalMeas;
      break;

    case plptMeas9Ctrl4:
      sAppId = saiDefaultMeasCtrl;
      break;

    default:
      sAppId = saiDefaultMeas;
      break;
  }

  // Konstruktion der SOAAP-Meldung
  //
  //sMsgLen = sMsg.getMsgA(area, slNr, sAppId, (char *) apTmpByteArray, apMeasByteArray);
  // Senden des Telegramm über serielle Schnittstelle
  //
  //txNr = crb.putStr((char *) apTmpByteArray);
  // Auf nächsten Messwert von einem Slave warten
  //
  devSendDataML();
  ap.enter(apWaitMeas);
}

void devSendDataML(void){
    PlpMeas9Ptr resPtr;
    short tmpShort;
    SlavePtr slPtr = blePoll.getSlavePtr(1);
    resPtr = (PlpMeas9Ptr) &slPtr->result;
      for (int i = 0; i < 9; i++)
      {
        // tmpShort = (short) slPtr->result.meas[i];
        tmpShort = (short)resPtr->meas[i];
        mon.prints(tmpShort);
        if(i<8)mon.cprint(' ');
      }
      mon.print("\n");
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
    sm.enter(smCheckSer);
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

  PlpMeas9Ptr resPtr;

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
    bleCom.getPduSentE(tmpByteArray, 0, 10);
    mon.println(tmpByteArray, 10, ' ');
    sm.resetEnter();
  }

    // --------------------------------------------------------------------------
  else if(mon.lastKeyIn == 'K' || mon.lastKeyIn == 'k')
  {
    mon.print(slNr); mon.print((char *)"|");
    for(int i= 0; i< apNrOfCtrlBytes; i++){
      mon.print(apCtrlByteArray[i]);
      mon.print((char *)" ");
    }
    mon.print((char *) "\n");
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

    resPtr = (PlpMeas9Ptr) &slPtr->result;

    mon.print(slPtr->cntLostPdu); mon.cprint('|');
    mon.print(slPtr->cntErrCrc); mon.cprint('|');
    //mon.print(slPtr->result.measCnt); mon.cprint('|');
    mon.print(resPtr->measCnt); mon.cprint('|');
    mon.print(slPtr->cntLostMeas); mon.cprint(' ');

    for(i = 0; i < 9; i++)
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
// Testen der seriellen Schnittstelle
// ----------------------------------------------------------------------------
//
void smCheckSer()
{
  if(sm.firstEnter())
  {
    mon.print((char *) "TestSer...");
    mon.lastKeyIn = ':';
  }

  if(mon.lastKeyIn == ':') return;

  if(mon.lastKeyIn == ' ')
  {
    mon.cFlag[4] = false;
    mon.print((char *) "-- Schleifenabbruch - drücke Enter");
    sm.enter(smCheckJobs);
  }
  else
  {
    crb.putChr(mon.lastKeyIn);
    mon.cprint(mon.lastKeyIn);
    mon.lastKeyIn = ':';
  }
}

// ----------------------------------------------------------------------------
// Debug-Informationen
// ----------------------------------------------------------------------------
//

void smReadPollValues()
{

}

#endif // DebugTerminal

