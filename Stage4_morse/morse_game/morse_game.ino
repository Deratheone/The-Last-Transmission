/*
  =====================================================================
  MORSE CODE ESCAPE ROOM PUZZLE  —  ESP32
  =====================================================================
  Hardware:
    - ESP32 Dev Module
    - 8x8 MAX7219 LED Matrix (MD_MAX72XX library)
    - TTP223 Capacitive Touch Sensor (digital SIG pin)
    - Active OR Passive Buzzer (switchable, see BUZZER_IS_PASSIVE below)
    - 0.96" SH1106 I2C OLED Display (128x64, driven via U8g2)

  -------------------------------------------------------------------
  WIRING TABLE
  -------------------------------------------------------------------
    MAX7219 Matrix        ESP32
    ---------------       -----------
    VCC                -> 5V (or 3V3 if your module supports it)
    GND                -> GND
    DIN                -> GPIO 23
    CS  (LOAD)         -> GPIO 5
    CLK                -> GPIO 18

    TTP223 Touch Sensor   ESP32
    ---------------       -----------
    VCC                -> 3V3
    GND                -> GND
    SIG (I/O / OUT)    -> GPIO 4

    Buzzer Module         ESP32
    ---------------       -----------
    VCC                -> 5V (active) or 3V3 (passive, check datasheet)
    GND                -> GND
    I/O / SIG          -> GPIO 25

    SH1106 OLED (I2C)     ESP32
    ---------------       -----------
    VCC                -> 3V3
    GND                -> GND
    SDA                -> GPIO 21
    SCL                -> GPIO 22

  -------------------------------------------------------------------
  LIBRARIES REQUIRED (install via Library Manager)
  -------------------------------------------------------------------
    - MD_MAX72XX      (by majicDesigns)
    - Bounce2         (by Thomas Fredericks)
    - U8g2             (by oliver / olikraus)
    - Wire, SPI, and ESP32 core LEDC functions are built in

  NOTE ON LEDC API:
    This sketch uses the classic ledcSetup()/ledcAttachPin()/ledcWriteTone()
    API found in ESP32 Arduino core 2.x. If you are on core 3.x, replace
    the three calls in setupBuzzer()/buzzerOn()/buzzerOff() with the newer
    ledcAttach(pin, freq, resolution) / ledcWriteTone(pin, freq) style
    (pin-based instead of channel-based). Everything else is unaffected.
  =====================================================================
*/

#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Bounce2.h>
#include <Wire.h>
#include <U8g2lib.h>

// =====================================================================
// -------------------------- PIN DEFINITIONS -------------------------
// =====================================================================
#define DIN_PIN      23     // MAX7219 DIN
#define CLK_PIN      18     // MAX7219 CLK
#define CS_PIN        5     // MAX7219 CS / LOAD

#define TOUCH_PIN     4     // TTP223 SIG

#define BUZZER_PIN   25     // Buzzer I/O

#define OLED_SDA_PIN 21     // SH1106 OLED SDA (I2C data)
#define OLED_SCL_PIN 22     // SH1106 OLED SCL (I2C clock)

// =====================================================================
// --------------------------- BUZZER TYPE ------------------------------
// Set to true for a PASSIVE buzzer (uses LEDC tone generation)
// Set to false for an ACTIVE buzzer (simple digitalWrite on/off, no tone)
// =====================================================================
#define BUZZER_IS_PASSIVE   true

#define BUZZER_LEDC_CHANNEL  0
#define BUZZER_LEDC_RES_BITS 8

// Tone frequencies used for passive buzzer feedback (ignored if active)
// A5 / E5 -- a pleasant musical third instead of a harsh flat beep
#define TONE_DOT_HZ    880    // A5 - short touch beep
#define TONE_DASH_HZ   659    // E5 - long touch beep

