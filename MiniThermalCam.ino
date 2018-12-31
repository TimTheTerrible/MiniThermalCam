#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "Adafruit_miniTFTWing.h"
#include <Adafruit_AMG88xx.h>
#include "Adafruit_Si7021.h"
#include "debugprint.h"

#define TFT_RST  -1    // we use the seesaw for resetting to save a pin
#define TFT_CS    8
#define TFT_DC    3
#define ABS_MINTEMP 0
#define ABS_MAXTEMP 80
#define MIN_TEMP_RANGE 10
#define DEFAULT_MINTEMP 15
#define DEFAULT_MAXTEMP 30
#define AMG_COLS 8
#define AMG_ROWS 8
#define INTERPOLATED_COLS 40
#define INTERPOLATED_ROWS 40
#define KEYPAD_INTERRUPT_INTERVAL 100000 // check every 100ms

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
Adafruit_miniTFTWing ss;
Adafruit_AMG88xx amg;

IntervalTimer keypadTimer;
uint32_t oldButtons;

float pixels[AMG_COLS * AMG_ROWS];

bool displayHold = false;
bool displayUnitsImperial = false;

int minTemp = DEFAULT_MINTEMP;
int maxTemp = DEFAULT_MAXTEMP;

const uint16_t camColors[] = {0x480F,
                              0x400F, 0x400F, 0x400F, 0x4010, 0x3810, 0x3810, 0x3810, 0x3810, 0x3010, 0x3010,
                              0x3010, 0x2810, 0x2810, 0x2810, 0x2810, 0x2010, 0x2010, 0x2010, 0x1810, 0x1810,
                              0x1811, 0x1811, 0x1011, 0x1011, 0x1011, 0x0811, 0x0811, 0x0811, 0x0011, 0x0011,
                              0x0011, 0x0011, 0x0011, 0x0031, 0x0031, 0x0051, 0x0072, 0x0072, 0x0092, 0x00B2,
                              0x00B2, 0x00D2, 0x00F2, 0x00F2, 0x0112, 0x0132, 0x0152, 0x0152, 0x0172, 0x0192,
                              0x0192, 0x01B2, 0x01D2, 0x01F3, 0x01F3, 0x0213, 0x0233, 0x0253, 0x0253, 0x0273,
                              0x0293, 0x02B3, 0x02D3, 0x02D3, 0x02F3, 0x0313, 0x0333, 0x0333, 0x0353, 0x0373,
                              0x0394, 0x03B4, 0x03D4, 0x03D4, 0x03F4, 0x0414, 0x0434, 0x0454, 0x0474, 0x0474,
                              0x0494, 0x04B4, 0x04D4, 0x04F4, 0x0514, 0x0534, 0x0534, 0x0554, 0x0554, 0x0574,
                              0x0574, 0x0573, 0x0573, 0x0573, 0x0572, 0x0572, 0x0572, 0x0571, 0x0591, 0x0591,
                              0x0590, 0x0590, 0x058F, 0x058F, 0x058F, 0x058E, 0x05AE, 0x05AE, 0x05AD, 0x05AD,
                              0x05AD, 0x05AC, 0x05AC, 0x05AB, 0x05CB, 0x05CB, 0x05CA, 0x05CA, 0x05CA, 0x05C9,
                              0x05C9, 0x05C8, 0x05E8, 0x05E8, 0x05E7, 0x05E7, 0x05E6, 0x05E6, 0x05E6, 0x05E5,
                              0x05E5, 0x0604, 0x0604, 0x0604, 0x0603, 0x0603, 0x0602, 0x0602, 0x0601, 0x0621,
                              0x0621, 0x0620, 0x0620, 0x0620, 0x0620, 0x0E20, 0x0E20, 0x0E40, 0x1640, 0x1640,
                              0x1E40, 0x1E40, 0x2640, 0x2640, 0x2E40, 0x2E60, 0x3660, 0x3660, 0x3E60, 0x3E60,
                              0x3E60, 0x4660, 0x4660, 0x4E60, 0x4E80, 0x5680, 0x5680, 0x5E80, 0x5E80, 0x6680,
                              0x6680, 0x6E80, 0x6EA0, 0x76A0, 0x76A0, 0x7EA0, 0x7EA0, 0x86A0, 0x86A0, 0x8EA0,
                              0x8EC0, 0x96C0, 0x96C0, 0x9EC0, 0x9EC0, 0xA6C0, 0xAEC0, 0xAEC0, 0xB6E0, 0xB6E0,
                              0xBEE0, 0xBEE0, 0xC6E0, 0xC6E0, 0xCEE0, 0xCEE0, 0xD6E0, 0xD700, 0xDF00, 0xDEE0,
                              0xDEC0, 0xDEA0, 0xDE80, 0xDE80, 0xE660, 0xE640, 0xE620, 0xE600, 0xE5E0, 0xE5C0,
                              0xE5A0, 0xE580, 0xE560, 0xE540, 0xE520, 0xE500, 0xE4E0, 0xE4C0, 0xE4A0, 0xE480,
                              0xE460, 0xEC40, 0xEC20, 0xEC00, 0xEBE0, 0xEBC0, 0xEBA0, 0xEB80, 0xEB60, 0xEB40,
                              0xEB20, 0xEB00, 0xEAE0, 0xEAC0, 0xEAA0, 0xEA80, 0xEA60, 0xEA40, 0xF220, 0xF200,
                              0xF1E0, 0xF1C0, 0xF1A0, 0xF180, 0xF160, 0xF140, 0xF100, 0xF0E0, 0xF0C0, 0xF0A0,
                              0xF080, 0xF060, 0xF040, 0xF020, 0xF800,
                             };

