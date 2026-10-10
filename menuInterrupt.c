#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// Fix 1: Add commas between enum items and a semicolon after currentMode
enum AppState {
  STATE_ANIMATION,
  STATE_MENU,
  STATE_STUDY_TIMER,
  STATE_BREATHING
};

AppState currentMode = STATE_ANIMATION;

// Define your button pin
const int BUTTON_PIN = 4; 

// Simple debounce timing variables
unsigned long lastPressTime = 0;
const unsigned long DEBOUNCE_DELAY = 200; // milliseconds

// ... (keep your existing eye layout, drawing functions, and drawFace here) ...

void setup() {
  Wire.begin(21, 22);
  Wire.setClock(400000);              
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  randomSeed(esp_random());

  // Configure button pin with internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  nextBlink  = millis() + 3000;
  nextGlance = millis() + 5000;
}

void eyes() {
  unsigned long now = millis();
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  // Start a blink when it's due
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
      oppenness = 1.0 - sin(PI * p);   
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

  // Global button check (triggers from any mode if pressed & debounced)
  if (digitalRead(BUTTON_PIN) == LOW && (now - lastPressTime) > DEBOUNCE_DELAY) {
    lastPressTime = now;
    
    // Cycle or jump modes when button is clicked
    if (currentMode == STATE_ANIMATION) {
      currentMode = STATE_MENU;
    } else {
      currentMode = STATE_ANIMATION; // Pressing it again exits back to eyes for now
    }
  }

  // State machine dispatcher
  switch (currentMode) {
    case STATE_ANIMATION:
      eyes();
      break; // Fix 2: Don't forget 'break', otherwise it falls through into the menu case!
      
    case STATE_MENU:
      // Code to draw your menu will go here
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
