#define APP_VERSION "2.0.1"

#define LASER_RX_PIN 25
#define LASER_TX_PIN 26
#define LASER_TRIGGER_PIN 2
#define LASER_MONITOR_PIN 15
#define LASER_ALARM_PIN 27
#define LASER_STATE_ON_BOOT HIGH // LOW for fans powered down by default, HIGH for fans on

#define BTN1_PIN 35
#define BTN2_PIN 0

#define APP_DEBUG 1
#define UI_DEBUG 1
#define LASER_DEBUG 1
#define OTA_DEBUG 1

#define UI_TESTING 0            // flag for testing ui states

#define BAUD_RATE 9600

#define CONFIG_TFT_SMOOTH_FONT
//#define SMOOTH_FONT
#define FS_NO_GLOBALS

/*
  Network
*/
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <PNGdec.h>
#include <Button2.h>
/*
  Common Utility Classes
*/
#include "list.hpp"
#include "timer.hpp"

#include "app_ota.hpp"
#include "laser_communicator.hpp"
#include "app_ui.hpp"

AsyncWebServer server(80);
Preferences prefs;

/*
  OTA
*/
AppOta ota(&server, &prefs);

/*
  Secondary Serial Port for Communicating with Laser
*/
HardwareSerial SerialPort(2);


/* 
  Laser Config
*/
LaserCommunicator laser(&SerialPort);

/*
  LilyGO T-Display
*/

PNG png; // PNG decoder instance
TFT_eSPI display = TFT_eSPI();

AppUi ui = AppUi::init(&laser, &png, &display, &ota);

Button2 button1;
Button2 button2;


#if APP_DEBUG
// timer to send debug messages over serial
AppTimer debugTimer(1000, true, true);  // AppTimer(int interval, bool active, bool repeat)
#endif

void setup() {
  // save build version for updates
  prefs.begin("falcon-2-u", false);
  // reset credentials
  // String version = prefs.getString("version", "2.0.0");
  //if (version == "2.0.0") {
  //  prefs.putString("ssid", "");
  //  prefs.putString("password", "");
  //}
  prefs.putString("version", APP_VERSION);
  // serial to usb
  Serial.begin(BAUD_RATE); 
  // serial to laser
  SerialPort.begin(BAUD_RATE, SERIAL_8N1, LASER_RX_PIN, LASER_TX_PIN);  
  logln("Setting up laser");
  laser.setup();
  logln("Setting up app");
  app_setup();
  logln("Setting up ui");
  ui.setup();
  logln("setting up ota");
  // ota.begin() is called on long press of button 2
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
  // set alarm pin to output
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
  if(ota.hosting){
    if(ota.resetFlag){
      prefs.putString("ssid", "");
      prefs.putString("password", "");
      logln("restarting esp because of reset request credentials request.");
      ESP.restart();
    } else {
      ota.resetFlag = true;
    }
  } else {
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
}
void click2(Button2& b) {
    // check for really long clicks
    if (b.wasPressedFor() > 1000) {
      log("Button 2 Long Press For:");
      logln(b.wasPressedFor());
      
      if(ota.polling){
        ota.end();
        ui.setConfigActive(false);
        ota.resetFlag = false;
      } else {
        ota.begin();
        ui.setConfigActive(true);
      }
    } else {
      if(ota.polling){
        ota.end();
        ui.setConfigActive(false);
        ota.resetFlag = false;
      }
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

