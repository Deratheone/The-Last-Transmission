#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// Stage 2 - EVA Archive: RFID Administrator Recovery
//
// Players recover a write credential from the smartboard, program it
// into Block 4 of a MIFARE Classic card, and present that card here.
// ============================================================

// ---------------- OLED ----------------
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int OLED_RESET = -1;
constexpr uint8_t OLED_ADDR = 0x3C;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------- RC522 ----------------
constexpr uint8_t RFID_SS_PIN = 5;
constexpr uint8_t RFID_RST_PIN = 27;
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

// MIFARE Classic sector 1, first data block. Do not use a trailer block.
constexpr byte TOKEN_BLOCK = 4;
const char CARD_TOKEN[] = "123456"; // six-digit credential revealed by the video
constexpr byte TOKEN_LENGTH = sizeof(CARD_TOKEN) - 1;

// This is Fragment 2 of the final master key and Stage 3's AP password.
const char* NEXT_ROOM = "Room No: 3";
const char* NEXT_CODE = "123456";

constexpr unsigned long MESSAGE_HOLD_MS = 3500;
unsigned long messageTimestamp = 0;
bool showingResult = false;

void drawHeader(const __FlashStringHelper* title) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(title);
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);
}

void showIdleScreen() {
  drawHeader(F("EVA ARCHIVE // NODE 2"));
  display.setCursor(0, 17); display.println(F("Admin memory locked."));
  display.setCursor(0, 31); display.println(F("Present programmed"));
  display.setCursor(0, 43); display.println(F("RFID recovery card."));
  display.setCursor(0, 55); display.println(F("Block 4 is checked."));
  display.display();
}

void showReadError(const String& line) {
  drawHeader(F("CARD READ ERROR"));
  display.setCursor(0, 20); display.println(line);
  display.setCursor(0, 38); display.println(F("Use MIFARE Classic"));
  display.setCursor(0, 50); display.println(F("and default key A."));
  display.display();
  showingResult = true;
  messageTimestamp = millis();
}

void showRejectedScreen(int matched) {
  drawHeader(F("EVA TOKEN REJECTED"));
  display.setCursor(0, 19); display.println(F("Wrong card data."));
  display.setCursor(0, 34); display.print(F("Correct bytes: "));
  display.print(matched); display.print('/'); display.println(TOKEN_LENGTH);
  display.setCursor(0, 50); display.println(F("Rewrite Block 4."));
  display.display();
  showingResult = true;
  messageTimestamp = millis();
}

void showSuccessScreen() {
  drawHeader(F("EVA MEMORY RECOVERED"));
  display.setCursor(0, 18); display.println(F("Administrator accepted."));
  display.setCursor(0, 34); display.println(NEXT_ROOM);
  display.setCursor(0, 50); display.print(F("Fragment: ")); display.println(NEXT_CODE);
  display.display();
  showingResult = true;
  messageTimestamp = millis();
}

String uidToString(const byte* buffer, byte bufferSize) {
  String uid;
  for (byte i = 0; i < bufferSize; i++) {
    if (buffer[i] < 0x10) uid += '0';
    uid += String(buffer[i], HEX);
    if (i + 1 < bufferSize) uid += ':';
  }
  uid.toUpperCase();
  return uid;
}

int matchingTokenBytes(const byte* data) {
  int matched = 0;
  for (byte i = 0; i < TOKEN_LENGTH; i++) {
    if (data[i] == static_cast<byte>(CARD_TOKEN[i])) matched++;
  }
  return matched;
}

bool readTokenBlock(byte* data, int& matched) {
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, TOKEN_BLOCK, &key, &(mfrc522.uid)
  );
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  byte buffer[18];
  byte bufferSize = sizeof(buffer);
  status = mfrc522.MIFARE_Read(TOKEN_BLOCK, buffer, &bufferSize);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Block read failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  memcpy(data, buffer, 16);
  matched = matchingTokenBytes(data);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println(F("\n=== STAGE 2: EVA RFID ARCHIVE ==="));

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("ERROR: OLED initialization failed."));
    while (true) delay(1000);
  }

  SPI.begin();
  mfrc522.PCD_Init();
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (version == 0x00 || version == 0xFF) {
    Serial.println(F("ERROR: RC522 not detected. Check wiring and 3.3 V power."));
    drawHeader(F("HARDWARE ERROR"));
    display.setCursor(0, 24); display.println(F("RC522 not found."));
    display.setCursor(0, 40); display.println(F("Check wiring."));
    display.display();
    while (true) delay(1000);
  }

  Serial.println(F("Write 123456 to MIFARE Classic Block 4."));
  Serial.println(F("The card must still use the factory default Key A: FF FF FF FF FF FF."));
  showIdleScreen();
}

void loop() {
  if (showingResult && millis() - messageTimestamp >= MESSAGE_HOLD_MS) {
    showingResult = false;
    showIdleScreen();
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(20);
    return;
  }

  Serial.print(F("Card UID: "));
  Serial.println(uidToString(mfrc522.uid.uidByte, mfrc522.uid.size));

  byte data[16];
  int matched = 0;
  if (!readTokenBlock(data, matched)) {
    showReadError(F("Cannot read Block 4."));
  } else if (matched == TOKEN_LENGTH) {
    Serial.println(F("Correct EVA administrator token."));
    showSuccessScreen();
  } else {
    Serial.print(F("Incorrect token. Matching bytes: "));
    Serial.print(matched); Serial.print('/'); Serial.println(TOKEN_LENGTH);
    showRejectedScreen(matched);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(350);
}
