/*
ESPboy IR blaster module
for www.ESPboy.com project by RomanS
https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun
*/

//Write one char commands. To "Read", type 'r'
//To enter the command - long press A button

#include <Adafruit_MCP23017.h>
#include <TFT_eSPI.h>

#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <IRsend.h>
#include <IRrecv.h>
#include "LittleFS.h"

#include <ESP8266WiFi.h>
#include "lib/ESPboyLogo.h"
#include "ESPboyGUI.h"


#define PAD_LEFT        0x01 //L
#define PAD_UP          0x02 //U
#define PAD_DOWN        0x04 //D
#define PAD_RIGHT       0x08 //R
#define PAD_ACT         0x10 //A
#define PAD_ESC         0x20 //E
#define PAD_LFT         0x40 //F
#define PAD_RGT         0x80 //G
#define PAD_ANY         0xff //Y

#define MCP23017address 0  
#define LEDPIN D4
#define SOUNDPIN D3
#define CSTFTPIN 8  // CS MCP23017 PIN to TFT

#define kRECIEVE_PIN D6
#define kSEND_PIN D8
#define kCAPT_BUF 1024
#define kTIMEOUT 50
#define kTIMEOUT_READING 8000
#define kFREQ 38000 

Adafruit_MCP23017 mcp;
TFT_eSPI tft;
ESPboyGUI* GUIobj = NULL;


IRsend irsend(kSEND_PIN);
IRrecv irrecv(kRECIEVE_PIN, kCAPT_BUF, kTIMEOUT, false);
decode_results resultsGlobal;

enum commands{
 IrREAD=1,
 IrASSIGN,
 IrCATALOGUE,
 IrDELETE,
 IrMODE,
 IrSAVE
}cmd;

uint8_t commandMatrice[][2] = {
  {'r', IrREAD},
  {'a', IrASSIGN},
  {'c', IrCATALOGUE},
  {'d', IrDELETE},
  {'o', IrMODE},
  {'s', IrSAVE},
  {0,0}
};


struct recordIR{
  String nameIR;
  uint8_t assignedButton = 0;
  decode_results resultsIR;
};


std::vector<recordIR> recordIRvector;

const char* PROGMEM printComList1="1-99 send   Onetouch";
const char* PROGMEM printComList2="Read Assign Del Save";

const char* PROGMEM printCommand="Command?";
const char* PROGMEM printSave="Saved";
const char* PROGMEM printOk="OK";
const char* PROGMEM printFail="FAIL";
const char* PROGMEM printTimeout="Time out";
const char* PROGMEM printWrongCommand="Wrong command";
const char* PROGMEM printNoRecords="No records";
const char* PROGMEM printWrongRecord="Wrong record No";
const char* PROGMEM printAssignNotFound="Assign not found";
const char* PROGMEM printUnknownFormat="Unknown IR format";

const char* PROGMEM printLoadData="Loading data...";
const char* PROGMEM printSaveData="Saving data...";
const char* PROGMEM printFileNotFound="Data file not found";

const char* PROGMEM printSending="Sending #";

const char* PROGMEM printCancelOneTouch="LGT+LFT to exit";
const char* PROGMEM printOneTouchMode="One-touch mode";

const char* PROGMEM printRead="Reading IR...";
const char* PROGMEM printName="Name?";

const char* PROGMEM printAssignNo="Assign No?";
const char* PROGMEM printPressAssignButton="Press assign button";
const char* PROGMEM printAssigned="Assigned #";

const char* PROGMEM printDeleteNo="Delete No?";
const char* PROGMEM printDeleted="Deleted #";



uint8_t getCommand(char cmd){
 if (cmd>=65 && cmd<=90) cmd+=32;
 for(uint8_t i=0;; i++){
   if (cmd == commandMatrice[i][0]) return(commandMatrice[i][1]);
   if (commandMatrice[i][0] == 0) return (0);
 }  
}


String capitaliseString(String str){
  for(uint8_t i = 0; i< str.length(); i++)
    if(str[i] >= 'a' && str[i] <= 'f') str.setCharAt(i,(char)((uint8_t)str[i]-32));
  return (str);
}


void setup() {
  
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  delay(100);


  // MCP23017 and buttons init, should preceed the TFT init
  mcp.begin(MCP23017address);
  delay(100);
  mcp.pinMode(CSTFTPIN, OUTPUT);
  for (int i = 0; i < 8; ++i) {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);
  }

  // Sound init and test
  pinMode(SOUNDPIN, OUTPUT);
  tone(SOUNDPIN, 200, 100);
  delay(100);
  tone(SOUNDPIN, 100, 100);
  delay(100);
  noTone(SOUNDPIN);

  // TFT init
  mcp.digitalWrite(CSTFTPIN, LOW);
  tft.begin();
  delay(100);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  // draw ESPboylogo
  tft.drawXBitmap(30, 20, ESPboyLogo, 68, 64, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(F("IR blaster"), 35, 95);

  delay(1000);

//GUI obj init
  GUIobj=new ESPboyGUI(&tft, &mcp);

//IR init
  irrecv.enableIRIn();  // Start up the IR receiver.
  irsend.begin();       // Start up the IR sender.

// clear screen
  tft.fillScreen(TFT_BLACK);

//Load data
  IrLOADfunction();
  
}


