#include <TM1637Display.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_VCNL4040.h>

// ================== PINS ==================
#define CLK 3
#define DIO 5
#define BUTTON_PIN 7
#define LED_PIN 6
#define NUM_LEDS 8
#define BUZZER_PIN 9

// ================== OBJECTS ==================
TM1637Display display(CLK, DIO);
CRGB leds[NUM_LEDS];
Adafruit_VCNL4040 vcnl;

// ================== GLASS ==================
#define GLASS_ON_THRESHOLD   18
#define GLASS_OFF_THRESHOLD  10

bool glassPresent = false;

// ================== MODES ==================
enum GameMode { H1, H2 };
GameMode currentMode = H1;

// ================== STATES ==================
enum GameState {
  IDLE,
  COUNTDOWN,
  RANDOM_WAIT,
  GO,
  RUNNING,
  FINISHED,
  LOSE
};
GameState state = IDLE;

// ================== SOUND ==================
bool soundEnabled = true;
bool showSoundStatus = false;
unsigned long soundStatusUntil = 0;

// ================== BUTTON ==================
bool buttonPressed = false;
unsigned long buttonPressTime = 0;

// ================== TIMING ==================
unsigned long startTime = 0;
unsigned long elapsedTime = 0;
unsigned long lastBeatTime = 0;

// ================== COUNTDOWN ==================
int countdownValue = 3;
unsigned long lastCountdownTick = 0;
unsigned long countdownStartTime = 0;

// ================== RANDOM ==================
unsigned long randomDelay = 0;
unsigned long randomStart = 0;

// ================== FLAGS ==================
bool glassWasLifted = false;
bool winMelodyPlayed = false;
bool loseMelodyPlayed = false;

// ================== DISPLAY SEGMENTS ==================
const uint8_t SEG_H1[]   = { SEG_B|SEG_C|SEG_E|SEG_F|SEG_G, SEG_B|SEG_C, 0, 0 };
const uint8_t SEG_H2[]   = { SEG_B|SEG_C|SEG_E|SEG_F|SEG_G, SEG_A|SEG_B|SEG_D|SEG_E|SEG_G, 0, 0 };
const uint8_t SEG_GO[]   = { SEG_A|SEG_C|SEG_D|SEG_E|SEG_F, SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F, 0, 0 };
const uint8_t SEG_ON[]   = { SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F, SEG_C|SEG_E|SEG_G, 0, 0 };
const uint8_t SEG_OFF[]  = { SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F, SEG_A|SEG_E|SEG_F|SEG_G, SEG_A|SEG_E|SEG_F|SEG_G, 0 };
const uint8_t SEG_LOSE[] = { SEG_D|SEG_E|SEG_F, SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F, SEG_A|SEG_C|SEG_D|SEG_F|SEG_G, SEG_A|SEG_D|SEG_E|SEG_F|SEG_G };

// ================= SETUP =================
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  randomSeed(analogRead(A0));

  display.setBrightness(6);
  display.clear();

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  Wire.begin();
  vcnl.begin();
}

// ================= LOOP =================
void loop() {
  handleButton();
  updateGlassState();

  if (showSoundStatus) {
    if (millis() < soundStatusUntil) return;
    showSoundStatus = false;
  }

  switch (state) {

    case IDLE:
      display.setSegments(currentMode == H1 ? SEG_H1 : SEG_H2);
      setAll(currentMode == H1 ? CRGB::Blue : CRGB::Purple);
      break;

    case COUNTDOWN:
      if (millis() - countdownStartTime < 150) break;
      if (!glassPresent) { state = LOSE; break; }

      if (millis() - lastCountdownTick >= 1000) {
        lastCountdownTick = millis();
        if (countdownValue > 0) {
          display.showNumberDec(countdownValue);
          if (soundEnabled) tone(BUZZER_PIN, 600, 120);
          countdownValue--;
        } else {
          if (currentMode == H2) {
            randomDelay = random(500, 5000);
            randomStart = millis();
            state = RANDOM_WAIT;
          } else {
            state = GO;
          }
        }
      }
      break;

    case RANDOM_WAIT:
      if (!glassPresent) { state = LOSE; break; }
      if (millis() - randomStart >= randomDelay) state = GO;
      break;

    case GO:
      display.setSegments(SEG_GO);
      setAll(CRGB::Green);
      if (soundEnabled) tone(BUZZER_PIN, 1200, 200);
      delay(300);

      startTime = millis();
      lastBeatTime = millis();
      glassWasLifted = false;
      winMelodyPlayed = false;
      loseMelodyPlayed = false;
      state = RUNNING;
      break;

    case RUNNING:
      runHeartbeat();
      showTimeHundredths(millis() - startTime);

      if (!glassPresent) glassWasLifted = true;
      if (glassPresent && glassWasLifted) {
        elapsedTime = millis() - startTime;
        state = FINISHED;
      }
      break;

    case FINISHED:
      showTimeHundredths(elapsedTime);
      pulseWhite();
      if (!winMelodyPlayed && soundEnabled) {
        playWinMelody();
        winMelodyPlayed = true;
      }
      break;

    case LOSE:
      display.setSegments(SEG_LOSE);
      blinkRed();
      if (!loseMelodyPlayed && soundEnabled) {
        playLoseMelody();
        loseMelodyPlayed = true;
      }
      break;
  }
}

