
#include "Final_Frontier_28.h"

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

#include <HardwareSerial.h>
#define ADC_EN_PIN 14 // enable powering through battery port

#include "list.hpp";
#include "timer.hpp";
#define BAUD_RATE 9600

HardwareSerial SerialPort(2);


#define GREY 0xCCCCCC

int16_t h = 135;
int16_t w = 240;


TFT_eSPI tft = TFT_eSPI();
String modelName("");


int txIdxVal = 0;
// message tokens
int lengthVal = 0;
int rxIdxVal = 0;
int msgTypeVal = 0;
bool lengthMatched = false;
bool rxIndexMatched = false;
bool msgTypeMatched = false;


// last byte received
int prevData = 0;
// parsed-ish version of inputData for serial debug
String inputString("");
// flag to indicate a line of data has been received (e.g. 0x0A \n)
bool messageReceived = false;
// todo: store for alarms
// List alarms {.length=0};
// store for current and last message received
List inputData {.length=0};
List prevInputData {.length=0};

// pre formatted messages for the boot sequence. after boot we are echoing the laser, monitoring alert codes, and incrementing a message index
List bootData1 {.length=18, .data={0x49, 0x4C, 0x6D, 0x70, 0x08, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A}};
List bootData2 {.length=14, .data={0x49, 0x4C, 0x6D, 0x70, 0x04, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0A}};
List bootData3 {.length=10, .data={0x49, 0x4C, 0x6D, 0x70, 0x00, 0x0A, 0x01, 0x00, 0x00, 0x0A}};

// boot sequence
#define DEFAULT_BOOT_DELAY 1185       // amount of time after first message received before responding 
#define BOOT_INTERVAL 180             // amount of time between each boot step
#define BOOT_BUMP_AMT 32              // amount of time added to bootInterval after first message
#define BOOT1a 4                      // send bootData1 5 times
#define BOOT2a 1                      // send bootData2 1 time
#define BOOT1b 5                      // send bootData1 5 times
#define BOOT2b 1                      // send bootData2 1 time
int boot1a = BOOT1a;
int boot2a = BOOT2a;
int boot1b = BOOT1b;
int boot2b = BOOT2b;



// flags and intervals for boot
bool rxReceived = false;              // have any complete packets have been received from laser so the boot sequence can start
bool waitingToBoot = true;            // has the boot sequence started
bool booted = false;                  // has the boot sequence completed
int bootDelay = DEFAULT_BOOT_DELAY;   // number of milliseconds after input received to start boot
int bootInterval = BOOT_INTERVAL;     // number of milliseconds between each boot message
int lastWaitTick = 0;                 // timer to resend last message  todo: likely unnecessary. communication is fine without the extra chatter

int lastMessageReceived = 0;          // timer to record amount of time since last message received
int idleTimeout = 5000;               // number of milliseconds without communication from laser before resetting boot

AppTimer bootTimer {.interval = bootDelay, .active = true, .repeat = true};  // timer to track bootDelay / bootInterval
AppTimer debugTimer {.interval = 1000, .active = true, .repeat = true};     // timer to send debug messages over usb

// 49 4C 6D 70 04 04 00 01 00 00 00 01 00 0A
// 49 4C 6D 70 00 47 01 00 00 0A
// 49 4C 6D 70 00 0A 01 00 00 0A
void setup() {

  /*
    ADC_EN is the ADC detection enable port
    If the USB port is used for power supply, it is turned on by default.
    If it is powered by battery, it needs to be set to high level
    */
  pinMode(ADC_EN_PIN, OUTPUT);
  digitalWrite(ADC_EN_PIN, HIGH);
  
  modelName.reserve(100);
  inputString.reserve(200);
  // serial to usb
  Serial.begin(BAUD_RATE); 
  // serial to laser
  SerialPort.begin(BAUD_RATE,SERIAL_8N1, 25, 26);  

  ui_setup();

  Serial.println("launched..");
}

void ui_setup() {
  tft.init();
  tft.setRotation(3);
  tft.loadFont(Final_Frontier_28);
}


int xPos = 50;

