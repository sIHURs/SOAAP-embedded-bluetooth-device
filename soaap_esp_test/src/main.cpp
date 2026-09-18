#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

#include <WString.h>
#include "Arduino.h"

#include <string.h>

#include "MeasMuse.h"
// Eine Klasse zur Verwaltung der im SOAAP-System benötigten UI-Parameter

#include "LoopCheck.h"
// We will use LoopCheck to control the timing of the device independent from
// the resources (timers) of the microcontroller.


#define TestOff   // "ProgrammTest" -> Testprogramm Block Laeuft, um Funktion zu testen oder debugen
                  // "TestOff" 

// ---------------------------------------------------------------------------
// Esp8266 WiFi Mode
// Modus Accesspoint oder Station
// ---------------------------------------------------------------------------

#define EspAccessPoint   // "EspAccessPoint" oder "EspWifiNetwork"

#ifdef EspAccessPoint    // Access Point Modus 
  #define APSSID "ESPap"
  #define APPSK "12345678"
#endif

#ifdef EspWifiNetwork    // Station Modus
  #define APSSID "Vodafone-5C9A"
  #define APPSK "prRrGN6wg7TXbUBd"
#endif

const char *ssid = APSSID;        // die Argumenten "ssid" und "passwort" fuer WiFi.begin(ssid, password) oder WiFi.softAP(ssid, password)
const char *password = APPSK;


typedef struct _Posture2Midi{
  // Konfigurationsdaten
  // ----------------------------

  int         channel;

  float       offsetRoll;
  float       minRoll;
  float       maxRoll;
  float       gapRoll;
  bool        doGapRoll;
  Meas2Midi   aimRoll;

  float       offsetPitch;
  float       minPitch;
  float       maxPitch;
  float       gapPitch;
  bool        doGapPitch;
  Meas2Midi   aimPitch;

  float       offsetYaw;
  float       maxYaw;
  float       minYaw;
  float       gapYaw;
  bool        doGapYaw;
  Meas2Midi   aimYaw;

  int         midiOpMode;
  MidiArea    midiAreaNoteType;
  MidiArea    midiAreaNoteVal;
  MidiArea    midiAreaNoteVel;

  MeasMap     mapRoll;
  MeasMap     mapPitch;
  MeasMap     mapYaw;

} Posture2Midi, *Posture2MidiPtr;

// ---------------------------------------------------------------------------
// INSTANCES and VARIABLES
// Class instances and variables defined outside functions to use them in
// any function
// ---------------------------------------------------------------------------
//

lcDateTime  dt;             // Time-Structure of LoopCheck

WiFiServer server(4210);    //Tcp Server Instanz herstellen

uint8_t  testStrTelegramm[] = "SOAAP_V_2;8;9;6;2;0;1;2;1;1;0;1;5.6;5.7;5.8;6.5;6.6;6.7;7.4;7.5;7.6;FF;7F;44;7A;23;79::";
char testStrToByte[] = "FF";
byte testByte;
int SerialComingByte;

// -------------------------------------------------------------------------
LoopCheck lc;
// -------------------------------------------------------------------------
// Eine statische Instanz der Klasse LoopCheck
// Darüber wird das zeitverhalten gesteurt (Software-Timer) und geprüft


// ---------------------------------------------------------------------------
// FUNKTIONEN
// ---------------------------------------------------------------------------
//

byte stringToByte(char *src);                           // Sting in Byte konvertieren

int  parseMsg(uint8_t * msg, unsigned int msgLen);         // "msg" -> Telegramm im Strig-Format
                                                        // "msgLen" -> Laenge des Telegramms
                                                        
int  storeDataMsg(uint8_t * msg, unsigned int msgLen);     // "msg" -> Telegramm im Strig-Format
                                                        // "msgLen" -> Laenge des Telegramms

void getValue(Posture2MidiPtr exampleParameter);         

void printIdx();                        // Debugsfunktion fuer parseMsg
void printArray();                      // Debugsfunktion fuer storeDataMsg
void printDatensatz(int chn);           // Debugsfunktion fuer getValue


