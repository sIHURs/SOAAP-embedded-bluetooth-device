/*
 * SoaapBleSlave.h
 *
 *  Created on: 26.04.2022
 *      Author: robert
 */

#ifndef SoaapBleSlave_h
#define SoaapBleSlave_h

#define DebugTerminal
// Mit dieser Definition werden die Klasse Monitor und weitere Testmethoden
// eingebunden, womit ein anwendungsorientiertes Debugging möglich ist


#include "LoopCheck.h"
#include "StateMachine.h"

#include "nRF52840Gpio.h"

#include "nRF52840Twi.h"
#include "SensorLSM9DS1.h"

#include "nRF52840Radio.h"
#include "BlePoll.h"

#ifdef DebugTerminal
#include "Monitor.h"
#endif

bool getValues(PlpType plpType, byte *dest);
bool xchgCtrl(PlpType plpType, byte *dest, byte *src, int sSize);

#ifdef DebugTerminal
void smInit() ;
void smCheckJobs() ;
void smDebDword() ;
void smCtrlPolling() ;
void smWaitPolling() ;
void smCheckSens() ;
void smSensReset1() ;
void smSensReset2() ;
void smSensGetValues1() ;
void smSensGetValues2() ;
void smSensGetValues3() ;
void smSensGetValues4() ;
void smSensGetValues5() ;
void smSensGetValues6() ;
void smSensGetValues7() ;
void smSensGetSoaapValues() ;
void smSensDebugValues() ;
void smSensGetErrors() ;
void smSensGetTimeOuts() ;
void smSensGetRunCounts() ;
#endif

#endif // SoaapBleSlave_h