// ================= GLASS =================
void updateGlassState() {
  uint16_t prox = vcnl.getProximity();

  if (!glassPresent && prox >= GLASS_ON_THRESHOLD) {
    glassPresent = true;
  }
  else if (glassPresent && prox <= GLASS_OFF_THRESHOLD) {
    glassPresent = false;
  }
}

// ================= BUTTON =================
void handleButton() {
  bool pressed = digitalRead(BUTTON_PIN) == LOW;

  if (pressed && !buttonPressed) {
    buttonPressed = true;
    buttonPressTime = millis();
  }

  if (!pressed && buttonPressed) {
    unsigned long duration = millis() - buttonPressTime;
    buttonPressed = false;

    if (duration < 1000) {
      if (state == IDLE) {
        currentMode = (currentMode == H1) ? H2 : H1;
      }
      else if (state == FINISHED || state == LOSE) {
        countdownValue = 3;
        lastCountdownTick = millis();
        countdownStartTime = millis();
        winMelodyPlayed = false;
        loseMelodyPlayed = false;
        state = COUNTDOWN;
      }
    }
    else {
      if (state == IDLE) {
        countdownValue = 3;
        lastCountdownTick = millis();
        countdownStartTime = millis();
        state = COUNTDOWN;
      }
      else if (state == FINISHED || state == LOSE) {
        soundEnabled = !soundEnabled;
        display.setSegments(soundEnabled ? SEG_ON : SEG_OFF);
        showSoundStatus = true;
        soundStatusUntil = millis() + 1500;
      }
    }
  }
}

// ================= EFFECTS =================
void runHeartbeat() {
  unsigned long t = millis() - startTime;
  unsigned long interval =
    t < 1500 ? 650 :
    t < 3000 ? 420 :
    t < 4500 ? 260 : 160;

  if (millis() - lastBeatTime >= interval) {
    lastBeatTime = millis();
    if (soundEnabled) tone(BUZZER_PIN, 120, 60);
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t hue = 12 - i * 2;
    uint8_t bri = beatsin8(18 + i * 2, 70, 220);
    leds[i] = CHSV(hue, 255, bri);
  }
  FastLED.show();
}

void setAll(CRGB c) {
  fill_solid(leds, NUM_LEDS, c);
  FastLED.show();
}

void blinkRed() {
  static bool on = false;
  static unsigned long t = 0;
  if (millis() - t > 300) {
    t = millis();
    on = !on;
    setAll(on ? CRGB::Red : CRGB::Black);
  }
}

void pulseWhite() {
  static uint8_t b = 60;
  static int d = 1;
  b += d * 3;
  if (b > 200 || b < 40) d = -d;
  setAll(CRGB(b, b, b));
}

void showTimeHundredths(unsigned long ms) {
  int v = ((ms / 1000) % 100) * 100 + ((ms / 10) % 100);
  display.showNumberDecEx(v, 0b01000000);
}

// ================= SOUND =================
void playWinMelody() {
  tone(BUZZER_PIN, 880, 120); delay(140);
  tone(BUZZER_PIN, 1175, 120); delay(140);
  tone(BUZZER_PIN, 1568, 220);
}

void playLoseMelody() {
  tone(BUZZER_PIN, 600, 180); delay(200);
  tone(BUZZER_PIN, 400, 220);
}
