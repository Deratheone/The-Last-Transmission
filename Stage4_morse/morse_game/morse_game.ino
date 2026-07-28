/*
  Stage 4 - EVA Morse Archive Decode & Transmit Challenge
  IEEE AP-S "The Last Transmission"

  Flow:
  1) Player enters the known 6-digit code using touch sensor in Morse.
  2) Dot = short press, Dash = long press.
  3) Pause between presses decides end-of-symbol.
  4) If each digit is correct -> green confirmation.
  5) After all 6 correct -> success melody on buzzer.
  6) Then LED transmits next 6-digit code in Morse.

  Beginner-safe notes:
  - Clear timing constants
  - Serial debug logs
  - Simple state machine
*/

#include <Arduino.h>

// ========================= PIN CONFIG =========================
constexpr uint8_t PIN_TOUCH  = 14;   // Touch sensor digital output
constexpr uint8_t PIN_BUZZER = 25;   // Passive buzzer
constexpr uint8_t PIN_LED_R  = 26;   // RGB LED Red
constexpr uint8_t PIN_LED_G  = 27;   // RGB LED Green
constexpr uint8_t PIN_LED_B  = 33;   // RGB LED Blue

// ========================= GAME CONFIG =========================
// Code to be entered by participants (from previous stage)
const char* REQUIRED_CODE = "123456";

// Code to transmit after success (next stage code)
const char* NEXT_CODE = "123456"; // Fragment 4 of the master key; next node is Room 5.
const char* NEXT_ROOM = "5";

// Morse timing (ms)
constexpr unsigned long DOT_DASH_THRESHOLD_MS = 260;   // < threshold = dot, >= threshold = dash
constexpr unsigned long SYMBOL_GAP_MS         = 700;   // gap after last tap => symbol complete
constexpr unsigned long DEBOUNCE_MS           = 35;

// Morse output timing
constexpr unsigned long MORSE_UNIT_MS = 170;  // dot duration
// dash = 3 units, symbol gap = 3 units, between digits = 7 units

// ========================= DATA STRUCTURES =========================
struct MorseMap {
  char digit;
  const char* morse;
};

const MorseMap MORSE_DIGITS[] = {
  {'0', "-----"},
  {'1', ".----"},
  {'2', "..---"},
  {'3', "...--"},
  {'4', "....-"},
  {'5', "....."},
  {'6', "-...."},
  {'7', "--..."},
  {'8', "---.."},
  {'9', "----."}
};

// ========================= STATE =========================
String currentSymbol = "";         // current entered dot/dash sequence for one digit
uint8_t codeIndex = 0;             // which digit user is currently entering
bool challengeCompleted = false;
bool nodeAwake = false;

// Touch tracking
bool lastTouchState = false;
unsigned long pressStartMs = 0;
unsigned long lastReleaseMs = 0;
unsigned long lastEdgeMs = 0;

// ========================= LED HELPERS =========================
void setRGB(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r ? HIGH : LOW);
  digitalWrite(PIN_LED_G, g ? HIGH : LOW);
  digitalWrite(PIN_LED_B, b ? HIGH : LOW);
}

void ledOff() {
  setRGB(false, false, false);
}

void blinkColor(bool r, bool g, bool b, int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    setRGB(r, g, b);
    delay(onMs);
    ledOff();
    delay(offMs);
  }
}

// ========================= BUZZER HELPERS =========================
void playTone(int freq, int durationMs) {
  tone(PIN_BUZZER, freq, durationMs);
  delay(durationMs + 20);
  noTone(PIN_BUZZER);
}

void playSuccessMelody() {
  // Simple uplifting "win" tone sequence
  const int notes[] = {523, 659, 784, 1047, 1319};  // C5 E5 G5 C6 E6
  const int durs[]  = {120, 120, 120, 180, 240};

  for (int i = 0; i < 5; i++) {
    playTone(notes[i], durs[i]);
  }
}

// ========================= MORSE HELPERS =========================
const char* digitToMorse(char d) {
  for (const auto& item : MORSE_DIGITS) {
    if (item.digit == d) return item.morse;
  }
  return "";
}

char morseToDigit(const String& symbol) {
  for (const auto& item : MORSE_DIGITS) {
    if (symbol == item.morse) return item.digit;
  }
  return '?'; // unknown symbol
}

void flashMorseSymbol(char symbol) {
  // Green LED used for transmission
  if (symbol == '.') {
    setRGB(false, true, false);
    delay(MORSE_UNIT_MS);                // dot = 1 unit
    ledOff();
    delay(MORSE_UNIT_MS);                // intra-symbol gap = 1 unit
  } else if (symbol == '-') {
    setRGB(false, true, false);
    delay(3 * MORSE_UNIT_MS);            // dash = 3 units
    ledOff();
    delay(MORSE_UNIT_MS);                // intra-symbol gap = 1 unit
  }
}

