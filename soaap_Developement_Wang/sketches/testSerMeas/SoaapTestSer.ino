//-----------------------------------------------------------------------------
//                                S O A A P
//-----------------------------------------------------------------------------
// Thema:   Steuerung optischer und akust. Ausgaben für Performance-Künstler
//          Testen der Software für Arduino Due (Vermittler/Konfigurator)
// Datei:   SoaapTestSer.ino
// Editor:  Robert Patzke
// URI/URL: www.hs-hannover.de
//-----------------------------------------------------------------------------
// Lizenz:  CC-BY-SA  (siehe Wikipedia: Creative Commons)
//

#include "Arduino.h"

#include "SoaapTestSer.h"
#include "LoopCheck.h"

#if defined TEST002
#include "StateMachine.h"
#include "SoaapMsg.h"
#endif

LoopCheck lc;
// Hilfsmethoden zur Steuerung zeitlicher Abläufe

SoaapMsg  soaMsg;
// Hilfsmethoden für Telegrammaufbau und -auswertung

#if defined TEST002
#define showMeasCycleTime 500
StateMachine showMeas(showMeasInit, NULL, showMeasCycleTime);
#endif

void setup()      // **********************************************************
{
  Serial.begin(115200);
  Serial1.begin(115200);
}



byte        tmpBuffer[256];
// Temporärer Zwischenpuffer für Zeichenfolgen

int         nrBytesIn  = 0;     // Antahl aktuell empfangener Zeichen
int         serInCount = 0;     // Zähler für empfangene Zeichen
int         serInIdx   = 0;     // Index für besonderes Zeichen
byte        serInChar;          // Einzelnes empfangenes Zeichen
int         slaveAdr;           // Adresse des Soaap-Slave
int         slaveArea;          // Quellennetzwerk-Info, Bereich, o.ä.
SoaapApId   appId;              // Anwendungskennung, Datentyp, o.ä.

#if defined TEST002
int   measIdx;            // Index für den aktuellen Messwert
short measList[13];       // Liste der Messwerte
int   nrMeas;             // Anzahl der Messwerte im Telegramm
int   resMeas;            // Auflösung der Messwerte in Zeichen
int   viewDiv;            // Teiler für Anzeigehäufigkeit
#endif

void loop()       // **********************************************************
{                 // **********************************************************
  lc.begin();     // Unbedingt an den Anfang der Arduino-LOOP
  // --------------------------------------------------------------------------

#ifdef TEST001
  // Durchreichen der Mastertelegramme von Serial1 an Serial (Serial0)
  // --------------------------------------------------------------------------
  // In einer einfacheren Variante dieses Tests wurde auf eine Zählung der
  // eingetroffenen Zeichen verzichtet. Es wurde aber festgestellt, dass bei
  // der Ausgabe sporadisch Zeichen fehlten (Telegramm um 1 Zeichen verkürzt).
  // Zur Analyse wurde die Zählung der eingetroffenen Zeichen eingeführt und
  // die Anzahl vor dem Telegramm ausgegeben.
  // Seitdem trat der Zeichenverlust nicht mehr auf, eine Erklärung wurde
  // nicht gefunden
  // Dieser Test könnte auch noch einmal mit den alternativen, auf Interrupt
  // auch beim Senden basierenden, Klassen von Robert Patzke (MFP GmbH) durch-
  // geführt werden. Das wurde aus Zeitgründen noch nicht gemacht, auch weil
  // die Ausgabe des Telegramms für SOAAP weniger wichtig ist.

  nrBytesIn = Serial1.available();
  // Anzahl der über Serial1 eingetroffenen Bytes (Zeichen)

  if(nrBytesIn > 0)
  {
    Serial1.readBytes(tmpBuffer, nrBytesIn);

    for(int i = 0; i < nrBytebyte      tmpBuffer[256];
        // Zwischenpuffer für Zeichenfolgen

sIn; i++)
    {
      serInCount++;
      char c = (char) tmpBuffer[i];
      if(c < 0x20)
      {
        Serial.println();
        Serial.print(serInCount);
        serInCount = 0;
      }
      Serial.print(c);
    }
  }

#endif

#ifdef TEST002

  if(lc.timerMicro(lcTimer0,showMeasCycleTime,0))
    showMeas.run();

#endif

  // --------------------------------------------------------------------------
  lc.end();       // Unbedingt an das Ende der Arduino-LOOP
}


#ifdef TEST002
// ----------------------------------------------------------------------------
// Zustandsmaschine zur Messwertanzeige
// ----------------------------------------------------------------------------
//

// Initialisierungen für die Zustandsmaschine und Startmeldung
//
void showMeasInit()
{
  Serial.println("Anzeige der Messwerte mehrerer Soaap-Module.");
  showMeas.enter(showMeasWaitMsg);
  // Beim nächsten Takt zum Zustand <showMeasWaitMsg>.
}