// =====================================================================
// ------------------------- MAX7219 MATRIX SETUP -----------------------
// If your matrix module doesn't light correctly, try changing the
// hardware type below to MD_MAX72XX::PAROLA_HW, GENERIC_HW, or ICSTATION_HW
// =====================================================================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   1

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DIN_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// =====================================================================
// --------------------------- OLED DISPLAY SETUP -------------------------
// SH1106 128x64 I2C display, driven via U8g2 (confirmed working driver
// for this exact module -- do not switch back to SSD1306/Adafruit_GFX)
// =====================================================================
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// =====================================================================
// ------------------------------ TIMING --------------------------------
// =====================================================================
const unsigned long DOT_MS          = 1000;   // broadcast dot ON time
const unsigned long DASH_MS         = 2000;   // broadcast dash ON time
const unsigned long ELEMENT_GAP_MS  = 1000;   // gap between elements of same letter
const unsigned long LETTER_GAP_MS   = 2000;   // gap between letters
const unsigned long WORD_GAP_MS     = 2000;   // gap after full word
const unsigned long REPEAT_GAP_MS   = 3000;   // extra pause before the whole word repeats

const unsigned long LONG_PRESS_MS       = 2000; // long press to enter password mode
const unsigned long TOUCH_DOT_MAX_MS    = 1000; // < this = dot, else dash
const unsigned long DIGIT_TIMEOUT_MS    = 2000; // release gap that ends a digit
const unsigned long FAILURE_HOLD_MS     = 3000; // wait after wrong password
const unsigned long SUCCESS_HOLD_MS     = 10000; // hold after correct password
const unsigned long FEEDBACK_FLASH_MS   =  300; // confirmation flash after release

// =====================================================================
// --------------------------- BITMAPS (8x8) -----------------------------
// Each byte = one row, MSB = leftmost column (or rightmost depending on
// module orientation -- flip bytes with a mirror if your matrix looks
// reversed left/right).
// =====================================================================
const byte BMP_BLANK[8] = {
  0b00000000,0b00000000,0b00000000,0b00000000,
  0b00000000,0b00000000,0b00000000,0b00000000
};

// Full matrix, all 64 LEDs -- used for broadcast DASH (long beep)
const byte BMP_FULL[8] = {
  0b11111111,0b11111111,0b11111111,0b11111111,
  0b11111111,0b11111111,0b11111111,0b11111111
};

// Middle 4x4 block, 16 LEDs -- used for broadcast DOT (short beep)
const byte BMP_MID_16[8] = {
  0b00000000,
  0b00000000,
  0b00111100,
  0b00111100,
  0b00111100,
  0b00111100,
  0b00000000,
  0b00000000
};

// Large filled dot -- used for live touch feedback during password entry
const byte BMP_LARGE_DOT[8] = {
  0b00000000,
  0b00111100,
  0b01111110,
  0b01111110,
  0b01111110,
  0b01111110,
  0b00111100,
  0b00000000
};

// Small dot -- confirmation symbol for a decoded DOT during password entry
const byte BMP_DOT_SMALL[8] = {
  0b00000000,0b00000000,
  0b00011000,0b00011000,
  0b00011000,0b00011000,
  0b00000000,0b00000000
};

// Horizontal dash -- confirmation symbol for a decoded DASH
const byte BMP_DASH[8] = {
  0b00000000,0b00000000,0b00000000,
  0b11111111,0b11111111,
  0b00000000,0b00000000,0b00000000
};

// Cross / X -- wrong password
const byte BMP_CROSS[8] = {
  0b10000001,
  0b01000010,
  0b00100100,
  0b00011000,
  0b00011000,
  0b00100100,
  0b01000010,
  0b10000001
};

// Smiley face -- correct password celebration
const byte BMP_SUCCESS[8] = {
  0b00111100,
  0b01000010,
  0b10100101,
  0b10000001,
  0b10100101,
  0b10011001,
  0b01000010,
  0b00111100
};

// =====================================================================
// --------------------------- MORSE LOOKUP -------------------------------
// Full alphabet + digit tables. Any valid letter or digit can be entered
// and will be recognized/displayed -- only the final PASSWORD comparison
// decides access granted/denied.
// =====================================================================
struct MorseLetter { char ch; const char* code; };
const MorseLetter MORSE_ALPHABET[] = {
  {'A',".-"},   {'B',"-..."}, {'C',"-.-."}, {'D',"-.."},  {'E',"."},
  {'F',"..-."}, {'G',"--."},  {'H',"...."}, {'I',".."},   {'J',".---"},
  {'K',"-.-"},  {'L',".-.."}, {'M',"--"},   {'N',"-."},   {'O',"---"},
  {'P',".--."}, {'Q',"--.-"}, {'R',".-."},  {'S',"..."},  {'T',"-"},
  {'U',"..-"},  {'V',"...-"}, {'W',".--"},  {'X',"-..-"}, {'Y',"-.--"},
  {'Z',"--.."}
};
const int NUM_MORSE_ALPHABET = sizeof(MORSE_ALPHABET) / sizeof(MorseLetter);

