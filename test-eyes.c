#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Eye layout (pixels)
const int EYE_W   = 44;
const int EYE_H   = 34;
const int EYE_GAP = 16;
const int RADIUS  = 12;
const int EYE_Y   = 26;   // top edge of the eyes

// Eyebrow layout
const int BROW_W     = 36;
const int BROW_THICK = 3;
const int BROW_Y     = 14;  // baseline height of the brows

// innerLift > 0: inner end higher (soft, friendly, worried)
// innerLift < 0: inner end lower (angry, determined)
void drawBrow(int x, int y, int w, int innerLift, bool leftSide) {
  int yLeft, yRight;
  if (leftSide) { yLeft = y; yRight = y - innerLift; }   // inner end is the right end
  else          { yLeft = y - innerLift; yRight = y; }   // inner end is the left end

  for (int t = 0; t < BROW_THICK; t++) {
    display.drawLine(x, yLeft + t, x + w, yRight + t, SSD1306_WHITE);
  }
}

void drawFace(int innerLift) {
  int totalW = EYE_W * 2 + EYE_GAP;
  int leftX  = (128 - totalW) / 2;
  int rightX = leftX + EYE_W + EYE_GAP;
  int browOffset = (EYE_W - BROW_W) / 2;   // centres each brow over its eye

  display.fillRoundRect(leftX,  EYE_Y, EYE_W, EYE_H, RADIUS, SSD1306_WHITE);
  display.fillRoundRect(rightX, EYE_Y, EYE_W, EYE_H, RADIUS, SSD1306_WHITE);

  drawBrow(leftX  + browOffset, BROW_Y, BROW_W, innerLift, true);
  drawBrow(rightX + browOffset, BROW_Y, BROW_W, innerLift, false);
}

void setup() {
  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();
  drawFace(3);   // gentle, friendly tilt
  display.display();
}

void loop() {}