// Warten auf ein Soaap-Telegramm von Serial 1
//
void showMeasWaitMsg()
{
  nrBytesIn = Serial1.available();
  // Anzahl der über Serial1 eingetroffenen Bytes (Zeichen)

  if(nrBytesIn < 1) return;
  // Beim nächsten Takt wieder in diesen Zustand, wenn kein Zeichen da

  for(serInIdx = 0; serInIdx < nrBytesIn; serInIdx++)
  {                               // Suchen nach Startzeichen
    serInChar = Serial1.read();
    if(serInChar < 0x20) break;
  }

  if(serInIdx == nrBytesIn) return;
  // Beim nächsten Takt wieder in diesen Zustand, wenn Startzeichen nicht dabei

  slaveArea = serInChar & 0x1F;   // Bereich auskodieren

  serInIdx = 0;     // Index neu setzen für Inhaltszuordnung
  showMeas.enter(showMeasHeader);
  // Beim nächsten Takt zum Zustand <showMeasHeader>
}

// Warten auf den Telegrammkopf und auswerten
//
void showMeasHeader()
{
  nrBytesIn = Serial1.available();
  // Anzahl der über Serial1 eingetroffenen Bytes (Zeichen)

  if(nrBytesIn < 1) return;
  // Beim nächsten Takt wieder in diesen Zustand, wenn kein Zeichen da

  serInChar = Serial1.read();
  // einzelnes Zeichen lesen

  if(serInIdx == 0)             // nach der Area folgt die Slaveadresse
  {
    slaveAdr = serInChar & 0x1F;               // 1 - 31
    serInIdx++;
    // Beim nächsten Takt wieder in diesen Zustand
  }
  else                     // und dann die Anwendungskennung
  {
    appId = (SoaapApId) serInChar;
    measIdx = 0;                      // Index für Messwertunterscheidung
    serInIdx = 0;                     // Index für Messwertaufbau
    nrMeas = soaMsg.measCnt(appId);   // Anzahl Messwerte im Telegramm
    resMeas = soaMsg.measRes(appId);  // Auflösung der Messwerte in Zeichen
    showMeas.enter(showMeasValue);
    // Beim nächsten Takt zum Zustand <showMeasValue>
  }
}

// Warten auf Messwerte und Speichern
//
void showMeasValue()
{
  int i;

  do
  {
    nrBytesIn = Serial1.available();
    // Anzahl der über Serial1 eingetroffenen Bytes (Zeichen)

    if(nrBytesIn < 1) return;
    // Beim nächsten Takt wieder in diesen Zustand, wenn kein Zeichen da

    for(i = 0; i < nrBytesIn; i++)
    {
      tmpBuffer[serInIdx++] = Serial1.read();
      // einzelnes Zeichen lesen

      if(serInIdx == resMeas) break;
      // Alle Zeichen vom Messwert da, also raus
    }

    if(serInIdx < resMeas) return;
    // Wenn noch nicht ale Zeichen vom Messwert erfasst, dann von vorn

    measList[measIdx++] = soaMsg.asc2meas(tmpBuffer);
    // Zeichenkette in Messwert wandeln und speichern

    if(measIdx == nrMeas) break;
    // Falls mehr als ein Telegramm eingetroffen ist
    // muss hier ein Ausstieg erfolgen

    serInIdx = 0;   // Nächste Zeichenfolge
  }
  while (Serial1.available() > 0);
  // Die Taktzeit der Zustandsmaschine ist größer, als die
  // Übertragungszeit von einem Zeichen.
  // Deshalb werden in einem Zustandstakt alle inzwischen eingetroffenen
  // Zeichen bearbeitet.

  if(measIdx < nrMeas) return;
  // Im Zustand bleiben, bis alle Messwerte gewandelt sind

  viewDiv = 0;          // Teiler für Anzeigehäufigkeit
  showMeas.enter(showMeasView);
  // Beim nächsten Takt zum Zustand <showMeasView>
}

// Anzeigen der Messwerte
//
void showMeasView()
{
  if(viewDiv > 0)       // Verzögern der Ausgabe über Zähler
  {                     // Ausgabe wird übersprungen und erst dann vorgenommen
    viewDiv--;          // wenn der Zähler auf 0 steht
    showMeas.enter(showMeasWaitMsg);  // neuen Messwert abwarten
    return;
  }

  if(slaveAdr == 1)
  {
    for(int i = 0; i < 9; i++)
    {
      Serial.print(' ');
      Serial.print(measList[i]);    // Messwert anzeigen
    }
    Serial.println();
  }
  showMeas.enter(showMeasWaitMsg);  // neuen Messwert abwarten
}

#endif
