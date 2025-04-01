#include "ui_support.h"                   // global png decode / screen positioning
#include "ui_font.h"
// images
#include "gear.h"                         // config gear
#include "splash.h"                       // loading / main screen
#include "laser-beam.h"                   // laser/fans awake with an active beam
#include "laser-beam-inactive.h"          // laser/fans awake with no active beam
#include "laser-beam-bw.h"                // laser/fans asleep
#include "logo.h"                         // Falcon Universal in Futura

class AppUi {
  #ifndef UI_DEBUG
  #define UI_DEBUG 0
  #endif
  LaserCommunicator *laser;
  PNG *pngDecoder;
  TFT_eSPI *tft;
  int16_t h = 135;
  int16_t w = 240;
  int alarmX = 0;
  int alarmY = h - 32;
  int activeAlarmsLen = 2;
  int alarmLen = 14;
  int uiTick = 0;
  int uiRate = 1000;
  int waitTicks = 0;
  int alarms[14] = {1,2,4,5,6,7,8,9,10,11,12,13,14,15};
  int activeAlarms[14] = {0,1,0,0,0,0,0,0,0,0,0,0,1,0};


  bool modelNameWritten = false;  // has the modelName been drawn on the screen, a delay in handshake may leave this blank the first couple loops

  enum View { SPLASH, WAITING, BOOTING, BOOTED, CONFIG, NONE };
  View view = SPLASH;
  View lastView = SPLASH;
  
  uint8_t defaultFontId = 0;
  uint8_t largeFontId = 0;

  AppUi(LaserCommunicator *laser, PNG *png, TFT_eSPI *display) {
    this->laser = laser;
    this->pngDecoder = png;
    this->tft = display;
    AppUi::instance = this;
  }

  public:

  static inline AppUi* instance = nullptr;
  
  static AppUi init(LaserCommunicator *laser, PNG *png, TFT_eSPI *display) {
    AppUi appUi = AppUi(laser, png, display);
    return appUi;
  }

  int* getActiveAlarms() {
    return activeAlarms;
  }

  void setup() {
    tft->init();
    tft->setRotation(3);
    //tft->loadFont(AA_FONT);
    //tft->setFreeFont(UI_FONT);

  }

  void loop() {
    long now = millis();
    if(now - uiTick > uiRate){
      uiTick = now;
      bool laserBooted = laser->hasBooted();
      bool waitingToBoot = laser->isWaitingToBoot();
#if UI_TESTING
      view = BOOTED;
      lastView = BOOTING;
      laserBooted = true;
      waitingToBoot = false;
#endif
      if(!laserBooted){
        if(waitingToBoot){
          if(view == WAITING && lastView == WAITING){
            // return;
            waitTicks += 1;
            if(waitTicks > 3){
              waitTicks = 0;
            }
          }
          view = WAITING;
          waiting_ui_update();
        }else {
          if(view == BOOTING && lastView == BOOTING){
            return;
          }
          view = BOOTING;
          boot_ui_update();
        }
      } else {
        view = BOOTED;
        ui_update();
      }
      lastView = view;
    }
  }
  
  void prep_ui_update() {
    tft->fillScreen(TFT_WHITE);  
    tft->setTextWrap(false);
    tft->setTextColor(TFT_BLACK, TFT_WHITE);
  }

  void waiting_ui_update() {
    if(lastView != WAITING && lastView != SPLASH) {
      prep_ui_update();
      drawSplash();
    }
    //tft->setTextPadding(w / alarmLen / 2);
    tft->setTextSize(2);
    tft->loadFont(UI_FONT_SMALL);
    String msg("WAITING");
    int amt = waitTicks;
    while(amt--){
      msg.concat(String("."));
    }
    tft->fillRect(90, h * .65, 150, 30, TFT_WHITE);
    tft->drawString(msg, 90, h * .65);
  }

  void boot_ui_update() {
    prep_ui_update();
    drawSplash();
    //tft->setTextPadding(w / alarmLen / 2);
    tft->loadFont(UI_FONT_SMALL);
    tft->setTextSize(2);
    tft->drawString("BOOTING", 90, h * .65);
  }

  void ui_update() {
    if(lastView != BOOTED){
      prep_ui_update();
      drawLogo();
      drawVersion();
      modelNameWritten = false;
    }

    if(!modelNameWritten) {
      tft->loadFont(UI_FONT_LARGE);
      tft->setTextDatum(TC_DATUM);
      tft->setTextSize(1);
      if(laser->modelName != "") {
        tft->drawString(laser->modelName, w / 2, 10);
        modelNameWritten = true;
      } else {
        tft->drawString("--", w / 2, 10);
      }
      tft->setTextDatum(TL_DATUM);
    }
    
    #if !UI_TESTING
    drawBeam();
    #else
    waitTicks += 1;
    if(waitTicks % 3 == 0) {
      drawBeamInactive();
    } else if(waitTicks % 2 == 0) {
      drawBeamNoPower();
    } else {
      drawBeamActive();
    }
    if(waitTicks > 12){
      waitTicks = 0;
    }
    #endif

    draw_alarm_codes();
  }