void transmitNextCodeInMorse() {
  Serial.println("EVA fragment restored. Next room: 5");
  Serial.println("Transmitting Room 5 and Fragment 04 via GREEN LED in Morse...");

  // Small lead-in
  blinkColor(false, true, false, 2, 120, 120);
  delay(400);

  // The first digit is the next room number. A placard can label this
  // transmission format as ROOM / ACCESS CODE for players.
  for (int i = 0; NEXT_ROOM[i] != '\0'; i++) {
    const char* morse = digitToMorse(NEXT_ROOM[i]);
    for (int j = 0; morse[j] != '\0'; j++) flashMorseSymbol(morse[j]);
    delay(7 * MORSE_UNIT_MS);
  }

  for (int i = 0; NEXT_CODE[i] != '\0'; i++) {
    const char* morse = digitToMorse(NEXT_CODE[i]);

    Serial.print("Digit ");
    Serial.print(NEXT_CODE[i]);
    Serial.print(" => ");
    Serial.println(morse);

    // Send each dot/dash
    for (int j = 0; morse[j] != '\0'; j++) {
      flashMorseSymbol(morse[j]);
    }

    // End of digit gap: 3 units total between symbols in one letter
    // Already gave 1 unit after final element, add 2 more.
    delay(2 * MORSE_UNIT_MS);

    // Additional separation between digits
    delay(4 * MORSE_UNIT_MS); // total with above becomes ~7 units
  }

  Serial.println("Morse transmission complete.");
  // Keep all LEDs ON to indicate mission handoff done
  setRGB(true, true, true);
}

// ========================= GAME FEEDBACK =========================
void showStartupSignal() {
  // A faint blue trace suggests an incoming damaged transmission.
  blinkColor(false, false, true, 2, 120, 120);
  ledOff();
}

void wakeMemoryNode() {
  if (nodeAwake) return;
  nodeAwake = true;
  Serial.println("Touch detected. EVA archive connection waking.");
  blinkColor(false, true, false, 1, 250, 80);
  playTone(330, 100);
  blinkColor(true, false, false, 2, 90, 70);
  Serial.println("Enter the recovered six-digit fragment in Morse.");
}

void showCorrectDigitFeedback() {
  // Green pulse for each correct digit
  blinkColor(false, true, false, 1, 250, 120);
}

void showWrongDigitFeedback() {
  // Red triple blink
  blinkColor(true, false, false, 3, 120, 90);
}

void showAllCompleteLED() {
  // All LEDs ON
  setRGB(true, true, true);
}

// ========================= INPUT PROCESSING =========================
void resetCurrentAttempt(bool fullReset) {
  currentSymbol = "";
  if (fullReset) {
    codeIndex = 0;
    Serial.println("Input reset to first digit.");
  }
}

void evaluateCurrentSymbol() {
  if (currentSymbol.length() == 0) return;

  char decoded = morseToDigit(currentSymbol);
  char expected = REQUIRED_CODE[codeIndex];

  Serial.print("Entered symbol: ");
  Serial.print(currentSymbol);
  Serial.print(" -> Decoded: ");
  Serial.print(decoded);
  Serial.print(" | Expected digit: ");
  Serial.println(expected);

  if (decoded == expected) {
    Serial.println("Correct digit!");
    showCorrectDigitFeedback();
    codeIndex++;
    currentSymbol = "";

    if (REQUIRED_CODE[codeIndex] == '\0') {
      // All 6 digits correct
      challengeCompleted = true;
      Serial.println("=== AUTHENTICATION COMPLETE ===");
      showAllCompleteLED();
      playSuccessMelody();
      delay(400);
      transmitNextCodeInMorse();
    } else {
      Serial.print("Proceed to next digit index: ");
      Serial.println(codeIndex);
    }
  } else {
    Serial.println("Wrong digit. Restarting from first digit.");
    showWrongDigitFeedback();
    resetCurrentAttempt(true);
  }
}

void handleTouchInput() {
  bool touchState = digitalRead(PIN_TOUCH) == HIGH; // adjust if module logic inverted
  unsigned long now = millis();

  // Debounced edge detect
  if (touchState != lastTouchState && (now - lastEdgeMs) > DEBOUNCE_MS) {
    lastEdgeMs = now;
    lastTouchState = touchState;

    if (touchState) {
      // Press started
      wakeMemoryNode();
      pressStartMs = now;
    } else {
      // Press released => determine dot or dash
      unsigned long pressDuration = now - pressStartMs;
      if (pressDuration < DOT_DASH_THRESHOLD_MS) {
        currentSymbol += ".";
        Serial.println("Input: DOT");
      } else {
        currentSymbol += "-";
        Serial.println("Input: DASH");
      }
      lastReleaseMs = now;

      // Small white blink to acknowledge each tap
      setRGB(true, true, true);
      delay(40);
      ledOff();
    }
  }

  // If user paused long enough after last release, symbol is complete
  if (!touchState && currentSymbol.length() > 0 && (now - lastReleaseMs) > SYMBOL_GAP_MS) {
    evaluateCurrentSymbol();
  }
}

// ========================= SETUP / LOOP =========================
void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_TOUCH, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);

  ledOff();
  showStartupSignal();

  Serial.println("\n=== Stage 4: EVA MORSE ARCHIVE ===");
  Serial.println("Instructions:");
  Serial.println("1) Use touch sensor as Morse key.");
  Serial.println("2) Short press = DOT, Long press = DASH.");
  Serial.println("3) Pause to finish one digit.");
  Serial.println("4) Enter the 6-digit memory fragment.");
  Serial.println("5) On success, Room 5's code transmits via GREEN LED.");

  Serial.print("Required code length: ");
  Serial.println(strlen(REQUIRED_CODE));
  Serial.println("System ready.");
}

void loop() {
  if (!challengeCompleted) {
    handleTouchInput();
  } else {
    // Completed state: keep alive
    delay(30);
  }
}