void IrDELETEfunction(){
 int8_t numbCom = 0;
 String rdCommand = "";
  GUIobj->printConsole(printDeleteNo, TFT_MAGENTA, 0, 0); 
  while (rdCommand == "") rdCommand = GUIobj->getUserInput();
  numbCom = rdCommand.toInt();
  if (numbCom && numbCom <= recordIRvector.size()){
    recordIRvector.erase(recordIRvector.begin()+(numbCom-1));
    GUIobj->printConsole(printDeleted+(String)numbCom, TFT_GREEN, 0, 0); 
    }
  else 
    GUIobj->printConsole(printWrongRecord, TFT_RED, 0, 0); 
  };


bool space2(char l, char r) {
    return l == r && std::isspace(l);
}

void IrREADfunction(){
  String rdName = "";
  long tme = millis();
  GUIobj->printConsole(printRead, TFT_MAGENTA, 0, 0); 
  
  irrecv.resume();
  while (!irrecv.decode(&resultsGlobal) && (millis()-tme)<kTIMEOUT_READING) delay(10);
  if ((millis()-tme)>=kTIMEOUT_READING)
    GUIobj->printConsole(printTimeout, TFT_RED, 0, 0); 
  else{
    GUIobj->printConsole(printOk, TFT_GREEN, 0, 0);
    String decodeInfo = resultToHumanReadableBasic(&resultsGlobal);
    for(uint8_t i = 0; i < decodeInfo.length()-1; i++)
      if((decodeInfo[i]<=32 && decodeInfo[i+1]<=32)) decodeInfo.remove(i,1);
    GUIobj->printConsole(decodeInfo, TFT_YELLOW, 1, 0);

    if (recordIRvector.back().resultsIR.decode_type != decode_type_t::UNKNOWN){
      GUIobj->printConsole(printName, TFT_MAGENTA, 0, 0); 
      while (rdName == "") rdName = GUIobj->getUserInput();
      if (rdName.length() > 12) rdName = rdName.substring(0, 12);
      GUIobj->printConsole(rdName, TFT_YELLOW, 0, 0); 
      recordIRvector.push_back(recordIR());
      recordIRvector.back().nameIR = rdName;
      recordIRvector.back().resultsIR = resultsGlobal;
      recordIRvector.back().assignedButton = 0;
    }
  }
}


void IrSENDNOfunction(uint8_t numbCom){
 uint8_t okflag=0;
  GUIobj->printConsole(printSending+recordIRvector[numbCom].nameIR, TFT_MAGENTA, 0, 0); 
  if (recordIRvector[numbCom].resultsIR.decode_type == decode_type_t::UNKNOWN) { 
    GUIobj->printConsole(printUnknownFormat, TFT_RED, 0, 0); 
    okflag = 0;} 
  else{
    if (hasACState(recordIRvector[numbCom].resultsIR.decode_type)) { 
      okflag = irsend.send(recordIRvector[numbCom].resultsIR.decode_type, recordIRvector[numbCom].resultsIR.state, recordIRvector[numbCom].resultsIR.bits / 8);} 
    else {
      okflag = irsend.send(recordIRvector[numbCom].resultsIR.decode_type, recordIRvector[numbCom].resultsIR.value, recordIRvector[numbCom].resultsIR.bits);}
  }
  if(okflag) GUIobj->printConsole(printOk, TFT_GREEN, 0, 0);
  else GUIobj->printConsole(printFail, TFT_RED, 0, 0); 
}



void IrASSIGNfunction(){
 int8_t numbCom = 0;
 uint8_t readKeys;
 String rdCommand = "";
  if (!recordIRvector.size())
    GUIobj->printConsole(printNoRecords, TFT_RED, 0, 0);
  else{
    GUIobj->printConsole(printAssignNo, TFT_MAGENTA, 0, 0);
    while (rdCommand == "") rdCommand = GUIobj->getUserInput();
    numbCom = rdCommand.toInt();
    if(!numbCom || numbCom>recordIRvector.size())
      GUIobj->printConsole(printWrongRecord, TFT_RED, 0, 0);
    else{
      GUIobj->printConsole(printPressAssignButton, TFT_MAGENTA, 0, 0);
      while(!(readKeys=GUIobj->getKeys()))delay(10);
      recordIRvector[numbCom-1].assignedButton = readKeys;
      GUIobj->printConsole(printAssigned + (String)numbCom +" > "+returnButtonChar(readKeys), TFT_GREEN, 0, 0);
      }
  }
};