  void draw_alarm_codes() {
    tft->unloadFont();
    tft->setTextSize(2);
    tft->drawString("Air", 100, h * .65);
    return;
    tft->setTextSize(3);
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
        tft->setTextColor(TFT_RED, TFT_BLACK);
      } else {
        tft->setTextColor(GREY, TFT_BLACK);
      }
      tft->drawString(String(alarms[i]), x, y);
    }
  }

  void drawVersion() {
    tft->unloadFont();
    tft->setTextSize(1);
    tft->setTextColor(TFT_BLACK);
    String version("v");
    version.concat(APP_VERSION);
    tft->drawString(version, 180, h * .9);
  }

  void setMessage(const char *msg) {
    TFT_eSprite m = TFT_eSprite(tft);
    m.setColorDepth(8);
    m.createSprite(w, 24);
    m.fillSprite(TFT_WHITE);

    m.setTextFont(1);
    m.setTextDatum(TL_DATUM);
    m.setTextColor(TFT_BLACK);
    m.setCursor(0, 0);
    m.print(msg);

    m.pushSprite(0, 10);
    m.deleteSprite();
  }

  void drawGear() {
    pngType = GEAR_PNG;
    int16_t rc = pngDecoder->openFLASH((uint8_t *)gear, sizeof(gear), pngDraw);
    endPng(rc);
  }

  void drawSplash() {
    pngType = SPLASH_PNG;
    int16_t rc = pngDecoder->openFLASH((uint8_t *)splash, sizeof(splash), pngDraw);
    endPng(rc);
    drawVersion();
  }

  void drawLogo() {
    // todo: getting some artifacts at the edge on this image
    /*pngType = LOGO_PNG;
    tft->fillRect(logoX, logoY, logoW, logoH, TFT_WHITE);
    int16_t rc = pngDecoder->openFLASH((uint8_t *)logo, sizeof(logo), pngDraw);
    endPng(rc);*/
  }

  void drawBeam() {
    if (laser->laserState == HIGH){
      if(laser->laserVal > 0){
        drawBeamActive();
      } else {
        drawBeamInactive();
      }
    } else {
      drawBeamNoPower();
    }
  }

  void drawBeamActive() {
    pngType = BEAM_PNG;
    int16_t rc = pngDecoder->openFLASH((uint8_t *)laser_beam, sizeof(laser_beam), pngDraw);
    endPng(rc);
  }

  void drawBeamInactive() {
    pngType = BEAM_INACTIVE_PNG;
    int16_t rc = pngDecoder->openFLASH((uint8_t *)laser_beam_inactive, sizeof(laser_beam_inactive), pngDraw);
    endPng(rc);
  }

  void drawBeamNoPower() {
    pngType = BEAM_BW_PNG;
    int16_t rc = pngDecoder->openFLASH((uint8_t *)laser_beam_bw, sizeof(laser_beam_bw), pngDraw);
    endPng(rc);
  }

  static void pngDraw(PNGDRAW *pDraw) {
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];          // Line buffer for rendering
    uint8_t  maskBuffer[1 + MAX_IMAGE_WIDTH / 8];  // Mask buffer

    AppUi::instance->pngDecoder->getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    if (AppUi::instance->pngDecoder->getAlphaMask(pDraw, maskBuffer, 255)) {
      int16_t x = 0;
      int16_t y = 0;
      switch(pngType){
        case SPLASH_PNG:
        x = 0;
        y = 0;
        break;
        case LOGO_PNG:
        x = logoX;
        y = logoY;
        break;
        case BEAM_PNG:
        case BEAM_BW_PNG:
        case BEAM_INACTIVE_PNG:
        x = beamX;
        y = beamY;
        break;
        case GEAR_PNG:
        default:
        x = gearX;
        y = gearY;
        break;
      }
      // Note: pushMaskedImage is for pushing to the TFT and will not work pushing into a sprite
      AppUi::instance->tft->pushMaskedImage(x, y + pDraw->y, pDraw->iWidth, 1, lineBuffer, maskBuffer);
    }
  }

  

  void endPng(int16_t rc) {
    uint16_t pngw = 0, pngh = 0; // To store width and height of image
    if (rc == PNG_SUCCESS) {
      //Serial.println("Successfully opened png file");
      pngw = pngDecoder->getWidth();
      pngh = pngDecoder->getHeight();
      //Serial.printf("Image metrics: (%d x %d), %d bpp, pixel type: %d\n", pngw, pngh, pngDecoder->getBpp(), pngDecoder->getPixelType());

      tft->startWrite();
      uint32_t dt = millis();
      rc = pngDecoder->decode(NULL, 0);
      tft->endWrite();
      //Serial.print(millis() - dt); Serial.println("ms");
      tft->endWrite();

      // pngDecoder->close(); // Required for files, not needed for FLASH arrays
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