struct MorseDigit { char ch; const char* code; };
const MorseDigit MORSE_DIGITS_FULL[] = {
  {'0',"-----"}, {'1',".----"}, {'2',"..---"}, {'3',"...--"}, {'4',"....-"},
  {'5',"....."}, {'6',"-...."}, {'7',"--..."}, {'8',"---.."}, {'9',"----."}
};
const int NUM_MORSE_DIGITS_FULL = sizeof(MORSE_DIGITS_FULL) / sizeof(MorseDigit);

// =====================================================================
// ------------------------ BROADCAST EVENT TABLE -------------------------
// Pre-built non-blocking event sequence for the word "KEY":
//   K = - . -      E = .      Y = - . - -
// =====================================================================
struct MorseEvent { bool ledOn; unsigned long duration; };

const MorseEvent KEY_SEQUENCE[] = {
  // ---- K : - . - ----
  {true,  DASH_MS},          // K1 dash
  {false, ELEMENT_GAP_MS},
  {true,  DOT_MS},           // K2 dot
  {false, ELEMENT_GAP_MS},
  {true,  DASH_MS},          // K3 dash
  {false, LETTER_GAP_MS},    // letter gap K -> E
  // ---- E : . ----
  {true,  DOT_MS},           // E1 dot
  {false, LETTER_GAP_MS},    // letter gap E -> Y
  // ---- Y : - . - - ----
  {true,  DASH_MS},          // Y1 dash
  {false, ELEMENT_GAP_MS},
  {true,  DOT_MS},           // Y2 dot
  {false, ELEMENT_GAP_MS},
  {true,  DASH_MS},          // Y3 dash
  {false, ELEMENT_GAP_MS},
  {true,  DASH_MS},          // Y4 dash
  {false, WORD_GAP_MS},      // word gap
  {false, REPEAT_GAP_MS}     // extra pause, then repeat
};
const int KEY_SEQUENCE_LEN = sizeof(KEY_SEQUENCE) / sizeof(MorseEvent);

// =====================================================================
// ----------------------------- MELODIES ---------------------------------
// =====================================================================
struct ToneStep { int freq; unsigned long duration; };

const ToneStep WRONG_MELODY[] = {
  {800, 200}, {600, 200}, {400, 200}, {200, 300}
};
const int WRONG_MELODY_LEN = sizeof(WRONG_MELODY) / sizeof(ToneStep);

const ToneStep SUCCESS_MELODY[] = {
  {523, 150}, {659, 150}, {784, 150}, {1047, 300},
  {784, 100}, {1047, 400}
};
const int SUCCESS_MELODY_LEN = sizeof(SUCCESS_MELODY) / sizeof(ToneStep);

// Short confirmation chirp played the moment a long press is detected
const ToneStep LONGPRESS_MELODY[] = {
  {700, 120}, {1000, 150}
};
const int LONGPRESS_MELODY_LEN = sizeof(LONGPRESS_MELODY) / sizeof(ToneStep);

// Descending "cancel/reset" chirp played when the reset code (Morse for 5) is entered
const ToneStep RESET_MELODY[] = {
  {900, 100}, {600, 100}, {350, 150}
};
const int RESET_MELODY_LEN = sizeof(RESET_MELODY) / sizeof(ToneStep);

// =====================================================================
// ------------------------------- FSM ------------------------------------
// =====================================================================
enum SystemState {
  BROADCAST,
  PASSWORD_ENTRY,
  VERIFY_PASSWORD,
  SUCCESS,
  FAILURE
};
SystemState state = BROADCAST;

// Predefined password (6 characters, any letter/digit) -- change freely
#define PASSWORD_LENGTH 6
const char PASSWORD[PASSWORD_LENGTH] = {'t','i','m','e','i','s'};

// =====================================================================
// ---------------------------- GLOBAL VARIABLES ---------------------------
// =====================================================================
Bounce touchBounce = Bounce();