void setup() {
  Serial.begin(115200);

// --------------------------------------------------------------------------  
// hier kann das Test Progrann laufen  
#ifdef ProgrammTest      

// test Parserverarbeitung         
  int parseReady, storeReady;
  Serial.println();
  Serial.print("MsgLen:");
  Serial.println(sizeof(testStrTelegramm));
  Serial.println();
  parseReady = parseMsg(testStrTelegramm, sizeof(testStrTelegramm));
  printIdx();
  Serial.println();

  storeReady = storeDataMsg(testStrTelegramm, sizeof(testStrTelegramm));
  Serial.print("storeReady = ");
  Serial.println(storeReady);
  printArray();

  printDatensatz(2);
  Posture2MidiPtr ParameterValuePtr;
  getValue(ParameterValuePtr);
  printDatensatz(2);


// test sringToByte Funktion

//   Serial.println();
//   testByte = stringToByte(testStrToByte);
//   Serial.println(testByte);

#endif

// --------------------------------------------------------------------------  

#ifdef TestOff

  Serial1.begin(115200);
  //pinMode(LED_BUILTIN, OUTPUT);
  Serial.println();

#ifdef EspAccessPoint
  Serial.print("Configuring access point...");
  
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.mode(WIFI_AP);
  boolean result = WiFi.softAP(ssid, password);
  if(result == true)
  {
    Serial.println("Ready");
  }
  else
  {
    Serial.println("Failed!");
  }

  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
#endif

#ifdef EspWifiNetwork
  Serial.print("Configuring WiFi Connecting...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    // delay(500);
    // digitalWrite(LED_BUILTIN, HIGH);
    // delay(500);
    // digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Connecting..");
  }
  // digitalWrite(LED_BUILTIN, HIGH);

  Serial.print("Connected to WiFi. IP:");
  Serial.println(WiFi.localIP());
#endif


  // TCP Server start
  server.begin();

#endif
}

void loop() {

#ifdef ProgrammTest
// TestProgramm hier schreiben

#endif

#ifdef TestOff
  
  lc.begin();       // Muss am Anfang von LOOP aufgerufen werden
  // -------------------------------------------------------------------------

  WiFiClient client = server.available();

#define RecAndParserCycleTime 200
  if(lc.timerMicro(lcTimer0, RecAndParserCycleTime, 0, 450)){
    if(client){
      //Serial.println("Client connected");
      uint8_t readBuf[256];
      while(client.connected()){
        int inComingBytes = client.available();
        if(inComingBytes && inComingBytes < 256){

          Serial.println(sizeof(inComingBytes));
          client.read(readBuf, inComingBytes);        // Lesen 
          Serial.write(readBuf, inComingBytes);       // DEBUG empfangenes Telegramm ueberpruefen 
          
          int parseReady, storeReady;

          parseReady = parseMsg(readBuf, inComingBytes);
          printIdx();
          Serial.println();
          
          storeReady = storeDataMsg(readBuf, inComingBytes);
          Serial.print("storeReady = ");
          Serial.println(storeReady);
          printArray();

          Posture2MidiPtr ParameterValuePtr;
          getValue(ParameterValuePtr);

          for(int i=0; i<17; i++){
            printDatensatz(i);
          }
        
          //Serial1.write(readBuf, inComingBytes);      // UART Serial Schnittstelle 1 output von esp zum Arduino nano (beim Test zum Arudino Due)
        }

      }
      client.stop();
      //Serial.println("client disconnected");
    }


  }


// #define SendMsgCycleTime 450
//   if(lc.timerMicro(lcTimer1, SendMsgCycleTime, 0, 1000)){
//       if(Serial.available()){
//       SerialComingByte = Serial.read();
//       printDatensatz(SerialComingByte);
//     }    
//   }


  // --------------------------------------------------------------------------
  lc.end();       // Unbedingt an das Ende der Arduino-LOOP

#endif
}





// *************************************************************************
// PARSER VERARNEITUNG
// *************************************************************************

#define NrOfChannelsMM 16

typedef enum PARSER_MSG_STATE{
  p_msg_st_unknown = 0,
  p_msg_st_waitFieldSeparator,
  //p_msg_st_FieldPduCount,
  p_msg_st_FieldNumInt,
  p_msg_st_FieldNumFloat,
  p_msg_st_FieldNumByte,
  P_msg_st_FieldValue,
  p_msg_st_Ready
};

// -------------------------------------------------------------------------
// Verarbeitung des eingegangenen Telegramms
// -------------------------------------------------------------------------
//

// Parser Verfahren Counter

//int idxFieldPduCount;
int idxFieldIntCount;
int idxFieldFloatCount;
int idxFieldByteCount;
int idxFieldValue;

int recParseCounter;    // Zähler des Receive Parse Prozess

