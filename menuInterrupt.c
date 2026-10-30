#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ---------- Application states ----------
enum AppState {
  STATE_ANIMATION,
  STATE_MENU,
  STATE_STUDY_TIMER,
  STATE_BREATHING
};

AppState currentMode = STATE_ANIMATION;
bool needsRedraw = true;

// ---------- Menu ----------
struct MenuItem {
  const char *label;
  AppState target;
};

MenuItem menuItems[] = {
  {"Study Timer", STATE_STUDY_TIMER},
  {"Breathing",   STATE_BREATHING},
  // add more here
};
const int MENU_COUNT   = sizeof(menuItems) / sizeof(menuItems[0]);
const int MENU_VISIBLE = 5;   // rows that fit under the header
const int MENU_ROW_H   = 10;
const int MENU_TOP_Y   = 14;

int menuIndex = 0;   // highlighted item
int menuTop   = 0;   // first visible item

// ---------- Buttons (each wired between the GPIO and GND) ----------
const int MENU_PIN   = 4;
const int UP_PIN     = 14;
const int DOWN_PIN   = 12;   // strapping pin: don't hold it low while powering up
const int SELECT_PIN = 13;

const unsigned long DEBOUNCE_DELAY = 30;  // ms, per button

struct Button {
  uint8_t pin;
  bool lastLevel;
  unsigned long lastChange;
};

Button btnMenu   = {MENU_PIN,   HIGH, 0};
Button btnUp     = {UP_PIN,     HIGH, 0};
Button btnDown   = {DOWN_PIN,   HIGH, 0};
Button btnSelect = {SELECT_PIN, HIGH, 0};

// Returns true once per press (falling edge; INPUT_PULLUP so LOW = pressed)
bool pressed(Button &b) {
  bool level = digitalRead(b.pin);
  unsigned long now = millis();
  if (level != b.lastLevel && (now - b.lastChange) > DEBOUNCE_DELAY) {
    b.lastChange = now;
    b.lastLevel = level;
    return level == LOW;
  }
  return false;
}

// ---------- Eye layout (pixels) ----------
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

// ---------- State switching ----------
void setMode(AppState next) {
  currentMode = next;
  needsRedraw = true;

  if (next == STATE_ANIMATION) {
    // Reset timers so eyes pick up smoothly instead of snapping
    lastFrame  = millis();
    nextBlink  = millis() + 2000;
    nextGlance = millis() + 3000;
  } else if (next == STATE_MENU) {
    menuIndex = 0;
    menuTop = 0;
  }
}

// ---------- Screens ----------
void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("Menu"));
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  for (int row = 0; row < MENU_VISIBLE; row++) {
    int i = menuTop + row;
    if (i >= MENU_COUNT) break;

    int y = MENU_TOP_Y + row * MENU_ROW_H;
    if (i == menuIndex) {
      // inverted bar for the selected item
      display.fillRect(0, y - 1, 128, MENU_ROW_H, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(4, y);
    display.print(menuItems[i].label);
  }

  // tiny scroll hints if there's more off-screen
  display.setTextColor(SSD1306_WHITE);
  if (menuTop > 0) {
    display.setCursor(120, 2);
    display.print('^');
  }
  if (menuTop + MENU_VISIBLE < MENU_COUNT) {
    display.setCursor(120, 56);
    display.print('v');
  }

  display.display();
}

void drawPlaceholder(const __FlashStringHelper *title) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(title);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 18);
  display.print(F("Coming soon"));
  display.display();
}

void menuMove(int dir) {
  menuIndex = (menuIndex + dir + MENU_COUNT) % MENU_COUNT;  // wraps around

  // keep the highlighted item inside the visible window
  if (menuIndex < menuTop) menuTop = menuIndex;
  if (menuIndex >= menuTop + MENU_VISIBLE) menuTop = menuIndex - MENU_VISIBLE + 1;
  needsRedraw = true;
}

// ---------- Arduino entry points ----------
void setup() {
  Wire.begin(21, 22);
  Wire.setClock(400000);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  randomSeed(esp_random());

  pinMode(MENU_PIN,   INPUT_PULLUP);
  pinMode(UP_PIN,     INPUT_PULLUP);
  pinMode(DOWN_PIN,   INPUT_PULLUP);
  pinMode(SELECT_PIN, INPUT_PULLUP);

  nextBlink  = millis() + 3000;
  nextGlance = millis() + 5000;
}

void loop() {
  // Poll every button exactly once per loop
  bool evMenu   = pressed(btnMenu);
  bool evUp     = pressed(btnUp);
  bool evDown   = pressed(btnDown);
  bool evSelect = pressed(btnSelect);

  // Menu button: anywhere -> menu, except from the menu itself -> eyes
  if (evMenu) {
    setMode(currentMode == STATE_MENU ? STATE_ANIMATION : STATE_MENU);
  }

  // State dispatcher
  switch (currentMode) {
    case STATE_ANIMATION:
      eyes();   // has its own frame limiter
      break;

    case STATE_MENU:
      if (evUp)     menuMove(-1);
      if (evDown)   menuMove(+1);
      if (evSelect) setMode(menuItems[menuIndex].target);
      if (needsRedraw) { drawMenu(); needsRedraw = false; }
      break;

    case STATE_STUDY_TIMER:
      if (needsRedraw) { drawPlaceholder(F("Study Timer")); needsRedraw = false; }
      break;

    case STATE_BREATHING:
      if (needsRedraw) { drawPlaceholder(F("Breathing")); needsRedraw = false; }
      break;
  }
}





































/*
 * The night has a thousand eyes, and the day but one;
 * yet the light of the bright world dies with the dying sun.
 * The mind has a thousand eyes, and the heart but one:
 * yet the light of a whole life dies when love is done.
 */
