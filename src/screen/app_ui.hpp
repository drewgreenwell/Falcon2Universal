#include "ui_support.h"                   // global png decode / screen positioning
#include "ui_font.h"
// images
#include "gear.h"                         // config gear
#include "splash.h"                       // loading / main screen
#include "laser-beam.h"                   // laser/fans awake with an active beam
#include "laser-beam-inactive.h"          // laser/fans awake with no active beam
#include "laser-beam-bw.h"                // laser/fans asleep

class AppUi {
  #ifndef UI_DEBUG
  #define UI_DEBUG 0
  #endif
  LaserCommunicator *laser;
  PNG *pngDecoder;
  TFT_eSPI *tft;
  AppTimer uiTimer = AppTimer(100, true, true); // delay, active, repeat
  int16_t screenHeight = 135;
  int16_t screenWidth = 240;
  int activeAlarmsLen = 2;
  int alarmLen = 14;
  
  int waitTicks = 0;  // ellipsis 
  int alarms[14] = {1,2,4,5,6,7,8,9,10,11,12,13,14,15};
  int activeAlarms[14] = {0,1,0,0,0,0,0,0,0,0,0,0,1,0};

  bool modelNameWritten = false;  // has the modelName been drawn on the screen, a delay in handshake may leave this blank the first couple loops

  enum View { SPLASH, WAITING, BOOTING, BOOTED, CONFIG, NONE };
  View view = SPLASH;
  #if !UI_TESTING
  View lastView = SPLASH;
  #else
  View lastView = BOOTING;
  #endif
  int lastState = -1;  // last laser state 0 off, 1 inactive, 2 active
  
  uint8_t defaultFontId = 0;
  uint8_t largeFontId = 0;

  AppUi(LaserCommunicator *laser, PNG *png, TFT_eSPI *display) {//}: spr(display) {
    this->laser = laser;
    this->pngDecoder = png;
    this->tft = display;
    AppUi::instance = this;
  }

  public:

  static inline AppUi* instance = nullptr;  // static instance for png decoding to reference
  
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
    tft->fillScreen(TFT_WHITE);
    drawSplash();
  }

  void loop() {
    uiTimer.loop();
    if(uiTimer.elapsed()) {
      bool laserBooted = laser->hasBooted();
      bool waitingToBoot = laser->isWaitingToBoot();
#if UI_TESTING
      view = BOOTED;
      laserBooted = true;
      waitingToBoot = false;
#endif
      if(!laserBooted){
        if(waitingToBoot){
          if(view == WAITING && lastView == WAITING) {
            waitTicks += 1;
            if(waitTicks > 3){
              waitTicks = 0;
            }
          }
          view = WAITING;
          logln("waiting");
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
    String msg("WAITING");
    int amt = waitTicks;
    while(amt--){
      msg.concat(String("."));
    }
    drawString(msg.c_str(), bootingX, bootingY, bootingW, bootingH, TC_DATUM, 2, UI_FONT_SMALL);
  }

  void boot_ui_update() {
    prep_ui_update();
    drawSplash();
    drawString("BOOTING", bootingX, bootingY, bootingW, bootingH, TC_DATUM, 2, UI_FONT_SMALL);
  }

  void ui_update() {
    if(lastView != BOOTED){
      prep_ui_update();
      modelNameWritten = false;
    }
    if(!modelNameWritten) {
      if(laser->modelName != "") {
        drawString(laser->modelName.c_str(), modelX, modelY, modelW, modelH, TC_DATUM, 2, UI_FONT_LARGE);
        modelNameWritten = true;
      } else {
        drawString("Receiving Data", modelX, modelY, modelW, modelH, TC_DATUM, 2, UI_FONT_LARGE);
      }
    }
    
    drawBeam();
    drawVersion();
    draw_alarm_codes();
    
  }

  void draw_alarm_codes() {
    String str("Air ");
    str.concat(millis());
    drawString(str.c_str(), alarmX, alarmY, alarmW, alarmH, TC_DATUM, 2, UI_FONT_LARGE);
    return;
    tft->setTextSize(3);
    for(int i = 0; i < alarmLen; i++){
      int x;
      int y;
      float half = alarmLen / 2;
      if(i < half){
        x = i * ( screenWidth / half) + 10;
        y = screenHeight - 62;
      } else {
        x = (i - half) * (screenWidth / half) + 5;
        y = screenHeight - 22;
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
    String version("v");
    version.concat(APP_VERSION);
    drawString(version.c_str(), versionX, versionY, versionW, versionH);
  }

  void drawString(const char *msg, int x, int y, int width, int height, uint8_t datum = TL_DATUM, int fontSize = 1, const uint8_t font[] = nullptr) {
    TFT_eSprite m = TFT_eSprite(tft);
    m.setColorDepth(8);
    m.createSprite(width, height);
    m.fillSprite(TFT_WHITE);
    if(font != nullptr) {
      m.loadFont(font);
    } else {
      m.setTextFont(1);
    }
    m.setTextSize(fontSize);
    m.setTextDatum(datum);
    m.setTextColor(TFT_BLACK);
    double sx = 0;
    if(datum == TC_DATUM){
      sx = width / 2;
    }
    m.drawString(msg, sx, 0);
    m.pushSprite(x, y);
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

bool again = false;
  void drawBeam() {
    if (laser->laserState == HIGH){
      if(laser->laserVal > 0){
        if(lastState == 2){
          return;
        }
        drawBeamActive();
        lastState = 2;
      } else {
        if(lastState == 1){
          return;
        }
        drawBeamInactive();
        lastState = 1;
      }
    } else {
      if(lastState == 0) {
        if(again) {
            return;
          }
          again = true;
      }
      drawBeamNoPower();
      lastState = 0;
    }
  }

  void drawBeamActive() {
    logln("beam active");
    pngType = BEAM_PNG;
    //int16_t rc = drawImage(laser_beam, sizeof(laser_beam), beamX, beamY);
    int16_t rc = pngDecoder->openFLASH((uint8_t *)laser_beam, sizeof(laser_beam), pngDraw);
    endPng(rc);
  }
  
  void drawBeamInactive() {
    pngType = BEAM_INACTIVE_PNG;
    //int16_t rc = drawImage(laser_beam_inactive,  sizeof(laser_beam_inactive), beamX, beamY);
    int16_t rc = pngDecoder->openFLASH((uint8_t *)laser_beam_inactive, sizeof(laser_beam_inactive), pngDraw);
    endPng(rc);
  }

  void drawBeamNoPower() {
    pngType = BEAM_BW_PNG;
    //int16_t rc = drawImage(laser_beam_bw,  sizeof(laser_beam_bw), beamX, beamY);
    int16_t rc = pngDecoder->openFLASH((uint8_t *)laser_beam_bw, sizeof(laser_beam_bw), pngDraw);
    endPng(rc);
  }

  static void pngDraw(PNGDRAW *pDraw) {
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];          // Line buffer for rendering
    uint8_t  maskBuffer[1 + MAX_IMAGE_WIDTH / 8];  // Mask buffer
    //logln(String("PngDraw");
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
    } else {
      Serial.println('failed to get alpha mask!');
    }
  }

  void endPng(int16_t rc) {
    if (rc == PNG_SUCCESS) {
      tft->startWrite();
      rc = pngDecoder->decode(NULL, 0);
      tft->endWrite();
      tft->endWrite();     
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
// todo: a shared sprite for the whole screen might be a better option but this is pretty flaky
 /*
 int16_t drawImage(const byte img[], size_t imgSize, int x, int y) {
    logln("draw image");
    //logln(esp_get_free_heap_size());
    int16_t rc = pngDecoder->openFLASH((uint8_t *)img, imgSize, pngDrawSprite);
    log("result ");
    logln(rc);
    return rc;
  }
 static void pngDrawSprite(PNGDRAW *pDraw) {
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];          // Line buffer for rendering
    uint8_t  maskBuffer[1 + MAX_IMAGE_WIDTH / 8];  // Mask buffer
    logln("PngDrawSprite");
    AppUi::instance->pngDecoder->getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    if (AppUi::instance->pngDecoder->getAlphaMask(pDraw, maskBuffer, 255)) {
      int16_t x = 0;
      int16_t y = 0;
      int16_t h = 60;
      int16_t w = 60;
      switch(pngType){
        case SPLASH_PNG:
        x = 0;
        y = 0;
        h = 135;
        w = 240;
        break;
        case LOGO_PNG:
        x = logoX;
        y = logoY;
        h = logoH;
        w = logoW;
        break;
        case BEAM_PNG:
        case BEAM_BW_PNG:
        case BEAM_INACTIVE_PNG:
        x = beamX;
        y = beamY;
        h = beamH;
        w = beamW;
        break;
        case GEAR_PNG:
        default:
        x = gearX;
        y = gearY;
        break;
      }
        String message("pushmask at x: ");
        message.concat(x);
        message.concat(" y: ");
        message.concat(y + pDraw->y);
        logln(message);
        AppUi::instance->pushMaskedImageToSprite(0, pDraw->y, pDraw->iWidth, 1, lineBuffer, maskBuffer, x, y, w, h);
      //}
    } else {
      Serial.println('failed to get alpha mask!');
    }
  }

  void pushMaskedImageToSprite(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *img, uint8_t *mask, int posX, int posY, int sw, int sh)
  {
    String msg = String("Push at x: ");
    msg.concat(posX);
    msg.concat(" y: ");
    msg.concat(posY);
    msg.concat(" w: ");
    msg.concat(sw);
    msg.concat(" h: ");
    msg.concat(sh);
    logln(msg);
    TFT_eSprite m = TFT_eSprite(tft);
    logln("sprite created");
    m.setColorDepth(8);
    m.createSprite(sw, sh);
    m.fillSprite(TFT_WHITE);

    uint8_t  *mptr = mask;
    uint8_t  *eptr = mask + ((w + 7) >> 3);
    uint16_t *iptr = img;
    uint32_t setCount = 0;
    logln("start loop");
    // For each line in the image
    while (h--) {
      uint32_t xp = 0;
      uint32_t clearCount = 0;
      uint8_t  mbyte= *mptr++;
      uint32_t bits  = 8;
      // Scan through each byte of the bitmap and determine run lengths
      do {
        setCount = 0;

        //Get run length for clear bits to determine x offset
        while ((mbyte & 0x80) == 0x00) {
          // Check if remaining bits in byte are clear (reduce shifts)
          if (mbyte == 0) {
            clearCount += bits;      // bits not always 8 here
            if (mptr >= eptr) break; // end of line
            mbyte = *mptr++;
            bits  = 8;
            continue;
          }
          mbyte = mbyte << 1; // 0's shifted in
          clearCount ++;
          if (--bits) continue;;
          if (mptr >= eptr) break;
          mbyte = *mptr++;
          bits  = 8;
        }

        //Get run length for set bits to determine render width
        while ((mbyte & 0x80) == 0x80) {
          // Check if all bits are set (reduces shifts)
          if (mbyte == 0xFF) {
            setCount += bits;
            if (mptr >= eptr) break;
            mbyte = *mptr++;
            //bits  = 8; // NR, bits always 8 here unless 1's shifted in
            continue;
          }
          mbyte = mbyte << 1; //or mbyte += mbyte + 1 to shift in 1's
          setCount ++;
          if (--bits) continue;
          if (mptr >= eptr) break;
          mbyte = *mptr++;
          bits  = 8;
        }

        // A mask boundary or mask end has been found, so render the pixel line
        if (setCount) {
          xp += clearCount;
          clearCount = 0;
          String msg("pushing at: ");
          msg.concat((x + xp));
          msg.concat(" y: ");
          msg.concat(y);
          Serial.println(msg);
          m.pushImage(x + xp, y, setCount, 1, iptr + xp);      // pushImage handles clipping
          xp += setCount;
        }
      } while (setCount || mptr < eptr);

      y++;
      iptr += w;
      eptr += ((w + 7) >> 3);
    }
   //m.pushSprite(posX, posY);
   //pngDecoder->decode(NULL, 0);
   endSprite(0, &m, posX, posY);
   //m.deleteSprite();
   logln("Pushed");
  }
  

  void endSprite(int16_t rc, TFT_eSprite *sprite, int x, int y) {
     uint16_t pngw = 0, pngh = 0; // To store width and height of image
    if (rc == PNG_SUCCESS) {
      logln("Successfully opened png file");
      pngw = pngDecoder->getWidth();
      pngh = pngDecoder->getHeight();
      logf("Image metrics: (%d x %d), %d bpp, pixel type: %d\n", pngw, pngh, pngDecoder->getBpp(), pngDecoder->getPixelType());
      rc = pngDecoder->decode(NULL, 0);
      //delay(100);
    }
    sprite->pushSprite(x,y);
    sprite->deleteSprite();
  }*/
