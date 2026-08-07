#include <SPI.h>
#include <MFRC522.h>

#define RFID_RST_PIN    27      
#define RFID_SS_PIN     5          

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println(F("Scan a blank card to set the new 1-2-3-4-5-6 password..."));
}

void loop() {
  // Look for a card
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  MFRC522::MIFARE_Key defaultKey;
  for (byte i = 0; i < 6; i++) {
    defaultKey.keyByte[i] = 0xFF;
  }

  int trailerBlock = 7;

  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &defaultKey, &(mfrc522.uid));
  
  if (status != MFRC522::STATUS_OK) {
    Serial.println(F("Authentication failed. This card might already have a custom password."));
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(2000);
    return;
  }

  byte newTrailer[16] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Your NEW Key A (6 bytes)
    0xFF, 0x07, 0x80, 0x69,             // FACTORY ACCESS BITS (Crucial: Do not change)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // Default Key B (6 bytes)
  };

  // 4. Write the new trailer block to the card
  status = mfrc522.MIFARE_Write(trailerBlock, newTrailer, 16);
  
  if (status == MFRC522::STATUS_OK) {
    Serial.println(F("Success! Password changed to 0x01 0x02 0x03 0x04 0x05 0x06."));
    Serial.println(F("DO NOT run this card on this script again."));
  } else {
    Serial.print(F("Failed to write password: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  delay(5000); // Wait 5 seconds before allowing another scan
}