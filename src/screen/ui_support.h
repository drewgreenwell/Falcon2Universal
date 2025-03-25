// global ui variables todo: migrate
// todo use sprites
#define MAX_IMAGE_WIDTH 240 // Sets rendering line buffer lengths, adjust for your images
enum PngType { SPLASH_PNG, GEAR_PNG, LOGO_PNG, BEAM_PNG, BEAM_BW_PNG, BEAM_INACTIVE_PNG };
PngType pngType = GEAR_PNG;
int16_t gearX = 40;
int16_t gearY = 40;
int16_t logoX = 0;
int16_t logoY = 115;
int16_t logoH = 20;
int16_t logoW = 125;
int16_t beamX = 0;
int16_t beamY = 47;

  //=========================================v==========================================
  //  pngDraw: Callback function to draw pixels to the display
  //====================================================================================
  // This function will be called during decoding of the png file to render each image
  // line to the TFT. PNGdec generates the image line and a 1bpp mask.
  void pngDraw(PNGDRAW *pDraw) {
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];          // Line buffer for rendering
    uint8_t  maskBuffer[1 + MAX_IMAGE_WIDTH / 8];  // Mask buffer

    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    if (png.getAlphaMask(pDraw, maskBuffer, 255)) {
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
      tft.pushMaskedImage(x, y + pDraw->y, pDraw->iWidth, 1, lineBuffer, maskBuffer);
    }
  }