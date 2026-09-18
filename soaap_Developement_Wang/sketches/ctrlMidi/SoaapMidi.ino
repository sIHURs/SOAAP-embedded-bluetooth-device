#include  "Arduino.h"

#include  "SoaapMidi.h"

#include  "LoopCheck.h"
#include  "StateMachine.h"
#include  "Monitor.h"

#include  "ComRingBuf.h"
#include  "nRF52840Ser.h"
#include  "MidiNotes.h"

#include  "nRF52840Twi.h"
#include  "SensorLSM9DS1.h"

#include  "RowMath.h"


LoopCheck     lc;
#define SmCycle       5
StateMachine  sm(smInit, NULL, SmCycle);
Monitor       mon(modeEcho | modeNl,0,&lc);
SerParams     ttyParams;
nRF52840Ser   tty;

ComRingBuf    crb;
#define       sndBufSize  256
byte          sndBuffer[sndBufSize];

#define MidiCycle     1000
MidiNotes  midi;

nRF52840Twi twi;
#define SensorCycle   500
SensorLSM9DS1 sens((IntrfTw *) &twi, SensorCycle);

#define RmBufSize     128
RowMath       rmAz;
short         rmBuffer[RmBufSize];
FilterResult  fi10, fi100;


void setup()
{
  Serial.begin(115200);

  TwiParams   twiPar;       // Parameter für den I2C-Bus

  ttyParams.inst    = 0;    // Instanzindex der Schnittstelle
  ttyParams.rxdPort = 1;    // Nummer des IO-Port mit RxD-Pin
  ttyParams.rxdPin  = 10;   // Nummer des RxD-Pin am Port
  ttyParams.txdPort = 1;    // Nummer des IO-Port mit TxD-Pin
  ttyParams.txdPin  = 3;    // Nummer des TxD-Pin am Port
  ttyParams.speed   = Baud31250;  // Enumerator für Bitrate
  ttyParams.type    = stCur;      // Stromschleife

  tty.begin(&ttyParams, (IntrfBuf *) &crb);
  tty.startSend();

  crb.setWriteBuffer(sndBufSize, sndBuffer);
  crb.begin((IntrfSerial *) &tty);

  midi.begin(120, nd32, MidiCycle, &crb);

  twiPar.inst     = 0;          // 1. Instanz der I2C
  twiPar.type     = TwiMaster;  // Auswahl Master/slave
  twiPar.clkPort  = 0;          // Takt über Port 0 (P0)
  twiPar.clkPin   = 15;         // Takt an Pin 15 (P0.15)
  twiPar.dataPort = 0;          // Daten über Port 0 (P0)
  twiPar.dataPin  = 14;         // Daten an Pin 14 (P0.14)
  twiPar.speed    = Twi100k;    // Taktfrequenz

  twi.begin(&twiPar); // Initialisierung des I2C-Bus

  sens.begin(FreqAG119, 12, MaxAcc4g, MaxGyro2000dps, FreqM10, 0, MaxMag4G);
  sens.syncValuesAG();

  rmAz.begin(RmBufSize, rmBuffer);
}

byte  TestMsg[32] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};

byte  OnCh0C3p[3] = { 0x90, 60, 100 };
byte  OfCh0C3p[3] = { 0x80, 60, 0 };
int loopCycle = 0;

void loop()
{
  lc.begin();
  // --------------------------------------------------------------------------
  mon.run();

  if(lc.timerMicro(lcTimer0, SensorCycle, 0))
    sens.run();

  if(lc.timerMicro(lcTimer1, MidiCycle, 0, 1000))
  {
    midi.run();
  }

  if(lc.timerMilli(lcTimer2, SmCycle, 0))
  {
    sm.run();
  }

  // --------------------------------------------------------------------------
  lc.end();
}

// ----------------------------------------------------------------------------
//                Z u s t a n d s m a s c h i n e
// ----------------------------------------------------------------------------
//
int   lastNoteIdx = 0;

