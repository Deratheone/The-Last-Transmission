#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

// ============================================================
// Desk Unit (Transmitter)
//
// Broadcasts a repeating prompt asking for the access code.
// Any participant device on the same channel can hear it —
// no MAC address needs to be known ahead of time.
//
// When a participant broadcasts back the correct password,
// this unit broadcasts the final reward message to everyone.
// ============================================================

constexpr uint8_t WIFI_CHANNEL = 1;
uint8_t BROADCAST_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const char* EXPECTED_PASSWORD   = "RY2042";
const char* REWARD_MESSAGE      = "Room Archive 7 - Password: EVA!!!";
const unsigned long PROMPT_INTERVAL_MS = 5000;

struct Packet {
  char type[16];
  char payload[64];
};

unsigned long lastPromptTime = 0;
bool solved = false;

void sendPacket(const char* type, const char* payload) {
  Packet pkt = {};
  strlcpy(pkt.type, type, sizeof(pkt.type));
  strlcpy(pkt.payload, payload, sizeof(pkt.payload));
  esp_now_send(BROADCAST_MAC, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));
}

void onDataRecv(const esp_now_recv_info_t*, const uint8_t* data, int len) {
  if (len != sizeof(Packet) || solved) return;

  Packet pkt;
  memcpy(&pkt, data, sizeof(pkt));
  pkt.type[sizeof(pkt.type) - 1] = '\0';
  pkt.payload[sizeof(pkt.payload) - 1] = '\0';

  if (strcmp(pkt.type, "ANSWER") != 0) return;

  Serial.print(F("Received answer: "));
  Serial.println(pkt.payload);

  if (strcmp(pkt.payload, EXPECTED_PASSWORD) == 0) {
    solved = true;
    Serial.println(F(">>> Correct password. Broadcasting reward. <<<"));
    sendPacket("REWARD", REWARD_MESSAGE);
  } else {
    Serial.println(F("Incorrect password."));
  }
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println(F("\n=== DESK UNIT ==="));

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
  Serial.println(F("Broadcasting prompt. Waiting for a valid password..."));
}

void loop() {
  if (!solved && millis() - lastPromptTime > PROMPT_INTERVAL_MS) {
    sendPacket("PROMPT", "Give me the access code.");
    lastPromptTime = millis();
  }
}
