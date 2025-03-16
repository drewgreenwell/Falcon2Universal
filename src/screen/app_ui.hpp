#include "png_support.h"
#include "Final_Frontier_28.h"
#include "gear.h"

class AppUi {
  #ifndef UI_DEBUG
  #define UI_DEBUG 0
  #endif
  LaserCommunicator *laser;
  int16_t h = 135;
  int16_t w = 240;
  int xPos = 50;
  int alarmX = 0;
  int alarmY = h - 32;
  int activeAlarmsLen = 2;
  int alarmLen = 14;
  int uiTick = 0;
  int uiRate = 1000;
  int alarms[14] = {1,2,4,5,6,7,8,9,10,11,12,13,14,15};
  int activeAlarms[14] = {0,1,0,0,0,0,0,0,0,0,0,0,1,0};

  public:
  
  
  int* getActiveAlarms() {
    return activeAlarms;
  }

  AppUi(LaserCommunicator *laser) {
    this->laser = laser;
  }

  void setup() {
    tft.init();
    tft.setRotation(3);
    tft.loadFont(Final_Frontier_28);
  }

  void loop() {
    long now = millis();
    if(now - uiTick > uiRate){
      uiTick = now;
      if(!laser->hasBooted()){
        if(laser->isWaitingToBoot()){
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
    // logln("pulse  " + String(TFT_WIDTH) + String(TFT_HEIGHT));
    prep_ui_update();
    xPos += 5;
    if(xPos > w){
      xPos = 0;
    }
    tft.setTextPadding(w / alarmLen / 2);
    tft.setTextSize(1.25);
    if(laser->modelName != ""){
      tft.drawString(laser->modelName, xPos, 10);
    } else {
      tft.drawString("Handshake..", xPos, 10);
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
      if(activeAlarms[i] == 1){
        active = true;
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

  void drawGear() {
    pngType = GEAR;
    int16_t rc = png.openFLASH((uint8_t *)gear, sizeof(gear), pngDraw);
    endPng(rc);
  }

  void endPng(int16_t rc) {
    uint16_t pngw = 0, pngh = 0; // To store width and height of image
    if (rc == PNG_SUCCESS) {
      //Serial.println("Successfully opened png file");
      pngw = png.getWidth();
      pngh = png.getHeight();
      //Serial.printf("Image metrics: (%d x %d), %d bpp, pixel type: %d\n", pngw, pngh, png.getBpp(), png.getPixelType());

      tft.startWrite();
      uint32_t dt = millis();
      rc = png.decode(NULL, 0);
      tft.endWrite();
      //Serial.print(millis() - dt); Serial.println("ms");
      tft.endWrite();

      // png.close(); // Required for files, not needed for FLASH arrays
    }
  }

  template <typename T>
  static void log(const T& data) {
    #if UI_DEBUG
    Serial.print(data);
    #endif
  }
  template <typename T, typename... Args>
  static void logf(const char* format, T first, Args... rest) {
    #if UI_DEBUG
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), format, first, rest...);
    Serial.print(buffer);
    #endif
  }
  template <typename T>
  static void logln(const T& data) {
    #if UI_DEBUG
    Serial.println(data);
    #endif
  }

};