// -------------------------------------------------------------------------
// Funktionen
// -------------------------------------------------------------------------
//

// -> SOAAP_V_2 ;     8;9;6    ;  0  ;  0;0;0  ; 0;0;0 ;    0   ; 0.0;0.0;0.0 ; 0.0;0.0;0.0 ; 0.0;0.0;0.0 ; 00;00;00;00;00;00 ::0
// -> Header    ;  ValueHeader ; chn ;   aim   ;mapping; opmode ;   RollArea  ;  PitchArea  ;   YawArea   ;       MidiArea    :: Ende


int parseMsg(uint8_t * msg, unsigned int msgLen){           // Dekodierung der eingehenden Telegramme
  unsigned int cntField;
  unsigned int idx;
  PARSER_MSG_STATE parserState;
  bool parserReady;
  bool parserError;

  recParseCounter++;

  //---------------------------------------------------------------------------
  // Parser initialisieren
  //---------------------------------------------------------------------------
  // Ergebnisdaten zuruecksetzen

  //int idxFieldPduCount    = 0;
  idxFieldIntCount    = 0;
  idxFieldFloatCount  = 0;
  idxFieldByteCount   = 0;
  idxFieldValue       = 0;
  
  // Zustandsmaschine zuruecksetzen
  cntField    = 0;
  parserState = p_msg_st_waitFieldSeparator;
  parserReady = false;
  parserError = false;

  //---------------------------------------------------------------------------
  // Telegramm parsen
  //---------------------------------------------------------------------------

  for(idx = 0; idx < msgLen; idx++){
    //-------------------------------------------------------------------------
    // Je nach Zustand verzweigen
    //-------------------------------------------------------------------------
    switch (parserState)
    {
      // ------------------------------------------------------------------- //
      case p_msg_st_waitFieldSeparator:
      // ------------------------------------------------------------------- //
        if(msg[idx] == ';'){

          cntField++;

          if(cntField == 1){
            // Naechsten Zustand setzen
            parserState = p_msg_st_FieldNumInt;
            break;
          }else if(cntField == 2){
            // Naechsten Zustand setzen
            parserState = p_msg_st_FieldNumFloat;
            break;
          }else if(cntField == 3){
            // Naechsten Zustand setzen
            parserState = p_msg_st_FieldNumByte;
            break;
          }else if(cntField == 4){
            // Naechsten Zustand setzen
            parserState = P_msg_st_FieldValue;
            break;
          }
          // Sonst im Zustand bleiben
        }

        break;

      // ------------------------------------------------------------------- //
      case p_msg_st_FieldNumInt:
      // ------------------------------------------------------------------- //
        idxFieldIntCount = idx;

        // Naechsten Zustand setzen
        parserState = p_msg_st_waitFieldSeparator;

        break;

      // ------------------------------------------------------------------- //
      case p_msg_st_FieldNumFloat:
      // ------------------------------------------------------------------- //
        idxFieldFloatCount = idx;

        // Naechsten Zustand setzen
        parserState = p_msg_st_waitFieldSeparator;

        break;

      // ------------------------------------------------------------------- //
      case p_msg_st_FieldNumByte:
      // ------------------------------------------------------------------- //
        idxFieldByteCount = idx;

        // Naechsten Zustand setzen
        parserState = p_msg_st_waitFieldSeparator;

        break;

      // ------------------------------------------------------------------- //
      case P_msg_st_FieldValue:
      // ------------------------------------------------------------------- //
        idxFieldValue = idx;

        // Merker 'Parser ist fertig' setzen
        parserReady = true;

        // Naechsten Zustand setzen
        parserState = p_msg_st_Ready;

        break;

      // ------------------------------------------------------------------- //
      case p_msg_st_Ready:
      // ------------------------------------------------------------------- //
        // Im  Zustand bleiben

        break;
    
      // ------------------------------------------------------------------- //
      default:
      // ------------------------------------------------------------------- //
        parserReady = true;  

        break;
    }

    //-------------------------------------------------------------------------
    // Eventuell das Parsen beenden
    //-------------------------------------------------------------------------
    if(parserReady == true)
    {
      break;
    }

    if(parserError == true)
    {
      break;
    }
  }

  //---------------------------------------------------------------------------
  // Ergebnis definieren
  //---------------------------------------------------------------------------
  if(parserReady != true)
  {
    return(0);
  }

  return(1);
}


