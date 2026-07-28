#include <WiFi.h>
#include <esp_now.h>

// =====================================================
// Stage 1 - EVA's Last Beacon (ESP-NOW Broadcast)
// The opening fragment of EVA's damaged final transmission.
// =====================================================

// Broadcast MAC address: sends to all nearby ESP-NOW receivers
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Message to transmit every 2 seconds. This is Fragment 1 of the master key.
char txMessage[] =
  "EVA // LAST BEACON // ROOM 1\n"
  "MEMORY FRAGMENT 01: 123456\n"
  "PROCEED TO ROOM 2";

// Timing control
constexpr unsigned long TRANSMIT_INTERVAL_MS = 2000;
unsigned long lastTransmitTime = 0;

// Optional callback: tells us whether ESP-NOW send attempt succeeded
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Success");
  } else {
    Serial.println("Failed");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== EVA LAST BEACON: TRANSMITTER STARTING ===");

  // 1) ESP32 must be in Station mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // keep disconnected from AP
  delay(100);

  Serial.print("Transmitter MAC: ");
  Serial.println(WiFi.macAddress());

  // 2) Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW initialization failed!");
    Serial.println("Restarting in 3 seconds...");
    delay(3000);
    ESP.restart();
  }

  // 3) Register send callback
  esp_now_register_send_cb(onDataSent);

  // 4) Register broadcast peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;      // 0 = current channel
  peerInfo.encrypt = false;  // broadcast must be unencrypted
  peerInfo.ifidx = WIFI_IF_STA;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERROR: Failed to add broadcast peer!");
    Serial.println("Restarting in 3 seconds...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("ESP-NOW ready. Broadcasting every 2 seconds.");
}

void loop() {
  unsigned long now = millis();

  if (now - lastTransmitTime >= TRANSMIT_INTERVAL_MS) {
    // Send char[] message (including null terminator is okay for text)
    esp_err_t result = esp_now_send(
      broadcastAddress,
      (uint8_t *)txMessage,
      strlen(txMessage) + 1
    );

    if (result == ESP_OK) {
      Serial.println("Transmission Sent:");
      Serial.println(txMessage);
      Serial.println("---------------------");
    } else {
      Serial.print("ERROR: esp_now_send failed with code ");
      Serial.println(result);
    }

    lastTransmitTime = now;
  }
}
