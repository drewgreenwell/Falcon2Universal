#define APP_VERSION "2.0.0"

#define LASER_RX_PIN 25
#define LASER_TX_PIN 26
#define LASER_TRIGGER_PIN 2
#define LASER_MONITOR_PIN 15
#define LASER_ALARM_PIN 27

#define APP_DEBUG 1
#define UI_DEBUG 1
#define LASER_DEBUG 1
#define OTA_DEBUG 1

#define UI_TESTING 1

/*
  Network
*/
#include <WiFi.h>
//#include <WiFiManager.h> // too heavy
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);

#include <Preferences.h>
Preferences prefs;

/*
  OTA
*/
#include "app_ota.hpp";
AppOta ota(&server);

/*
  Secondary Serial Port for Communicating with Laser
*/
#include <HardwareSerial.h>
#define BAUD_RATE 9600
HardwareSerial SerialPort(2);

/*
  Common Utility Classes
*/
#include "list.hpp";
#include "timer.hpp";

/* 
  Laser Config
*/
#define LASER_STATE_ON_BOOT LOW // LOW for fans powered down by default, HIGH for fans on
#include "laser_communicator.hpp";
LaserCommunicator laser(&SerialPort);

/*
  LilyGO T-Display
*/
#define CONFIG_TFT_SMOOTH_FONT
//#define SMOOTH_FONT
#define FS_NO_GLOBALS
#include <FS.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI();

#include <PNGdec.h>
PNG png; // PNG decoder instance

#define GREY 0xCCCCCC
#include "app_ui.hpp"
AppUi ui(&laser);

#include <Button2.h>
#define BTN1_PIN 35
#define BTN2_PIN 0
Button2 button1;
Button2 button2;


#if APP_DEBUG
// timer to send debug messages over serial
AppTimer debugTimer(1000, true, true);  // AppTimer(int interval, bool active, bool repeat)
#endif

void setup() {
  // save build version for updates
  prefs.begin("falcon-2-u", false);
  prefs.putString("version", APP_VERSION);
  // serial to usb
  Serial.begin(BAUD_RATE); 
  // serial to laser
  SerialPort.begin(BAUD_RATE, SERIAL_8N1, LASER_RX_PIN, LASER_TX_PIN);  
  
  laser.setup();
  
  app_setup();

  //if (!SPIFFS.begin()) {
    //  logln("SPIFFS initialisation failed!");
      // while (1) yield(); // Stay here twiddling thumbs waiting
   // }

  //listDir(SPIFFS, "/", true);

  ui.setup();

  ui.drawSplash();
  
  // this will be toggled by a button press
  ota.setup();

  logln("launched..");
}

void loop() {
  // communicate with the laser
  laser.loop();
  // manage trigger, monitor, buttons, and alarms
  app_loop();
  // update the ui
  ui.loop();
  // poll ota, this will be toggled via button
  ota.loop();
  // send status over serial
  #if APP_DEBUG
  debugLoop();
  #endif
}

void app_setup() {
  // enable laser trigger and set power to on at boot
  // todo: make configurable
  pinMode(LASER_TRIGGER_PIN, OUTPUT);
  laser.laserState = prefs.getUChar("laser-boot", LASER_STATE_ON_BOOT);
  digitalWrite(LASER_TRIGGER_PIN, laser.laserState);
  // set alarm pin to output, pulled high by external resistor
  pinMode(LASER_ALARM_PIN, OUTPUT);
  // set laser monitor to input to monitor laser pulses
  pinMode(LASER_MONITOR_PIN, INPUT);

  button_setup();

}

void button_setup() {
  button1.begin(BTN1_PIN);
  button2.begin(BTN2_PIN);
  button1.setTapHandler(click1);
  button2.setTapHandler(click2);
}

void debugLoop(){
  debugTimer.loop();
  if(!debugTimer.elapsed()) {
    return;
  }
  log("pulse ");
  if(laser.isWaitingToBoot()){
    logln("waiting for input to boot.");
  } else if(!laser.hasBooted()){
    logln("booting.");
  } else {
    logln("booted.");
    logf("Laser voltage: %fV ( %hu / 4095)\r\n", laser.laserVoltage, (unsigned short int)laser.laserVal);
  }
}

void app_loop() {
  button1.loop();
  button2.loop();
  // max 4095
  uint16_t newLaserVal = analogRead(LASER_MONITOR_PIN);
  if(newLaserVal != laser.laserVal) {
    if(laser.laserVal > 0){
      laser.laserVoltage = 3.3 / (4095  / laser.laserVal);
    } else {
      laser.laserVoltage = 0;
    }
    laser.lastLaserChange = millis();
  }
}

void click1(Button2& b) {
  // check for really long clicks
  if (b.wasPressedFor() > 1000) {
    log("Button 1 Long Press For");
    logln(b.wasPressedFor());
  } else {
    logln("Button 1 Press, Sleep / Wake");
    uint8_t currentState = digitalRead(LASER_TRIGGER_PIN);
    laser.laserState = currentState == HIGH ? LOW : HIGH;
    digitalWrite(LASER_TRIGGER_PIN, laser.laserState);
  }
}
void click2(Button2& b) {
    // check for really long clicks
    if (b.wasPressedFor() > 1000) {
      log("Button 2 Long Press For:");
      logln(b.wasPressedFor());
      ui.drawGear();
    } else {
      logln("Button 2 Press");
    }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  fs::File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  fs::File file = root.openNextFile();
  while (file) {

    if (file.isDirectory()) {
      Serial.print("DIR : ");
      String fileName = file.name();
      Serial.print(fileName);
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      String fileName = file.name();
      Serial.print("  " + fileName);
      int spaces = 32 - fileName.length(); // Tabulate nicely
      if (spaces < 1) spaces = 1;
      while (spaces--) Serial.print(" ");
      String fileSize = (String) file.size();
      spaces = 8 - fileSize.length(); // Tabulate nicely
      if (spaces < 1) spaces = 1;
      while (spaces--) Serial.print(" ");
      Serial.println(fileSize + " bytes");
    }

    file = root.openNextFile();
  }
}

template <typename T>
void log(const T& data) {
  #if APP_DEBUG
  Serial.print(data);
  #endif
}

template <typename T, typename... Args>
void logf(const char* format, T first, Args... rest) {
  #if APP_DEBUG
  static char buffer[256];
  snprintf(buffer, sizeof(buffer), format, first, rest...);
  Serial.print(buffer);
  #endif
}

template <typename T>
void logln(const T& data) {
  #if APP_DEBUG
  Serial.println(data);
  #endif
}