#define MAXVALCHRLEN  128
#define BYTEVAL_LEN_MAX 256

int intCount;
int floatCount;
int byteCount;

int     intArray[8];
double  floatArray[9];
byte    byteArray[6];

int storeDataMsg(uint8_t * msg, unsigned int msgLen){

  int locIntCount;
  int locFloatCount;
  int locByteCount;

  int     intValue;
  double  floatValue;
  byte    byteValue;

  unsigned int idxValue;
  unsigned int idxValueStr;
  unsigned int idxValueChar;

  char         valueBuf[MAXVALCHRLEN];
  unsigned int idxBuf;
  char         chr;

  bool         storeReady;

  //---------------------------------------------------------------------------
  // Parser Ergebnis ueberpruefen
  //---------------------------------------------------------------------------

  if(idxFieldIntCount < 1 || idxFieldIntCount >= msgLen)
  {
    return(-1);
  }

  if(idxFieldFloatCount < 1 || idxFieldFloatCount >= msgLen)
  {
    return(-2);
  }

  if(idxFieldByteCount < 1 || idxFieldByteCount >= msgLen)
  {
    return(-3);
  }

  if(idxFieldValue < 1 || idxFieldValue >= msgLen)
  {
    return(-4);
  }

  //---------------------------------------------------------------------------
  // Einzelne Datenelemente speichern
  //---------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // intCount, floatCount, byteCount erstmal lokal sperichern
    // Erst mal werden alle Werte gespeichert,
    // dann werden Zaehler uebernommen
    //-------------------------------------------------------------------------

  intCount    = locIntCount   = atoi((char *)&msg[idxFieldIntCount]);
  floatCount  = locFloatCount = atoi((char *)&msg[idxFieldFloatCount]);
  byteCount   = locByteCount  = atoi((char *)&msg[idxFieldByteCount]);

    //-------------------------------------------------------------------------
    // Werte intArray, floatArray, byteArray
    //-------------------------------------------------------------------------
      //-----------------------------------------------------------------------
      // Speichern initialisieren
      //-----------------------------------------------------------------------
  // Index fuer die Position von der Wert-Strings setzen
  idxValueStr = idxFieldValue;

      //-----------------------------------------------------------------------
      // intArray
      //-----------------------------------------------------------------------

  if(locIntCount != 0){
    idxValue = 0;
    idxBuf = 0;

    for(idxValueChar = idxValueStr; idxValueChar < msgLen; idxValueChar++){
      //-----------------------------------------------------------------------
      // Einzelne Zeichen auswerten
      //-----------------------------------------------------------------------
      chr = msg[idxValueChar];

      if(chr != ';' && chr != ':'){
        if(idxBuf <= MAXVALCHRLEN - 2){
          valueBuf[idxBuf] = chr;
          idxBuf++;
        }
      }else{
        // String abschliessen
        valueBuf[idxBuf] = 0;

        // Wert ASCII->INT konvertieren
        intValue = atoi(valueBuf);

        // Wert speichern
        if(idxValue < 8){
          intArray[idxValue] = intValue;
          idxValue++;
        }

        // Das Speichern weiterer Werte initialisieren
        idxBuf = 0;

        locIntCount--;
        if(locIntCount == 0)
          break;

      }
    }

    // idxValueChar zeigt jetzt auf;
    idxValueStr = idxValueChar + 1;   // idxValueStr auf naechstes Wertfeld
  }

      //-----------------------------------------------------------------------
      // intArray
      //-----------------------------------------------------------------------

  if(locFloatCount != 0){
    idxValue = 0;
    idxBuf = 0;

    for(idxValueChar = idxValueStr; idxValueChar < msgLen; idxValueChar++){
      //-----------------------------------------------------------------------
      // Zeichen auswerten
      //-----------------------------------------------------------------------
      chr = msg[idxValueChar];

      if(chr != ';' && chr != ':'){
        if(idxBuf <= MAXVALCHRLEN - 2){
          valueBuf[idxBuf] = chr;
          idxBuf++;
        }
      }else{
        //String abschliessen
        valueBuf[idxBuf] = 0;

        // Wert ASCII-> FLOAT konvertieren
        floatValue = atof(valueBuf);

        // Wert speichern
        if(idxValue < 9){
          floatArray[idxValue] = floatValue;
          idxValue++;
        }

        // Das Speichern weitere Werte initialisieren
        idxBuf = 0;

        locFloatCount--;
        if(locFloatCount == 0)
          break;
      }
    }

    // idxValueChar zeigt jetzt auf;
    idxValueStr = idxValueChar + 1;   // idxValueStr auf naechstes Wertfeld
  }

      //-----------------------------------------------------------------------
      // byteArray
      //-----------------------------------------------------------------------
  if(locByteCount != 0){
    idxValue = 0;
    idxBuf = 0;

    for(idxValueChar = idxValueStr; idxValueChar < msgLen; idxValueChar++){
      //-----------------------------------------------------------------------
      // Je nach Zeichen verzweigen
      //-----------------------------------------------------------------------
      chr = msg[idxValueChar];

      if(chr != ';' && chr != ':'){
        if(idxBuf < MAXVALCHRLEN - 2){
          valueBuf[idxBuf] = chr;
          idxBuf++;

          idxValueChar++;
          chr = msg[idxValueChar];
          valueBuf[idxBuf] = chr;
          idxBuf++;
        }
      }else{
        //String abschliessen
        valueBuf[idxBuf] = 0;

        //debug
        Serial.println(valueBuf);
        
        // Wert zum Byte konvertieren
        byteValue = stringToByte(valueBuf);

        // Wert speichern
        if(idxValue < 6){
          byteArray[idxValue] = byteValue;
          idxValue++;
        }

        // Das Speichern weitere Werte initialisieren
        idxBuf = 0;

        locByteCount--;
        if(locByteCount == 0)
          break;
      }
    }
  }

  return 1;
}


