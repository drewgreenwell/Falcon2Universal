// global ui variables todo: migrate
#define UI_FONT_SMALL NotoSansBold15      // smaller ui font (messaging)
#define UI_FONT_LARGE NotoSansMonoScb20   // larger UI font (header)
#define GREY 0xCCCCCC
#define MAX_IMAGE_WIDTH 240 // Sets rendering line buffer lengths
enum PngType { SPLASH_PNG, GEAR_PNG, LOGO_PNG, BEAM_PNG, BEAM_BW_PNG, BEAM_INACTIVE_PNG };
PngType pngType = GEAR_PNG;
int16_t gearX = 0;
int16_t gearY = 0;
int16_t logoX = 5;
int16_t logoY = 118;
int16_t logoH = 20;
int16_t logoW = 125;
int16_t beamX = 0;
int16_t beamY = 0;
//int16_t beamY = 47;
int16_t beamW = 240;
int16_t beamH = 53;
int16_t versionX = 190;
int16_t versionY = 125;
int16_t versionH = 20;
int16_t versionW = 50;

int16_t btn1X = 0;
int16_t btn1Y = 125;

int16_t alarmX = 80;
int16_t alarmY = 80;
int16_t alarmW = 120;
int16_t alarmH = 30;
int16_t bootingX = 90;
int16_t bootingY = 90;
int16_t bootingH = 30;
int16_t bootingW = 100;
int16_t modelX = 0;
int16_t modelY = 10;
int16_t modelH = 30;
int16_t modelW = 240;