void smInit()
{
  midi.setNoteType(MidiNotes::nti8);
  lastNoteIdx = midi.addChordNote(MidiNotes::nti8, 60, 100);
  midi.setOpMode(momSequence);
  rmAz.setEdge(0, 10);
  rmAz.setEdge(1, 100);
  sm.enter(smIdle);
}


void smIdle()
{
  //sm.enter(smTestValues);
  sm.enter(smGetValues);
  //mon.printcr("Hier Test Midi");
  //sm.setDelay(500);
  //if(mon.cFlag[0])
  //{
    //sm.enter(smGetValues);
  //}
}


RawDataAG   rawData;

void smGetValues()
{
  if(sm.firstEnter())
  {
    mon.lastKeyIn = ':';
    sens.syncValuesAG();
  }

  if(mon.lastKeyIn == 'c' || mon.lastKeyIn == 'C')
  {
    rmAz.resetInts(0);
    rmAz.resetInts(1);
  }

  if(!sens.getValuesAG(&rawData)) return;

  //rmAz.enterValue(rawData.valueAG.G.x);
  //rmAz.filter(0, &fi10);
  //rmAz.filter(1, &fi100);

  // Test
  if(mon.cFlag[1])
  {
    mon.print(rawData.valueAG.A.x);
    mon.putBuf(' ');
    mon.print(rawData.valueAG.A.y);
    mon.putBuf(' ');
    mon.println(rawData.valueAG.A.z);
    //mon.print(fi10.avg);
    //mon.putBuf('|');
    //mon.print(fi100.avg);
    //mon.putBuf(' ');
    //mon.putBuf(' ');
    //mon.print(fi10.hp);
    //mon.putBuf('|');
    //mon.print(fi100.hp);
    //mon.putBuf(' ');
    //mon.print(fi10.intI);
    //mon.putBuf('|');
    //mon.print(fi100.intI);
    //mon.putBuf(' ');
    //mon.println(fi10.intII);
    //mon.putBuf('|');
    //mon.println(fi100.intII);
  }

  //sm.setDelay(1000);
  sm.enter(smCheckValues);
}


RawDataAG   oldRawData;

void smCheckValues()
{
  bool  newData = false;

  newData |= (rawData.valueAG.A.z != oldRawData.valueAG.A.z);
  newData |= (rawData.valueAG.A.x != oldRawData.valueAG.A.x);

  oldRawData = rawData;
  if(!newData)
    sm.enter(smGetValues);
  else
    sm.enter(smCalcResult);
}


short borderLowAz = 100;
short borderHighAz = 8000;
short borderLowAy = 100;
short borderHighAy = 8300;
short borderLowAx = 100;
short borderHighAx = 4000;

byte  lowNote = 23;
//byte  highNote = 72;
byte highNote = 96;

byte  resultAz;
byte  resultAy;
byte  resultAx;

void smCalcResult()
{
  if((rawData.valueAG.A.z < borderLowAz) && (rawData.valueAG.A.x < borderLowAx))
  {
    sm.enter(smGetValues);
    return;
  }

  short testY = rawData.valueAG.A.z + rawData.valueAG.A.y / 4;
  int result = lowNote + (highNote * testY) / borderHighAz;
  if(result > highNote)
    resultAz = highNote;
  else if (result < lowNote)
    resultAz = lowNote;
  else
    resultAz = result;

  testY = rawData.valueAG.A.x + rawData.valueAG.A.y / 4;
  result = (MidiNotes::nti32 * testY) / borderHighAz;
  if(result < MidiNotes::nti2) result = MidiNotes::nti2;
  if(result > MidiNotes::nti32) result = MidiNotes::nti32;
  resultAx = result;

  sm.enter(smSetResult);
}


void smSetResult()
{
  //mon.print("  ");
  //mon.print(resultAz);
  //mon.print("  ");
  //mon.println(resultAx);
  midi.setChordNote(lastNoteIdx, resultAx, resultAz, 60);
  sm.enter(smGetValues);
}

byte  testValue = 0;

void smTestValues()
{
  midi.setChordNote(lastNoteIdx, MidiNotes::nti4, testValue, 60);
  testValue++;
  if(testValue > 127)
    testValue = 0;
  sm.setDelay(100);
}