//-------------------------------------------------------------------------
// Datensatz speichern
//-------------------------------------------------------------------------


Posture2Midi ParameterValue[NrOfChannelsMM];


void getValue(Posture2MidiPtr exampleParameter){

  // --- intArray ---

  int chn;
  chn = intArray[0];
  if(chn < 0) return;
  if(chn > NrOfChannelsMM) return;
  exampleParameter = &ParameterValue[chn];
  //Serial.println("pointer create");

  //channel
  exampleParameter->channel = chn;
  //Serial.println("getchannel");

  //aimRoll
  if(intArray[1] == 0){
    exampleParameter->aimRoll = Nothing;
  }else if(intArray[1] == 1){
    exampleParameter->aimRoll = NoteType; 
  }else if(intArray[1] == 2){
    exampleParameter->aimRoll = NoteVal; 
  }else if(intArray[1] == 3){
    exampleParameter->aimRoll = NoteVel; 
  }else return;
  
  //aimPitch
  if(intArray[2] == 0){
    exampleParameter->aimPitch = Nothing;
  }else if(intArray[2] == 1){
    exampleParameter->aimPitch = NoteType; 
  }else if(intArray[2] == 2){
    exampleParameter->aimPitch = NoteVal; 
  }else if(intArray[2] == 3){
    exampleParameter->aimPitch = NoteVel; 
  }else return;

  //aimYaw
  if(intArray[3] == 0){
    exampleParameter->aimYaw = Nothing;
  }else if(intArray[3] == 1){
    exampleParameter->aimYaw = NoteType; 
  }else if(intArray[3] == 2){
    exampleParameter->aimYaw = NoteVal; 
  }else if(intArray[3] == 3){
    exampleParameter->aimYaw = NoteVel; 
  }else return;
  //Serial.println("get aim");

  //Mapping
  if(intArray[4] == 0){
    exampleParameter->mapRoll = OneToOne;
  }else if(intArray[4] == 1){
    exampleParameter->mapRoll = BiLinear;
  }else return;

  if(intArray[5] == 0){
    exampleParameter->mapPitch = OneToOne;
  }else if(intArray[5] == 1){
    exampleParameter->mapPitch = BiLinear;
  }else return;

  if(intArray[6] == 0){
    exampleParameter->mapYaw = OneToOne;
  }else if(intArray[6] == 1){
    exampleParameter->mapYaw = BiLinear;
  }else return;
  //Serial.println("get map");


  //OpMode
  if(intArray[7] == 0){
    exampleParameter->midiOpMode = momIdle;
  }else if(intArray[7] == 1){
    exampleParameter->midiOpMode = momSequence;
  }else if(intArray[7] == 2){
    exampleParameter->midiOpMode = momRunDelta;
  }else return;
  //Serial.println("get OpMode");


  // --- floatArray ---

  exampleParameter->offsetRoll = floatArray[0];
  exampleParameter->minRoll = floatArray[1];
  exampleParameter->maxRoll = floatArray[2];

  exampleParameter->offsetRoll = floatArray[3];
  exampleParameter->minRoll = floatArray[4];
  exampleParameter->maxRoll = floatArray[5];

  exampleParameter->offsetRoll = floatArray[6];
  exampleParameter->minRoll = floatArray[7];
  exampleParameter->maxRoll = floatArray[8];
  //Serial.println("get WinkelArea");

  // --- byteArray ---

  exampleParameter->midiAreaNoteType.high = byteArray[0];
  exampleParameter->midiAreaNoteType.low = byteArray[1];

  exampleParameter->midiAreaNoteVal.high = byteArray[2];
  exampleParameter->midiAreaNoteVal.low = byteArray[3];

  exampleParameter->midiAreaNoteVel.high = byteArray[4];
  exampleParameter->midiAreaNoteVel.low = byteArray[5];
  //Serial.println("get midiArea");

}


