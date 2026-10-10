#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Application States
enum AppState {
  STATE_ANIMATION,
  STATE_MENU,
  STATE_STUDY_TIMER,
  STATE_BREATHING
};

AppState currentMode = STATE_ANIMATION;

// Button configuration
const int BUTTON_PIN = 4; // Connect button between GPIO 4 and GND
unsigned long lastPressTime = 0;
const unsigned long DEBOUNCE_DELAY = 200; // milliseconds

// Eye layout (pixels)
const int EYE_W   = 44;
const int EYE_H   = 34;
const int EYE_GAP = 16;
const int RADIUS  = 12;
const int EYE_CY  = 43;         // vertical centre of the eyes

// Eyebrow layout
const int BROW_W      = 32;     // line length, centre of one end to centre of the other
const int BROW_R      = 2;      // end radius; thickness is about 2 x this + 1
const int BROW_Y      = 12;     // baseline height of the brows
const int INNER_LIFT  = 0;      // >0 worried/sad, 0 neutral, <0 angry/determined

// Timing
const unsigned long FRAME_MS = 40;    // about 25 fps
const unsigned long BLINK_MS = 180;   // how long one blink takes

// Animation state variables
float lookX = 0, lookY = 0;           
float targetX = 0, targetY = 0;       
bool blinking = false;
unsigned long lastFrame = 0;
unsigned long blinkStart = 0;
unsigned long nextBlink;
unsigned long nextGlance;

// Draws a brow as a row of circles, giving rounded ends
void drawBrow(int x, int y, bool leftSide) {
  int yL, yR;
  if (leftSide) { yL = y; yR = y - INNER_LIFT; }        
  else          { yL = y - INNER_LIFT; yR = y; }        

  for (int i = 0; i <= BROW_W; i++) {
    float t = (float)i / BROW_W;
    int cx = x + BROW_R + i;
    int cy = (int)round(yL + (yR - yL) * t) + BROW_R;
    display.fillCircle(cx, cy, BROW_R, SSD1306_WHITE);
  }
}

// openness: 1.0 = fully open, 0.0 = closed
void drawFace(float lx, float ly, float openness) {
  int totalW     = EYE_W * 2 + EYE_GAP;
  int baseLeftX  = (128 - totalW) / 2;
  int baseRightX = baseLeftX + EYE_W + EYE_GAP;

  int eyeShiftX = (int)round(lx);
  int h = max(3, (int)round(EYE_H * openness));
  int y = EYE_CY - h / 2 + (int)round(ly);
  int r = min(RADIUS, h / 2);

  display.fillRoundRect(baseLeftX  + eyeShiftX, y, EYE_W, h, r, SSD1306_WHITE);
  display.fillRoundRect(baseRightX + eyeShiftX, y, EYE_W, h, r, SSD1306_WHITE);

  int browCentreOff = (EYE_W - (BROW_W + 2 * BROW_R)) / 2;
  int browShiftX    = (int)round(lx * 0.5);
  int browY         = BROW_Y + (int)round(ly * 0.5) + (int)round((1.0 - openness) * 2);

  drawBrow(baseLeftX  + browCentreOff + browShiftX, browY, true);
  drawBrow(baseRightX + browCentreOff + browShiftX, browY, false);
}

void setup() {
  Wire.begin(21, 22);
  Wire.setClock(400000);              
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  randomSeed(esp_random());

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  nextBlink  = millis() + 3000;
  nextGlance = millis() + 5000;
}

// Eye animation routine
void eyes() {
  unsigned long now = millis();
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  if (!blinking && now >= nextBlink) {
    blinking = true;
    blinkStart = now;
  }

  float openness = 1.0;
  if (blinking) {
    float p = (now - blinkStart) / (float)BLINK_MS;
    if (p >= 1.0) {
      blinking = false;
      nextBlink = (random(100) < 15) ? now + 120 : now + random(2500, 6000);
    } else {
      openness = 1.0 - sin(PI * p);   
    }
  }

  if (now >= nextGlance) {
    if (random(100) < 40) {            
      targetX = 0; targetY = 0;
    } else {
      targetX = random(-5, 6);
      targetY = random(-2, 3);
    }
    nextGlance = now + random(3000, 8000);
  }

  lookX += (targetX - lookX) * 0.25;
  lookY += (targetY - lookY) * 0.25;

  display.clearDisplay();
  drawFace(lookX, lookY, openness);
  display.display();
}

void loop() {
  unsigned long now = millis();

  // Check button press globally with debounce to toggle modes
  if (digitalRead(BUTTON_PIN) == LOW && (now - lastPressTime) > DEBOUNCE_DELAY) {
    lastPressTime = now;
    
    if (currentMode == STATE_ANIMATION) {
      currentMode = STATE_MENU;
    } else {
      currentMode = STATE_ANIMATION;
      
      // Reset timers so eyes pick up smoothly instead of snapping/offsetting
      lastFrame = millis();
      nextBlink = millis() + 2000;
      nextGlance = millis() + 3000;
    }
  }

  // State dispatcher
  switch (currentMode) {
    case STATE_ANIMATION:
      eyes();
      break;
      
    case STATE_MENU:
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(10, 20);
      display.print(F("> Menu Mode"));
      display.display();
      break;

    case STATE_STUDY_TIMER:
      break;
      
    case STATE_BREATHING:
      break;
  }
}