int alarmX = 0;
int alarmY = h - 32;
int alarms[] = {1,2,4,5,6,7,8,9,10,11,12,13,14,15};
int activeAlarms[] = { 2, 14 };
int activeAlarmsLen = 2;
int alarmLen = 14;

void loop() {
  serial_loop();
  ui_loop();
}


void serial_loop() {
  long now = millis();
  
  // send status over usb
  debugTimer.loop();
  if(debugTimer.elapsed) {
    debugLoop();
  }
  // check if we need to start booting
  if(!booted) {
    // once input has been recived start timing
    if(rxReceived) {
      // waiting to fire our first boot message
      if(waitingToBoot) {
        bootTimer.loop();
        if(bootTimer.elapsed) {
          waitingToBoot = false;
          bootTimer.interval = bootInterval;
        }
      } else {
        // running boot sequence
        bootTimer.loop();
        if(bootTimer.elapsed) {
          // 1 tick from waiting to boot, 1 tick from boot sequence = add offset on 3rd tick
          if(bootTimer.tickCount == 3) {
            // after the first boot message there is a larger offset
            // add ~32 milliseconds here to keep things lined up
            bootTimer.interval += BOOT_BUMP_AMT;
          }
          bootLoop();
        }
      }
    } else {
      // waiting for input to start boot sequence
    }
  } else {
    // no message has been revceived in enough time to assume laser is off 
    if(now - lastMessageReceived > idleTimeout) {
      resetBoot();
    }
  }
  // if we have processed a message, update it and echo back the new message after a short delay
  if(messageReceived){
    delay(30);
    handleMessage();
    messageReceived = false;
    lastWaitTick = now;
  } 
  // unnecessary. communication is fine without this extra chatter that the controller emits
  // else if(booted && now - lastWaitTick > 38){
  //   delay(180);
  //   lastWaitTick = millis();
  //   Serial.println("write most recent " + String(prevInputData.length));
  //   // writeList(prevInputData);
  // }
  checkSerial();
}


int uiTick = 0;
int uiRate = 1000;

void ui_loop(){

  long now = millis();
  if(now - uiTick > uiRate){
    uiTick = now;
    if(!booted){
      if(waitingToBoot){
        waiting_ui_update();
      }else {
        boot_ui_update();
      }
    } else {
      ui_update();
    }
  }
}

void prep_ui_update() {
  tft.fillScreen(TFT_BLACK);  
  tft.setTextWrap(false);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void waiting_ui_update() {
  prep_ui_update();
  tft.setTextPadding(w / alarmLen / 2);
  tft.setTextSize(2);
  tft.drawString("Waiting to boot.", 10, (h / 2) - 10);
}

void boot_ui_update() {
  prep_ui_update();
  tft.setTextPadding(w / alarmLen / 2);
  tft.setTextSize(2);
  tft.drawString("Booting", 10, (h / 2) - 10);
}

void ui_update() {
  // Serial.println("pulse  " + String(TFT_WIDTH) + String(TFT_HEIGHT));
  prep_ui_update();
  xPos += 5;
  if(xPos > w){
    xPos = 0;
  }
  tft.setTextPadding(w / alarmLen / 2);
  tft.setTextSize(1.25);
  if(modelName != ""){
    tft.drawString(modelName, xPos, 10);
  } else {
    tft.drawString("TEST", xPos, 10);
  }
  draw_alarm_codes();
}

void draw_alarm_codes() {
  tft.setTextSize(2.75);
  for(int i = 0; i < alarmLen; i++){
    int x;
    int y;
    float half = alarmLen / 2;
    if(i < half){
      x = i * ( w / half) + 10;
      y = h - 62;
    } else {
      x = (i - half) * (w / half) + 5;
      y = h - 22;
      if(alarms[i] == 9) // start of second row (single digit)
      {
        x += 5;
      }
    }
    bool active = false;
    for(int j = 0; j < activeAlarmsLen; j++){
      if(activeAlarms[j] == alarms[i]){
        active = true;
      }
    }
    if(active) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
    } else {
      tft.setTextColor(GREY, TFT_BLACK);
    }
    tft.drawString(String(alarms[i]), x, y);
  }
}

