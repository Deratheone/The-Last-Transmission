#include <WiFi.h>
#include <esp_now.h>

// =====================================================
// Stage 1 - EVA's Last Beacon Receiver
// Receives the opening memory fragment without assuming fixed content.
// =====================================================

// Receive callback
void onDataRecv(const esp_now_recv_info_t *recvInfo, const uint8_t *incomingData, int len) {
  Serial.println("Transmission Received!");

  // Print sender MAC address
  Serial.print("Sender MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", recvInfo->src_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  // Safely copy received bytes into a local char buffer
  // Do not assume message content
  const int MAX_MSG_LEN = 250;
  char messageBuffer[MAX_MSG_LEN + 1];

  int copyLen = len;
  if (copyLen > MAX_MSG_LEN) {
    copyLen = MAX_MSG_LEN;
  }

  memcpy(messageBuffer, incomingData, copyLen);
  messageBuffer[copyLen] = '\0'; // ensure null-terminated for Serial printing

  Serial.println("Message:");
  Serial.println(messageBuffer);
  Serial.println("---------------------");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== EVA LAST BEACON: RECEIVER STARTING ===");

  // 1) ESP32 must be in Station mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  // 2) Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW initialization failed!");
    Serial.println("Restarting in 3 seconds...");
    delay(3000);
    ESP.restart();
  }

  // 3) Register receive callback
  if (esp_now_register_recv_cb(onDataRecv) != ESP_OK) {
    Serial.println("ERROR: Failed to register receive callback!");
    Serial.println("Restarting in 3 seconds...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("ESP-NOW receiver ready. Waiting for transmissions...");
}

void loop() {
  // Keep loop light; callbacks handle received data
  delay(10);
}
