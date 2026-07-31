#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

// ============================================================
// Participant Unit
//
// Listens for the desk unit's broadcast prompt (no MAC address
// setup needed). Once the team figures out the access code, type
// it into the Serial Monitor and press Enter — it's broadcast
// back the same way. If correct, the desk unit broadcasts the
// final reward, which this unit prints.
// ============================================================

constexpr uint8_t WIFI_CHANNEL = 1;
uint8_t BROADCAST_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct Packet {
  char type[16];
  char payload[64];
};

String serialLine;

void sendPacket(const char* type, const char* payload) {
  Packet pkt = {};
  strlcpy(pkt.type, type, sizeof(pkt.type));
  strlcpy(pkt.payload, payload, sizeof(pkt.payload));
  esp_now_send(BROADCAST_MAC, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));
}

void onDataRecv(const esp_now_recv_info_t*, const uint8_t* data, int len) {
  if (len != sizeof(Packet)) return;

  Packet pkt;
  memcpy(&pkt, data, sizeof(pkt));
  pkt.type[sizeof(pkt.type) - 1] = '\0';
  pkt.payload[sizeof(pkt.payload) - 1] = '\0';

  if (strcmp(pkt.type, "PROMPT") == 0) {
    Serial.println(F("\n=== MESSAGE FROM DESK UNIT ==="));
    Serial.println(pkt.payload);
    Serial.println(F("Type the access code and press Enter."));
  } else if (strcmp(pkt.type, "REWARD") == 0) {
    Serial.println(F("\n=== SUCCESS ==="));
    Serial.println(pkt.payload);
    Serial.println(F("==============="));
  }
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println(F("\n=== PARTICIPANT UNIT ==="));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ERROR: ESP-NOW init failed."));
    while (true) delay(1000);
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println(F("ERROR: Could not register broadcast peer."));
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onDataRecv);
  Serial.println(F("Listening for the desk unit's broadcast..."));
}

void loop() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      serialLine.trim();
      if (serialLine.length() > 0) {
        sendPacket("ANSWER", serialLine.c_str());
        Serial.print(F("Sent answer: "));
        Serial.println(serialLine);
        serialLine = "";
      }
    } else {
      serialLine += c;
    }
  }
}