void setMessage(const char *msg) {
  TFT_eSprite m = TFT_eSprite(&tft);
  m.setColorDepth(8);
  m.createSprite(w, 24);
  m.fillSprite(TFT_BLACK);

  m.setTextFont(1);
  m.setTextDatum(TL_DATUM);
  m.setTextColor(TFT_WHITE);
  m.setCursor(0, 0);
  m.print(msg);
 
  m.pushSprite(0, 10);
  m.deleteSprite();
}


// restart timings and counts for boot loop
void resetBoot() {
  Serial.println("Resetting boot!");
  bootDelay = DEFAULT_BOOT_DELAY;
  bootInterval = BOOT_INTERVAL;
  boot1a = BOOT1a;
  boot2a = BOOT2a;
  boot1b = BOOT1b;
  boot2b = BOOT2b;
  rxReceived = false;
  booted = false;
  waitingToBoot = true;
}

// once communication starts, send a series of messages to handshake with the laser
void bootLoop(){
  Serial.println("boot loop");
  if(boot1a > 0){
    writeList(bootData1);
    boot1a -= 1;
    Serial.print("boot1a - ");
    Serial.println(boot1a);
  } else if(boot2a > 0){
    writeList(bootData2);
    boot2a -= 1;
    Serial.print("boot2a - ");
    Serial.println(boot2a);
  } else if(boot1b > 0){
    writeList(bootData1);
    boot1b -= 1;
    Serial.print("boot1b - ");
    Serial.println(boot1b);
  } else if(boot2b > 0) {
    writeList(bootData3);
    boot2b -= 1;
    Serial.print("boot2b - ");
    Serial.println(boot2b);
  } else {
    booted = true;
  }
}

void debugLoop(){
    Serial.print("pulse ");
    if(waitingToBoot){
      Serial.println("waiting for input to boot.");
    } else if(!booted){
      Serial.println("booting.");
    } else {
      Serial.println("booted.");
    }
}

void handleMessage(){
  Serial.println(inputString + "---" + (inputData.length) + "---" + (lengthVal));
  // dont respond with the laser header, just dump last line
  if(!booted){
    inputData.clear();
    inputString = "";
  }
  if (inputData.length >= 100){
    byte dataLen = inputData.data[4];
    byte dataStart = 8;
    // byte end = dataStart + dataLen;
    byte chars = 20;
    byte end = dataStart + chars;
    for(int i = dataStart; i < end; i++){
      modelName.concat((char)inputData.data[i]);
    }
    modelName.trim();
    Serial.print("writing previous line instead of header: ");
    Serial.println(modelName);
    writeList(prevInputData);
  } else {
    saveList(inputData, prevInputData);
    writeList(inputData);
  }
  inputData.clear();
  inputString = "";
}

// save one list to another e.g. inputData to prevInputData
void saveList(List &list, List &toList){
  toList.clear();
  for (byte i = 0; i < list.length; i++){
    toList.append(list.data[i]);
  }
}

// write the packet to serial
void writeList(List &list){
  list.data[5] = txIdxVal;
  byte lastByte;
  for(byte i = 0; i < list.length; i++){
    lastByte = list.data[i];
    SerialPort.write(lastByte);
    Serial.print((char)lastByte);
  }
  // make sure debug message is closed
  if(lastByte != 0x0A){
    Serial.println();
  }
  txIdxVal += 1;
  if(txIdxVal > 255){
    txIdxVal = 0;
  }
  delay(30);
}

// check for communication and start parsing
void checkSerial() {
  // Serial.println("Check serial");
  if(messageReceived){
    Serial.println("skip serial check");
    return;
  }
  int bytes = 0;
  while(SerialPort.available()){
    lastMessageReceived = millis(); 
    int data = SerialPort.read();
    inputData.append((byte)data);
    prevData = data;
    bytes += 1;
    // processing more than one message break
    if(bytes > 7){
      if((char)data == '\n'){
        break;
      }
    }
  }
  if(bytes > 0){
    Serial.print("Input Data Length: ");
    Serial.print(bytes);
    Serial.print(" : ");
    Serial.println(inputData.length);
    if(inputData.length > 0){
      byte lv = inputData.data[inputData.length - 1];
      char inChar = (char)lv;
      Serial.print("Last Char: ");
      Serial.print((int)lv);
      if(inChar == '\n'){
        Serial.print("New Line");
      }
      Serial.println(inChar);
      if(inChar == '\n'){
        if(rxReceived){
          messageReceived = true;
          parse_input_data();
        } else {
          // flag that input has been received so booting can begin
          rxReceived = true;
          parse_input_data();
          Serial.print("First Input: ");
          Serial.println(inputString);
          inputString = "";
          inputData.clear();
        }   
      }
    }
  }
}