bool          touchActive        = false;
unsigned long touchStartTime     = 0;
unsigned long lastReleaseTime    = 0;

// Prevents the tail end of the long-press touch (and the moment right after
// it) from being misread as the first password character
bool          suppressTouch      = false;   // true until the long-press finger lifts
unsigned long passwordUnlockAt   = 0;       // real input accepted only after this time
const unsigned long POST_LONGPRESS_GAP_MS = 1000; // 1s gap after long press before entry

String        morseBuffer        = "";     // current character's dots/dashes
char          enteredPassword[PASSWORD_LENGTH];
int           digitIndex         = 0;

// Broadcast sequence playback state
int           broadcastEventIdx  = 0;
unsigned long broadcastEventStart = 0;

// Confirmation flash state (after releasing a touch during password entry)
bool          flashActive        = false;
unsigned long flashStart         = 0;
const byte*   flashBitmap        = BMP_BLANK;

// Melody playback state (used in FAILURE / SUCCESS states)
int           melodyIdx          = 0;
unsigned long melodyEventStart   = 0;
bool          melodyPlaying      = false;

// Failure / success hold timers
unsigned long stateEnterTime     = 0;

// Scrolling marquee state for the OLED "decrypt this..." message (BROADCAST only)
const char*   SCROLL_TEXT          = "DECODE THE NOISE AND LONG PRESS TO CONTINUE     ";
const unsigned long SCROLL_STEP_MS = 40;   // ms between each 1px scroll step
int           scrollX              = 0;
int           scrollTextWidthPx    = 0;
unsigned long lastScrollStep       = 0;
#define OLED_WIDTH_PX 128

// =====================================================================
// ------------------------------ SETUP -----------------------------------
// =====================================================================
void setup() {
  Serial.begin(115200);

  // ---- Matrix init ----
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 8); // brightness 0-15
  clearMatrix();

  // ---- Touch sensor init ----
  pinMode(TOUCH_PIN, INPUT);
  touchBounce.attach(TOUCH_PIN);
  touchBounce.interval(15); // ms debounce

  // ---- Buzzer init ----
  setupBuzzer();

  // ---- OLED init ----
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  oled.begin();
  oled.clearBuffer();
  oled.sendBuffer();

  // ---- Start in BROADCAST mode ----
  enterBroadcast();
}

// =====================================================================
// ------------------------------- LOOP ------------------------------------
// =====================================================================
void loop() {
  touchBounce.update();
  handleTouchInput();          // always monitors touch, regardless of state
  updateMelodyPlayback();      // advances any in-progress melody/chirp, any state

  switch (state) {
    case BROADCAST:
      playMorseWord();          // advances the non-blocking KEY sequence
      updateScrollMessage();    // advances the scrolling OLED marquee
      break;

    case PASSWORD_ENTRY:
      updateConfirmationFlash();
      checkDigitTimeout();
      break;

    case VERIFY_PASSWORD:
      verifyPassword();         // transitions immediately to SUCCESS/FAILURE
      break;

    case SUCCESS:
      if (millis() - stateEnterTime >= SUCCESS_HOLD_MS) {
        clearMatrix();
        buzzerOff();
        resetPasswordEntry();
        enterBroadcast();       // back to the decrypting/broadcast stage
      }
      break;

    case FAILURE:
      if (millis() - stateEnterTime >= FAILURE_HOLD_MS) {
        resetPasswordEntry();
        enterPasswordEntry();
      }
      break;
  }
}

// =====================================================================
// --------------------------- STATE TRANSITIONS ---------------------------
// =====================================================================
void enterBroadcast() {
  state = BROADCAST;
  broadcastEventIdx = 0;
  broadcastEventStart = millis();
  applyBroadcastEvent();
  startScrollMessage();
  suppressTouch = false;
  passwordUnlockAt = 0;
}

void enterPasswordEntry() {
  state = PASSWORD_ENTRY;
  clearMatrix();
  buzzerOff();
  lastReleaseTime = millis();
  oledShowPasswordProgress();
}

void enterVerify() {
  state = VERIFY_PASSWORD;
}

