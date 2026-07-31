#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ============================================================
// EVA ARCHIVE: NODE 2 // MAIN TERMINAL LOCK
// ============================================================

// ---------------- OLED (SH1106) ----------------
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int OLED_RESET = -1;
constexpr uint8_t OLED_ADDR = 0x3C;
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------- RC522 RFID ----------------
constexpr uint8_t RFID_SS_PIN = 5;
constexpr uint8_t RFID_RST_PIN = 27;
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

// ---------------- GAME PASSWORD LOGIC ----------------
constexpr byte TOKEN_BLOCK = 4;
const char CARD_TOKEN[] = "123456"; 
constexpr byte TOKEN_LENGTH = 6;

// The reward shown on the screen when they win
const char* NEXT_ROOM = "Room No: 3";
const char* NEXT_CODE = "New password = TANCEI";

// ---------------- TIMERS ----------------
constexpr unsigned long MESSAGE_HOLD_MS = 4000; // Shows result for 4 seconds
unsigned long messageTimestamp = 0;
bool showingResult = false;

// ============================================================
// DISPLAY SCREENS
// ============================================================

void showIdleScreen() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  
  display.setCursor(0, 0);
  display.println(F("EVA_AI // OFFLINE"));
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SH110X_WHITE);
  
  display.setCursor(0, 16); display.println(F("CRITICAL LOCKDOWN"));
  display.setCursor(0, 26); display.println(F("Admin token missing."));
  display.setCursor(0, 36); display.println(F("Facility doors LOCKED."));
  
  display.setCursor(0, 48); display.println(F("Awaiting programmed"));
  display.setCursor(0, 56); display.println(F("recovery card..."));
  
  display.display();
}

// NEW ERROR SCREEN FOR UNFORMATTED TAGS OR BAD SCANS
void showReadErrorScreen() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  
  display.setCursor(0, 0);
  display.println(F("EVA_AI // ERROR"));
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SH110X_WHITE);
  
  display.setCursor(0, 24); display.println(F("READ FAILED"));
  display.setCursor(0, 40); display.println(F("Wrong tag,"));
  display.setCursor(0, 52); display.println(F("or try again."));
  display.display();
  
  showingResult = true;
  messageTimestamp = millis();
}

// ERROR SCREEN FOR THE WRONG PASSWORD
void showRejectedScreen(int matched) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  
  display.setCursor(0, 0);
  display.println(F("EVA_AI // ERROR"));
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SH110X_WHITE);
  
  display.setCursor(0, 18); display.println(F("ACCESS DENIED"));
  display.setCursor(0, 30); display.println(F("Invalid Token Data."));
  
  // Show them how many characters they got right
  display.setCursor(0, 44); display.print(F("Valid Bytes: "));
  display.print(matched); display.print(F("/")); display.println(TOKEN_LENGTH);
  
  display.setCursor(0, 56); display.println(F("Rewrite Block 4."));
  display.display();
  
  showingResult = true;
  messageTimestamp = millis();
}

// SUCCESS SCREEN
void showSuccessScreen() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  
  display.setCursor(0, 0);
  display.println(F("EVA_AI // ONLINE"));
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SH110X_WHITE);
  
  display.setCursor(0, 16); display.println(F("LOCKDOWN OVERRIDDEN"));
  display.setCursor(0, 26); display.println(F("Welcome, Admin."));
  
  display.setCursor(0, 42); display.println(NEXT_ROOM);
  display.setCursor(0, 52); display.println(NEXT_CODE);
  display.display();
  
  showingResult = true;
  messageTimestamp = millis();
}

// ============================================================
// RFID READING LOGIC
// ============================================================

int matchingTokenBytes(const byte* data) {
  int matched = 0;
  for (byte i = 0; i < TOKEN_LENGTH; i++) {
    if (data[i] == static_cast<byte>(CARD_TOKEN[i])) matched++;
  }
  return matched;
}

bool readTokenBlock(byte* data, int& matched) {
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF; // Factory default key

  // Authenticate Block 4
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, TOKEN_BLOCK, &key, &(mfrc522.uid)
  );
  if (status != MFRC522::STATUS_OK) return false;

  // Read Block 4
  byte buffer[18];
  byte bufferSize = sizeof(buffer);
  status = mfrc522.MIFARE_Read(TOKEN_BLOCK, buffer, &bufferSize);
  if (status != MFRC522::STATUS_OK) return false;

  memcpy(data, buffer, 16);
  matched = matchingTokenBytes(data);
  return true;
}

// ============================================================
// SETUP & LOOP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(400);

  Wire.begin(21, 22);
  delay(1000); 
  
  if (!display.begin(OLED_ADDR, true)) {
    Serial.println(F("ERROR: OLED initialization failed."));
    while (true) delay(1000);
  }
  
  display.clearDisplay();
  display.display();

  SPI.begin();
  mfrc522.PCD_Init();
  
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (version == 0x00 || version == 0xFF) {
    display.clearDisplay();
    display.setCursor(0, 24); display.println(F("RFID HARDWARE ERROR"));
    display.setCursor(0, 40); display.println(F("Check SPI wiring."));
    display.display();
    while (true) delay(1000);
  }

  showIdleScreen();
}

void loop() {
  // Return to idle screen after message timer ends
  if (showingResult && millis() - messageTimestamp >= MESSAGE_HOLD_MS) {
    showingResult = false;
    showIdleScreen();
  }

  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(20);
    return;
  }

  byte data[16];
  int matched = 0;
  
  if (!readTokenBlock(data, matched)) {
    Serial.println(F("Failed to read card memory. Wrong tag or read error."));
    showReadErrorScreen(); // <-- Triggers the new screen!
  } else if (matched == TOKEN_LENGTH) {
    Serial.println(F("Correct Admin Token presented!"));
    showSuccessScreen();
  } else {
    Serial.println(F("Incorrect Token Data."));
    showRejectedScreen(matched);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(350);
}