void parse_input_data() {
  lengthVal = (int)inputData.data[4];
  rxIdxVal = (int)inputData.data[5];
  msgTypeVal = (int)inputData.data[6];
  inputString.clear();
  inputString.concat((char)inputData.data[0]);
  inputString.concat((char)inputData.data[1]);
  inputString.concat((char)inputData.data[2]);
  inputString.concat((char)inputData.data[3]);
  inputString.concat("-");
  inputString.concat(lengthVal);
  inputString.concat("-");
  inputString.concat(rxIdxVal);
  inputString.concat("-");
  inputString.concat(msgTypeVal);
  inputString.concat("-");
  if(inputData.length > 7 + lengthVal) {
    for(int i = 7; i < inputData.length; i++) {
      if(inputData.length > 50){
        inputString.concat(String(inputData.data[i], HEX) + " ");
      } else {
        inputString.concat(String(inputData.data[i], HEX) + " ");
      }
    }
  }
  Serial.print("RX ");
  Serial.print(rxIdxVal);
  Serial.print(" Parsed String Length: ");
  Serial.println(inputData.length);
  Serial.println(inputString);
}

// void checkSerial() {
//   if(messageReceived){
//     return;
//   }
//   while (SerialPort.available()) 
//   {
//     lastMessageReceived = millis(); 
//     int data = SerialPort.read();
//     // append the data to a list of bytes
//     // this and the line complete check below is essentially all that is needed.
//     // the rest of this method is an attempt to parse out the values for debugging
//     inputData.append(data);
    
//     // attempt to parse data
//     char inChar = (char)data;
//     // example message formats
//     // START(4) LEN(1) IDX(1) TYPE(1) ?(1) DELIM?(4) ?(3) ALARMCODE(1) NL(1)
//     // 49 4C 6D 70 08 07 07 80  47 00 00 80 3F 0D 03 0A
//     // 49 4C 6D 70 08 13 07 00 D1 B5 47 00 00 40 40 4D 02 0A
//     // 49 4C 6D 70 08 CF 07 80 32 B9 47 00 80 DB 44 51 03 0A
//     // START(4) LEN(1) IDX(1) ?(1) TYPE(1) ?(4) NUL(1) NL(1) 
//     // 49 4C 6D 70 04 09 03 03 03 00 00 06 00 0A
//     // 49 4C 6D 70 04 0A 00 02 00 00 00 02 00 0A 
//     if(inputString == "ILmp") { // 49 4C 6D 70
//       // the message length
//       lengthVal = data;
//       inputString += data;
//       lengthMatched = true;
//     } else if(lengthMatched) {
//       // the current rx line index
//       rxIdxVal = data;
//       inputString.concat(data);
//       // reset length and move to next token
//       lengthMatched = false;
//       rxIndexMatched = true;
//     } else if (rxIndexMatched) {
//       // message type (control character 07=BEL or 00=NUL)
//       msgTypeVal = data;
//       inputString.concat(String(data, HEX));
//       // reset index match and move to next token
//       rxIndexMatched = false;
//       msgTypeMatched = true;
//     } else if (msgTypeMatched) {
//       // ? = data;
//       inputString.concat(String(data, HEX));
//       // move to next token
//       msgTypeMatched = false;
//     }  
//     else {
//       // not a known data bit. handle as char
//       inputString += inChar;
//     }
//     // if the incoming character is a newline, set a flag so the main loop can
//     if (inChar == '\n') {
//       if(rxReceived){
//         messageReceived = true;
//       } else {
//         // flag that input has been received so booting can begin
//         rxReceived = true;
//         inputString = "";
//         inputData.clear();
//       }
//     }
//     prevData = data;
//   } 
// }