char returnButtonChar(uint8_t keyCode){
 switch (keyCode){
   case 0:        return ('-'); break;
   case PAD_LEFT: return ('L'); break;
   case PAD_UP:   return ('U'); break;
   case PAD_DOWN: return ('D'); break;
   case PAD_RIGHT:return ('R'); break;
   case PAD_ACT:  return ('A'); break;
   case PAD_ESC:  return ('E'); break;
   case PAD_LFT:  return ('F'); break;
   case PAD_RGT:  return ('G'); break;
  }  
};


uint8_t returnVectorNo(uint8_t butonNo){
  uint8_t returnNo = 0;
  for (uint8_t i = 0; i < recordIRvector.size(); i++)
   if (recordIRvector[i].assignedButton == butonNo) {
     returnNo = i+1;
     break;}
  return (returnNo); 
};


void IrMODEfunction(){
  uint8_t keyPressed;
  GUIobj->printConsole (printOneTouchMode, TFT_MAGENTA, 0, 0);
  GUIobj->printConsole (printCancelOneTouch, TFT_MAGENTA, 0, 0);
  while(1){
    keyPressed = GUIobj->getKeys();
    if(keyPressed&PAD_LFT && keyPressed&PAD_RGT) break;
    if (keyPressed)
      if (!returnVectorNo(keyPressed))
        GUIobj->printConsole (printAssignNotFound, TFT_RED, 0, 0);
      else
        IrSENDNOfunction(returnVectorNo(keyPressed)-1);
    delay(10);
  }
};


void IrSAVEfunction(){
 File f;
 uint8_t saveBuffer[sizeof(recordIR)];

  GUIobj->printConsole (printSaveData, TFT_MAGENTA, 0, 0);
  LittleFS.begin();
  if (!(f = LittleFS.open("/IRdata.dta", "w")))
    GUIobj->printConsole (printFileNotFound, TFT_RED, 0, 0);
  else{
     f.print((String)recordIRvector.size());
     f.print('\n');
     for(uint8_t i=0; i< recordIRvector.size(); i++){
       memcpy(&saveBuffer[0], &recordIRvector[i], sizeof (recordIR));
       for(uint8_t j=0; j<sizeof (recordIR); j++){  
         f.write(saveBuffer[j]);
     }
  }
  LittleFS.end(); 
  }
}


void IrLOADfunction(){
 File f;
 uint8_t vectorsRecords = 0;
 uint8_t loadBuffer[sizeof (recordIR)];

  GUIobj->printConsole (printLoadData, TFT_MAGENTA, 0, 0);
  LittleFS.begin();
  if (!(f = LittleFS.open("/IRdata.dta", "r")))
    GUIobj->printConsole (printFileNotFound, TFT_RED, 0, 0);
  else{
     vectorsRecords = f.readStringUntil('\n').toInt(); 
     for(uint8_t i=0; i< vectorsRecords; i++){
       recordIRvector.push_back(recordIR());  
       for(uint8_t j=0; j<sizeof (recordIR); j++)
         loadBuffer[j] = f.read(); 
       memcpy(&recordIRvector.back(), &loadBuffer[0], sizeof (recordIR));
     }
  }
  LittleFS.end(); 
}



void loop() {
  int8_t numbCom = 0;
  String rdCommand = "";
  
  GUIobj->printConsole(" ", TFT_BLACK, 0, 0);  
  if (!recordIRvector.size())
    GUIobj->printConsole(printNoRecords, TFT_WHITE, 0, 0);
  else
    for (uint8_t i=0; i<recordIRvector.size(); i++ )
      GUIobj->printConsole((String)(i+1) + " " + recordIRvector[i].nameIR + " > " + returnButtonChar(recordIRvector[i].assignedButton), TFT_WHITE, 0, 0);

  GUIobj->printConsole(" ", TFT_BLACK, 0, 0);  
  GUIobj->printConsole(printComList1, TFT_YELLOW, 0, 0);
  GUIobj->printConsole(printComList2, TFT_YELLOW, 0, 0);   
  GUIobj->printConsole(printCommand, TFT_MAGENTA, 0, 0);

  while (rdCommand == "") rdCommand = GUIobj->getUserInput();
  numbCom = rdCommand.toInt();

  if(numbCom){
    if (numbCom>recordIRvector.size()) GUIobj->printConsole(printWrongRecord, TFT_RED, 0, 0);
    else IrSENDNOfunction(numbCom-1);}
  else{
      switch (getCommand(rdCommand[0])){
        case IrREAD: 
          IrREADfunction();
          break;
        case IrASSIGN:
          IrASSIGNfunction();
          break;
        case IrDELETE:
          IrDELETEfunction(); 
          break;
        case IrMODE:
          if(recordIRvector.size())IrMODEfunction();
          else GUIobj->printConsole(printNoRecords, TFT_RED, 0, 0);;
          break;
        case IrSAVE:
          if(recordIRvector.size())IrSAVEfunction();
          else GUIobj->printConsole(printNoRecords, TFT_RED, 0, 0);;
          break;
        default:
          GUIobj->printConsole(printWrongCommand, TFT_RED, 0, 0);
          break;
      }
    }
    delay(100);
}

 