float get_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
void set_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y, float f);
void get_adjacents_1d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
void get_adjacents_2d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
float cubicInterpolate(float p[], float x);
float bicubicInterpolate(float p[], float x, float y);
void interpolate_image(float *src, uint8_t src_rows, uint8_t src_cols, 
                       float *dest, uint8_t dest_rows, uint8_t dest_cols);
                       
void setup() {
  delay(500);

  Serial.begin(115200);

  //while ( !Serial )
  //  delay(200);

  debugprint(DEBUG_INFO, "Mini Thermal Cam V0.2");

  // Set up the Seesaw
  if ( !ss.begin() ) {
    debugprint(DEBUG_ERROR, "seesaw init error!");
    while (true);
  }

  debugprint(DEBUG_INFO, "Seesaw initialized!");
  ss.tftReset();
  ss.setBacklight(0x0); //set the backlight fully on
  
  // Set up the miniTFT display
  tft.initR(INITR_MINI160x80);   // initialize a ST7735S chip, mini display
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  
  debugprint(DEBUG_INFO, "TFT initialized!");

  // Draw the splash screen
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.println("Mini Thermal Cam V0.1");

  // Set up the thermal sensor
  if ( !amg.begin() ) {
    debugprint(DEBUG_ERROR, "Couldn't find a valid AMG88xx sensor, check wiring!");
    tft.setTextColor(ST77XX_RED);
    tft.println("Couldn't find a valid AMG88xx sensor, check wiring!");
    while (1);
  }

  debugprint(DEBUG_INFO, "Thermal Sensor initialized!");
  tft.println("Thermal Sensor initialized!");

  // Set up the keypad timer interrupt...
  keypadTimer.begin(checkKeypad, KEYPAD_INTERRUPT_INTERVAL);

  // Let the boot screen show for a second...
  delay(1000);
  tft.fillScreen(ST77XX_BLACK);

  Debug = DEBUG_ERROR | DEBUG_WARN | DEBUG_INFO;
  //Debug = DEBUG_ALL;
}

void loop() {

  // Read the thermal sensor...
  debugprint(DEBUG_TRACE, "Reading pixels...");

  // Disable interrupts, as interrupting the read call
  // seems to introduce errors into the thermal image data...
  noInterrupts();
  amg.readPixels(pixels);
  interrupts();  

  float dest_2d[INTERPOLATED_ROWS * INTERPOLATED_COLS];

  int32_t t = millis();
  interpolate_image(pixels, AMG_ROWS, AMG_COLS, dest_2d, INTERPOLATED_ROWS, INTERPOLATED_COLS);
  debugprint(DEBUG_TRACE, "Interpolation took %d ms", millis() - t );

  uint16_t boxsize = min(tft.width() / INTERPOLATED_COLS, tft.height() / INTERPOLATED_COLS);
  
  // Update the screen...
  drawScale(get_point(dest_2d, INTERPOLATED_ROWS, INTERPOLATED_COLS, INTERPOLATED_ROWS / 2, INTERPOLATED_COLS / 2));
  drawPixels(dest_2d, INTERPOLATED_ROWS, INTERPOLATED_COLS, boxsize, boxsize);
  drawUI();
}

int units(int inDegrees, bool returnImperial) {
  
  if ( returnImperial ) {
    return (inDegrees * 9/5) + 32;
  }
  else {
    return inDegrees;
  }
}

void drawScale(int targetTemp) {

  for ( int row = tft.height(); row > 0; row-- ) {
    tft.drawFastHLine(0, row, 39, camColors[map(row, tft.height(), 0, 0, 255)]);
  }
  
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 5);
  tft.println(units(maxTemp, displayUnitsImperial));
  tft.setCursor(5, 35);
  tft.println(units(targetTemp, displayUnitsImperial));
  tft.setCursor(5, 65);
  tft.println(units(minTemp, displayUnitsImperial));
}

