#define APP_VERSION "2.0.0"
#define APP_DEBUG true
#define LASER_DEBUG true

#include <WiFi.h>
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include "app_ota.hpp";
AppOta ota;

#include <Preferences.h>
Preferences prefs;

//#include <WiFiManager.h> // too heavy

// Secondary Serial Port for Communicating with Laser
#include <HardwareSerial.h>
#define BAUD_RATE 9600
HardwareSerial SerialPort(2);

// Common Utility Classes
#include "list.hpp";
#include "timer.hpp";

// Laser Config
#include "laser_communicator.hpp";
LaserCommunicator laser(&SerialPort);

// LilyGO T-Display
#include <TFT_eSPI.h> 
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI();
#define GREY 0xCCCCCC
#include "app_ui.hpp"
AppUi ui(&laser);


#if APP_DEBUG
// timer to send debug messages over serial
AppTimer debugTimer(1000, true, true);  // AppTimer(int interval, bool active, bool repeat)
#endif

// AsyncWebServer server(80);

void setup() {
  // save build version for updates
  prefs.begin("falcon-2-u", true);
  prefs.putString("version", APP_VERSION);
  // serial to usb
  Serial.begin(BAUD_RATE); 
  // serial to laser
  SerialPort.begin(BAUD_RATE, SERIAL_8N1, 25, 26);  
  
  // this will be toggled by a button press
  ota.setup();

  ui.setup();

  laser.setup();
  
  logln("launched..");
}

void loop() {
  // communicate with the laser
  laser.loop();
  // update the ui
  ui.loop();
  // poll ota, this will be toggled via button
  ota.loop();
  // send status over serial
  #if APP_DEBUG
  debugTimer.loop();
  if(debugTimer.elapsed()) {
    debugLoop();
  }
  #endif
}

void debugLoop(){
    log("pulse ");
    if(laser.isWaitingToBoot()){
      logln("waiting for input to boot.");
    } else if(!laser.hasBooted()){
      logln("booting.");
    } else {
      logln("booted.");
    }
}

template <typename T>
void log(const T& data) {
  #if APP_DEBUG
  Serial.print(data);
  #endif
}

template <typename T>
void logln(const T& data) {
  #if APP_DEBUG
  Serial.println(data);
  #endif
}