//-------------------------------------------------------------------------
// Debug Funktionen
//-------------------------------------------------------------------------

void printIdx(){
  Serial.println(idxFieldIntCount);
  Serial.println(idxFieldFloatCount);
  Serial.println(idxFieldByteCount);
  Serial.println(idxFieldValue);
}

void printArray(){
  int i;

  for(i = 0; i < 8; i++){
    Serial.print(intArray[i]);
    Serial.print(";");
  }

  for(i = 0; i < 9; i++){
    Serial.print(floatArray[i]);
    Serial.print(";");
  }

  for(i = 0; i < 6; i++){
    Serial.print(byteArray[i]);
    Serial.print(";");
  }

  Serial.println();
}


void printDatensatz(int chn){

  Serial.print("Channel : ");  
  Serial.println(ParameterValue[chn].channel);
  
  Serial.print("aimRoll | aimPitch | aimYaw : ");
  Serial.print(ParameterValue[chn].aimRoll); Serial.print(" | ");
  Serial.print(ParameterValue[chn].aimPitch); Serial.print(" | ");
  Serial.println(ParameterValue[chn].aimYaw);

  Serial.print("mapRoll | mapPitch | mapYaw " );
  Serial.print(ParameterValue[chn].mapRoll); Serial.print(" | ");
  Serial.print(ParameterValue[chn].mapPitch); Serial.print(" | ");
  Serial.println(ParameterValue[chn].mapYaw);

  Serial.print("OpMode : ");
  Serial.println(ParameterValue[chn].midiOpMode);

  Serial.print("offsetRoll | minRoll | maxRoll | offsetPitch | minPitch | maxPitch | offsetYaw | minYaw | maxYaw : " );
  Serial.print(ParameterValue[chn].offsetRoll); Serial.print(" | ");
  Serial.print(ParameterValue[chn].minRoll); Serial.print(" | ");
  Serial.print(ParameterValue[chn].maxRoll); Serial.print(" | ");
  Serial.print(ParameterValue[chn].offsetPitch); Serial.print(" | ");
  Serial.print(ParameterValue[chn].minPitch); Serial.print(" | ");
  Serial.print(ParameterValue[chn].maxPitch); Serial.print(" | ");
  Serial.print(ParameterValue[chn].offsetYaw); Serial.print(" | ");
  Serial.print(ParameterValue[chn].minYaw); Serial.print(" | ");
  Serial.println(ParameterValue[chn].maxYaw);

  Serial.print("midiLowNoteType | midiHighNoteType | midiLowNoteVal | midiLowNoteVal | midiHighNoteVel | midiLowNoteVel : ");
  Serial.print(ParameterValue[chn].midiAreaNoteType.high); Serial.print(" | ");
  Serial.print(ParameterValue[chn].midiAreaNoteType.low); Serial.print(" | ");
  Serial.print(ParameterValue[chn].midiAreaNoteVal.high); Serial.print(" | ");
  Serial.print(ParameterValue[chn].midiAreaNoteVal.low); Serial.print(" | ");
  Serial.print(ParameterValue[chn].midiAreaNoteVel.high); Serial.print(" | ");
  Serial.println(ParameterValue[chn].midiAreaNoteVel.low);
  
}

// -------------------------------------------------------------------------
// Hilfsfunktionen
// -------------------------------------------------------------------------
//

byte stringToByte(char *src){
  uint32_t number;
  byte NumberByte;

  number = strtol(src, NULL, 16);
  NumberByte = number & 0XFF;
  
  return NumberByte;
}