void drawUI() {
  /*      121                  159
   *    0 +--------------------+
   *      |    battery icon    |
   *   20 +--------------------+
   *      |        units       |
   *   40 +--------------------+
   *      |                    |
   *      |      disk icon     |
   *      |                    |
   *   79 +--------------------+
   */

  // draw a battery
  tft.drawRect(125, 5, 30, 10, ST77XX_WHITE);
  tft.fillRect(155, 7, 3, 5, ST77XX_WHITE);

  // Draw the units
  // Draw the old value in black to erase it,
  // then draw the new one in white.
  tft.setTextSize(2);
  if ( displayUnitsImperial ) {
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(135, 25);
    tft.println("C");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(135, 25);
    tft.println("F");
  }
  else {
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(135, 25);
    tft.println("F");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(135, 25);
    tft.println("C");
  }

}

void drawPixels(float *p, uint8_t rows, uint8_t cols, uint8_t boxWidth, uint8_t boxHeight) {

  int colorTemp;

  for ( int row = 0; row < rows; row++ ) {

    for ( int col = 0; col < cols; col++ ) {

      float val = get_point(p, rows, cols, row, col);

      if ( val >= maxTemp )
        colorTemp = maxTemp;
      else if ( val <= minTemp )
        colorTemp = minTemp;
      else
        colorTemp = val;
      
      uint8_t colorIndex = map(colorTemp, minTemp, maxTemp, 0, 255);
      colorIndex = constrain(colorIndex, 0, 255);

      //draw the pixels!
      if ( row == rows / 2 && col == cols / 2 ) {
        // we're drawing the target
        colorTemp = ST77XX_WHITE;
      }
      else {
        colorTemp = camColors[colorIndex];
      }
      tft.fillRect( 40 + boxWidth * col, boxHeight * row, boxWidth, boxHeight, colorTemp);
        
    } // next col
  } // next row
}

void checkKeypad() {

  uint32_t buttons = ss.readButtons();

  // Bail out if the same buttons are still down from last time...
  if ( buttons == oldButtons )
    return;

  if (! (buttons & TFTWING_BUTTON_LEFT)) {
    moveMidpoint(-1);
  }
  if (! (buttons & TFTWING_BUTTON_RIGHT)) {
    moveMidpoint(+1);
  }
  if (! (buttons & TFTWING_BUTTON_DOWN)) {
    moveRange(-1);
  }
  if (! (buttons & TFTWING_BUTTON_UP)) {
    moveRange(+1);
  }
  if (! (buttons & TFTWING_BUTTON_A)) {
    displayUnitsImperial = !displayUnitsImperial;
  }
  if (! (buttons & TFTWING_BUTTON_B)) {
    displayHold = !displayHold;
  }
  if (! (buttons & TFTWING_BUTTON_SELECT)) {
    resetRange();
  }

  oldButtons = buttons;
}

void moveMidpoint( int distance ) {
  
  if ( distance < 0 ) { // going down?
    if ( minTemp + distance >= ABS_MINTEMP )
      minTemp += distance;
    if ( maxTemp + distance >= ABS_MINTEMP + MIN_TEMP_RANGE )
      maxTemp += distance;
  }
  else if ( distance > 0 ) { // going up?
    if ( minTemp + distance <= ABS_MAXTEMP - MIN_TEMP_RANGE )
      minTemp += distance;
    if ( maxTemp + distance <= ABS_MAXTEMP )
      maxTemp += distance;
  }
  else { // going nowhere?!?
    debugprint(DEBUG_ERROR, "moveMidpoint(): Invalid distance: %d", distance);    
  }

  debugprint(DEBUG_INFO, "midpoint += %d\n", distance);
}

void moveRange( int distance ) {
  debugprint(DEBUG_INFO, "\nbefore: minTemp = %d; maxTemp = %d", minTemp, maxTemp);

  if ( distance < 0 ) { // reducing range?
    // Range-check the range...
    if ( (maxTemp + distance) - (minTemp - distance) >= MIN_TEMP_RANGE ) {
      minTemp -= distance;
      maxTemp += distance;
    }
    // range-check the endpoints?
  }
  else if ( distance > 0 ) { // increasing range?
    // Range-check the endpoints...
    if ( minTemp - distance >= ABS_MINTEMP and maxTemp + distance < ABS_MAXTEMP ) {
      minTemp -= distance;
      maxTemp += distance;
    }
    // Max range is only 80 degrees C, so no need to range-check the range
  }
  else { // then why did you wake me up?!?
    debugprint(DEBUG_ERROR, "moveRange(): Invalid distance: %d", distance);    
  }

  debugprint(DEBUG_INFO, "after: minTemp = %d; maxTemp = %d\n", minTemp, maxTemp);
}

void resetRange() {
  minTemp = DEFAULT_MINTEMP;
  maxTemp = DEFAULT_MAXTEMP;
  debugprint(DEBUG_INFO, "min and max reset to %d and %d\n", minTemp, maxTemp);
}