void enterSuccess() {
  state = SUCCESS;
  stateEnterTime = millis();
  showSuccess();
  startMelody(SUCCESS_MELODY, SUCCESS_MELODY_LEN);
  oledShowMessage("ARCHIVE 5", "--> RY2042");
}

void enterFailure() {
  state = FAILURE;
  stateEnterTime = millis();
  showFailure();
  startMelody(WRONG_MELODY, WRONG_MELODY_LEN);
  oledShowMessage("ACCESS", "DENIED");
}

void resetPasswordEntry() {
  digitIndex = 0;
  morseBuffer = "";
  for (int i = 0; i < PASSWORD_LENGTH; i++) enteredPassword[i] = 0;
}

// =====================================================================
// ------------------------ BROADCAST (STATE 1) ----------------------------
// =====================================================================
// Non-blocking playback of the KEY_SEQUENCE event table.
void playMorseWord() {
  unsigned long elapsed = millis() - broadcastEventStart;
  if (elapsed >= KEY_SEQUENCE[broadcastEventIdx].duration) {
    broadcastEventIdx = (broadcastEventIdx + 1) % KEY_SEQUENCE_LEN;
    broadcastEventStart = millis();
    applyBroadcastEvent();
  }
}

// Applies the LED + buzzer output for the current broadcast event
void applyBroadcastEvent() {
  bool on = KEY_SEQUENCE[broadcastEventIdx].ledOn;
  if (on) {
    bool isDash = KEY_SEQUENCE[broadcastEventIdx].duration >= DASH_MS;
    if (isDash) {
      showBitmap(BMP_FULL);      // long beep -> full matrix lit
      buzzerOn(TONE_DASH_HZ);
    } else {
      showBitmap(BMP_MID_16);    // short beep -> middle 16 LEDs lit
      buzzerOn(TONE_DOT_HZ);
    }
  } else {
    clearMatrix();
    buzzerOff();
  }
}

// =====================================================================
// --------------------------- TOUCH HANDLING -------------------------------
// =====================================================================
void handleTouchInput() {
  bool pressed = touchBounce.read() == HIGH; // TTP223 SIG is HIGH while touched

  // ---- Suppress the tail end of the long-press touch itself ----
  if (suppressTouch) {
    if (!pressed) {
      // The finger that triggered the long press has now actually lifted.
      // Start the 1s cooldown before real password input is accepted.
      suppressTouch = false;
      passwordUnlockAt = millis() + POST_LONGPRESS_GAP_MS;
    }
    return; // ignore everything else while the original touch is still down
  }

  // ---- Cooldown window after the long press, before entry starts ----
  if (state == PASSWORD_ENTRY && millis() < passwordUnlockAt) {
    return;
  }

  // ---- Press started ----
  if (pressed && !touchActive) {
    touchActive = true;
    touchStartTime = millis();
  }

  // ---- Currently held ----
  if (touchActive && pressed) {
    unsigned long heldFor = millis() - touchStartTime;

    // Long-press detection: only meaningful while broadcasting
    if (state == BROADCAST && heldFor >= LONG_PRESS_MS) {
      detectLongPress();
      touchActive = false; // consume this press so it isn't reused
      return;
    }

    // Live feedback while holding during password entry
    if (state == PASSWORD_ENTRY) {
      showBitmap(BMP_LARGE_DOT);
      buzzerOn(heldFor < TOUCH_DOT_MAX_MS ? TONE_DOT_HZ : TONE_DASH_HZ);
    }
  }

  // ---- Release ----
  if (!pressed && touchActive) {
    touchActive = false;
    unsigned long pressDuration = millis() - touchStartTime;
    lastReleaseTime = millis();

    if (state == PASSWORD_ENTRY) {
      buzzerOff();
      registerMorseElement(pressDuration);
    }
  }
}

// Called the moment a long press (>2s) is confirmed during BROADCAST
void detectLongPress() {
  clearMatrix();
  buzzerOff();
  resetPasswordEntry();
  enterPasswordEntry();
  suppressTouch = true; // ignore the still-held finger until it actually lifts
  startMelody(LONGPRESS_MELODY, LONGPRESS_MELODY_LEN); // audible confirmation
}

