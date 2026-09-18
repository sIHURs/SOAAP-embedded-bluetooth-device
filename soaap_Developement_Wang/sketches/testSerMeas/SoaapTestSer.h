//-----------------------------------------------------------------------------
//                                S O A A P
//-----------------------------------------------------------------------------
// Thema:   Steuerung optischer und akust. Ausgaben für Performance-Künstler
//          Testen der Software für Arduino Due (Vermittler/Konfigurator)
// Datei:   SoaapTestSer.h
// Editor:  Robert Patzke
// URI/URL: www.hs-hannover.de
//-----------------------------------------------------------------------------
// Lizenz:  CC-BY-SA  (siehe Wikipedia: Creative Commons)
//

#ifndef SoaapTestSer_h
#define SoaapTestSer_h

// ----------------------------------------------------------------------------
// Definieren der aktuellen Testumgebung
// ----------------------------------------------------------------------------

//#define TEST001
// Durchreichen der Mastertelegramme von Serial1 an Serial (Serial0)

#define TEST002
// Ermitteln der Messwerte und Anzeige (dezimal) in einer Zustandsmaschine


#if defined TEST002

#define rbSize  128

// Vorwärtsreferenzen für die Zusztandsmaschine
void showMeasInit();
void showMeasWaitMsg();
void showMeasHeader();
void showMeasValue();
void showMeasView();

#endif

#endif // SoaapTestSer_h
