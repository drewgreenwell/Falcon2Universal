// global ui variables. pngdec doesnt seem to handle coordinates
// todo use sprite
enum PngType { GEAR };
PngType pngType = GEAR;
int16_t gearX = 40;
int16_t gearY = 40;


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
        case GEAR:
        default:
        x = gearX;
        y = gearY;
        break;
      }
      // Note: pushMaskedImage is for pushing to the TFT and will not work pushing into a sprite
      tft.pushMaskedImage(x, y + pDraw->y, pDraw->iWidth, 1, lineBuffer, maskBuffer);
    }
  }