// Classifies one touch as dot/dash, appends to buffer, starts confirmation flash
void registerMorseElement(unsigned long duration) {
  char symbol;
  if (duration < TOUCH_DOT_MAX_MS) {
    symbol = '.';
    flashBitmap = BMP_DOT_SMALL;
  } else {
    symbol = '-';
    flashBitmap = BMP_DASH;
  }
  morseBuffer += symbol;

  flashActive = true;
  flashStart = millis();
  showBitmap(flashBitmap);
}

// Turns off the brief confirmation symbol after FEEDBACK_FLASH_MS
void updateConfirmationFlash() {
  if (flashActive && millis() - flashStart >= FEEDBACK_FLASH_MS) {
    flashActive = false;
    if (!touchActive) clearMatrix();
  }
}

// If enough silence has passed since the last release, the current digit
// is considered finished -> decode it and move to the next character.
void checkDigitTimeout() {
  if (touchActive) return;                 // still touching, nothing to finalize
  if (morseBuffer.length() == 0) return;    // nothing entered yet
  if (millis() - lastReleaseTime < DIGIT_TIMEOUT_MS) return; // still within same digit

  char decoded = decodeMorse(morseBuffer);

  // ---- Reset code: Morse "5" (.....) restarts password entry from scratch ----
  if (decoded == '5') {
    Serial.println("RESET code ('5') entered -- restarting password entry from scratch");
    morseBuffer = "";
    resetPasswordEntry();
    oledShowPasswordProgress();

    flashBitmap = BMP_CROSS;      // brief visual confirmation of the reset
    flashActive = true;
    flashStart = millis();
    showBitmap(flashBitmap);

    startMelody(RESET_MELODY, RESET_MELODY_LEN);
    return;
  }

  if (decoded != '?') {
    enteredPassword[digitIndex] = decoded;
    Serial.print("Decoded character ");
    Serial.print(digitIndex + 1);
    Serial.print("/");
    Serial.print(PASSWORD_LENGTH);
    Serial.print(": '");
    Serial.print(decoded);
    Serial.print("' from morse '");
    Serial.print(morseBuffer);
    Serial.println("'");
    digitIndex++;
    oledShowPasswordProgress();
  } else {
    // Invalid morse sequence: ignore and let user retry
    Serial.print("Invalid morse sequence '");
    Serial.print(morseBuffer);
    Serial.println("', character ignored -- try again");
  }
  morseBuffer = "";

  if (digitIndex >= PASSWORD_LENGTH) {
    Serial.println("Password entry complete, verifying...");
    enterVerify();
  }
}

// =====================================================================
// ------------------------------ DECODING ----------------------------------
// =====================================================================
char decodeMorse(String buffer) {
  for (int i = 0; i < NUM_MORSE_ALPHABET; i++) {
    if (buffer.equals(MORSE_ALPHABET[i].code)) {
      return tolower(MORSE_ALPHABET[i].ch); // lowercase to match PASSWORD[] entries
    }
  }
  for (int i = 0; i < NUM_MORSE_DIGITS_FULL; i++) {
    if (buffer.equals(MORSE_DIGITS_FULL[i].code)) {
      return MORSE_DIGITS_FULL[i].ch;
    }
  }
  return '?'; // no match -- not a valid morse sequence at all
}

// =====================================================================
// --------------------------- PASSWORD VERIFY -------------------------------
// =====================================================================
void verifyPassword() {
  bool match = true;
  Serial.print("Entered: ");
  for (int i = 0; i < PASSWORD_LENGTH; i++) Serial.print(enteredPassword[i]);
  Serial.print("  Expected: ");
  for (int i = 0; i < PASSWORD_LENGTH; i++) Serial.print(PASSWORD[i]);
  Serial.println();

  for (int i = 0; i < PASSWORD_LENGTH; i++) {
    if (enteredPassword[i] != PASSWORD[i]) {
      match = false;
      break;
    }
  }

  Serial.println(match ? "MATCH -> success" : "MISMATCH -> failure");
  if (match) {
    enterSuccess();
  } else {
    enterFailure();
  }
}

// =====================================================================
// ------------------------------- DISPLAYS ----------------------------------
// =====================================================================
void showSuccess() {
  showBitmap(BMP_SUCCESS);
}

void showFailure() {
  showBitmap(BMP_CROSS);
}

void showBitmap(const byte *bmp) {
  for (int row = 0; row < 8; row++) {
    mx.setRow(0, row, bmp[row]);
  }
}

