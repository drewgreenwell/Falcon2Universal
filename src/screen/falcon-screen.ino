#define APP_VERSION "2.0.0"
#define LASER_RX_PIN 25
#define LASER_TX_PIN 26
#define APP_DEBUG 1
#define UI_DEBUG 1
#define LASER_DEBUG 1
#define OTA_DEBUG 1

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
#include "laser_communicator.hpp";
LaserCommunicator laser(&SerialPort);

/*
  LilyGO T-Display
*/
#include <TFT_eSPI.h> 
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI();

#define MAX_IMAGE_WIDTH 64 // Sets rendering line buffer lengths, adjust for your images
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
  prefs.begin("falcon-2-u", true);
  prefs.putString("version", APP_VERSION);
  // serial to usb
  Serial.begin(BAUD_RATE); 
  // serial to laser
  SerialPort.begin(BAUD_RATE, SERIAL_8N1, LASER_RX_PIN, LASER_TX_PIN);  
  
  laser.setup();

  button_setup();

  ui.setup();
  
  // this will be toggled by a button press
  ota.setup();

  logln("launched..");
}

void loop() {
  // communicate with the laser
  laser.loop();
  // check button presses
  button_loop();
  // update the ui
  ui.loop();
  // poll ota, this will be toggled via button
  ota.loop();
  // send status over serial
  #if APP_DEBUG
  debugLoop();
  #endif
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
  }
}

void button_setup() {
  button1.begin(BTN1_PIN);
  button2.begin(BTN2_PIN);
  button1.setTapHandler(click1);
  button2.setTapHandler(click2);
}

void button_loop() {
  button1.loop();
  button2.loop();
}

void click1(Button2& b) {
  // check for really long clicks
  if (b.wasPressedFor() > 1000) {
    log("Button 1 Long Press For");
    logln(b.wasPressedFor());
  } else {
    logln("Button 1 Press");
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

