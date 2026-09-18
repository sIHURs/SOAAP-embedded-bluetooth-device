//-----------------------------------------------------------------------------
// Thema:   Social Manufacturing Network / Process Measurements
// Datei:   ProcMeas.h
// Editor:  Robert Patzke
// URI/URL: www.mfp-portal.de
//-----------------------------------------------------------------------------
// Lizenz:  CC-BY-SA  (siehe Wikipedia: Creative Commons)
//
#ifndef _ProcMeas_h
#define _ProcMeas_h
//-----------------------------------------------------------------------------

#include <stddef.h>
#include <string.h>
#include "arduinoDefs.h"
#include "IntrfMeas.h"
#include "EulerAngles.h"

#define smnRAD_TO_DEG 57.295779513082320876798154814105 // Um die Fehler von compilieren vorübergehend auszuschalten

//#define ProcMeasDebug

typedef enum _PmState
{
  pmInit,
  pmWait,
  pmCalc
} PmState;

typedef struct _Posture
{
  float roll;
  float pitch;
  float yaw;
}Posture, *PosturePtr;

// ---------------------------------------------------------------------------
// class ProcMeas
// ---------------------------------------------------------------------------
//
class ProcMeas
{
  // -------------------------------------------------------------------------
  // Klassenspezifische Datentypen
  // -------------------------------------------------------------------------
  //
#ifdef  ProcMeasDebug
  typedef struct _Statistics
  {
    dword pmInitCnt;
    dword pmWaitCnt;
    dword pmCalcCnt;
  } Statistics, *StatisticsPtr;
#endif

private:
  // -------------------------------------------------------------------------
  // Lokale Variablen
  // -------------------------------------------------------------------------
  //
  bool        gravAnglesAvail;
  IntrfMeas   *pMeas;
  TriFloat    gravAngles;
  byte        gravSigns;
  PmState     pmState;
  EulerAngles euler;
  Posture     posture;

private:
  // -------------------------------------------------------------------------
  // Lokale Funktionen
  // -------------------------------------------------------------------------
  //

public:
  // -------------------------------------------------------------------------
  // Konstruktoren und Initialisierungen
  // -------------------------------------------------------------------------
  //
  ProcMeas(IntrfMeas *measPtr);

  // -------------------------------------------------------------------------
  // Anwenderfunktionen
  // -------------------------------------------------------------------------
  //
  void  run();
  bool  availAngles(bool reset);
  float getRollValue();
  float getPitchValue();
  float getYawValue();
  byte  getGravSigns();

  // -------------------------------------------------------------------------
  // Debug-Funtionen
  // -------------------------------------------------------------------------
  //
#ifdef  ProcMeasDebug
  Statistics  statistics;

#endif



};

//-----------------------------------------------------------------------------
#endif  // _ProcMeas_h