void clearMatrix() {
  mx.clear();
}

// =====================================================================
// -------------------------------- OLED ---------------------------------
// =====================================================================

// Shows a one or two-line status message.
void oledShowMessage(const String &line1, const String &line2) {
  oled.clearBuffer();

  oled.setFont(u8g2_font_ncenB10_tr);
  oled.drawStr(0, 26, line1.c_str());

  if (line2.length() > 0) {
    oled.drawStr(0, 50, line2.c_str());
  }

  oled.sendBuffer();
}

// Overload for a single-line message
void oledShowMessage(const String &line1) {
  oledShowMessage(line1, "");
}

// Shows the characters decoded so far during password entry, with
// underscores marking characters not yet entered, e.g. "_ _" -> "A _" -> "A B"
void oledShowPasswordProgress() {
  oled.clearBuffer();

  oled.setFont(u8g2_font_ncenB08_tr);
  oled.drawStr(0, 12, "ENTER PASSWORD");

  String progress = "";
  for (int i = 0; i < PASSWORD_LENGTH; i++) {
    progress += (i < digitIndex) ? String(enteredPassword[i]) : String('_');
    progress += ' ';
  }

  oled.setFont(u8g2_font_ncenB14_tr);
  oled.drawStr(0, 45, progress.c_str());

  oled.sendBuffer();
}

// Resets and (re)starts the scrolling "decrypt this..." marquee. Call once
// whenever BROADCAST begins (including every repeat after a failed password).
void startScrollMessage() {
  oled.setFont(u8g2_font_ncenB08_tr);
  scrollTextWidthPx = oled.getStrWidth(SCROLL_TEXT);
  scrollX = OLED_WIDTH_PX;          // start just off the right edge
  lastScrollStep = millis();
}

// Non-blocking scroll step, call every loop() while state == BROADCAST.
// Runs independently of the Morse/buzzer timing so both animate together.
void updateScrollMessage() {
  if (millis() - lastScrollStep < SCROLL_STEP_MS) return;
  lastScrollStep = millis();

  scrollX--;
  if (scrollX < -scrollTextWidthPx) {
    scrollX = OLED_WIDTH_PX;        // wrap back to the right edge
  }

  oled.clearBuffer();
  oled.setFont(u8g2_font_ncenB08_tr);
  oled.drawStr(scrollX, 40, SCROLL_TEXT);
  oled.sendBuffer();
}

// =====================================================================
// -------------------------------- BUZZER ------------------------------------
// =====================================================================
void setupBuzzer() {
  if (BUZZER_IS_PASSIVE) {
    // ESP32 Arduino core 3.x: pin-based LEDC API (no explicit channel needed)
    ledcAttach(BUZZER_PIN, 1000, BUZZER_LEDC_RES_BITS);
    ledcWriteTone(BUZZER_PIN, 0);
  } else {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void buzzerOn(int freq) {
  if (BUZZER_IS_PASSIVE) {
    ledcWriteTone(BUZZER_PIN, freq);
  } else {
    digitalWrite(BUZZER_PIN, HIGH); // active buzzer: frequency ignored
  }
}

void buzzerOff() {
  if (BUZZER_IS_PASSIVE) {
    ledcWriteTone(BUZZER_PIN, 0);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// =====================================================================
// ------------------------------- MELODIES -----------------------------------
// Non-blocking melody playback, driven from loop() via updateMelodyPlayback()
// =====================================================================
const ToneStep* activeMelody = nullptr;
int             activeMelodyLen = 0;

void startMelody(const ToneStep melody[], int len) {
  activeMelody = melody;
  activeMelodyLen = len;
  melodyIdx = 0;
  melodyEventStart = millis();
  melodyPlaying = true;
  buzzerOn(activeMelody[0].freq);
}

void updateMelodyPlayback() {
  if (!melodyPlaying) return;

  if (millis() - melodyEventStart >= activeMelody[melodyIdx].duration) {
    melodyIdx++;
    if (melodyIdx >= activeMelodyLen) {
      buzzerOff();
      melodyPlaying = false;
      return;
    }
    melodyEventStart = millis();
    buzzerOn(activeMelody[melodyIdx].freq);
  }
}