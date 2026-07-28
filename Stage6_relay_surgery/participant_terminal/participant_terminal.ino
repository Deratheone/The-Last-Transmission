#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

// ============================================================
// Stage 6 - Participant ESP-NOW Recovery Terminal
//
// Set NODE_6_MAC to the address printed by the Node 6 ESP32. Open Serial
// Monitor at 115200, enter the six-digit Stage 5 access key, then press Send.
// ============================================================

constexpr uint8_t WIFI_CHANNEL = 1;

// Replace all six bytes with Node 6's station MAC address.
uint8_t NODE_6_MAC[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

struct Packet {
  char type[16];
  char payload[64];
};

String serialLine;

bool nodeMacConfigured() {
  for (byte i = 0; i < 6; i++) {
    if (NODE_6_MAC[i] != 0x00) return true;
  }
  return false;
}

void onDataSent(const wifi_tx_info_t*, esp_now_send_status_t status) {
  Serial.print(F("Request delivery: "));
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? F("confirmed") : F("failed"));
}

void onDataRecv(const esp_now_recv_info_t*, const uint8_t* data, int len) {
  if (len != sizeof(Packet)) {
    Serial.println(F("Ignored malformed reply."));
    return;
  }

  Packet reply = {};
  memcpy(&reply, data, sizeof(reply));
  reply.type[sizeof(reply.type) - 1] = '\0';
  reply.payload[sizeof(reply.payload) - 1] = '\0';

  Serial.println(F("\n=== NODE 6 RESPONSE ==="));
  Serial.print(F("Type: ")); Serial.println(reply.type);
  Serial.print(F("Message: ")); Serial.println(reply.payload);
  Serial.println(F("======================="));
}

void sendRecoveryRequest(const String& key) {
  if (key.length() != 6) {
    Serial.println(F("Enter exactly six digits."));
    return;
  }

  Packet request = {};
  strlcpy(request.type, "EVA_RESTORE", sizeof(request.type));
  key.toCharArray(request.payload, sizeof(request.payload));

  esp_err_t result = esp_now_send(NODE_6_MAC, reinterpret_cast<uint8_t*>(&request), sizeof(request));
  if (result != ESP_OK) {
    Serial.print(F("ERROR: could not send request. ESP-NOW code: "));
    Serial.println(result);
  }
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println(F("\n=== EVA RECOVERY TERMINAL ==="));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (!nodeMacConfigured()) {
    Serial.println(F("ERROR: Set NODE_6_MAC before uploading this sketch."));
    while (true) delay(1000);
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ERROR: ESP-NOW initialization failed."));
    while (true) delay(1000);
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, NODE_6_MAC, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println(F("ERROR: Could not register Node 6 as a peer."));
    while (true) delay(1000);
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  Serial.println(F("Terminal ready. Enter the Stage 5 six-digit key and press Send."));
}

void loop() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (serialLine.length() > 0) {
        serialLine.trim();
        sendRecoveryRequest(serialLine);
        serialLine = "";
      }
    } else {
      serialLine += c;
    }
  }
}
