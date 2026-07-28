#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

// ============================================================
// Stage 6 - EVA Hidden Synchronization Cache (Node Firmware)
//
// This ESP32 waits for a two-way ESP-NOW recovery request. A participant
// terminal must send the Stage 5 access key. The node replies with the final
// memory fragment and the location of the Communication Core.
// ============================================================

constexpr uint8_t WIFI_CHANNEL = 1;
const char* REQUIRED_KEY = "123456";  // Shared access code, recovered at Stage 5.
const char* FINAL_FRAGMENT = "123456"; // Fragment 6 of the master key.

struct Packet {
  char type[16];
  char payload[64];
};

Packet receivedPacket = {};
uint8_t receivedMac[6] = {};
volatile bool packetReady = false;

bool addPeerIfNeeded(const uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) return true;

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  return esp_now_add_peer(&peerInfo) == ESP_OK;
}

void sendPacket(const uint8_t* destination, const char* type, const char* payload) {
  if (!addPeerIfNeeded(destination)) {
    Serial.println(F("ERROR: Could not add requesting terminal as an ESP-NOW peer."));
    return;
  }

  Packet response = {};
  strlcpy(response.type, type, sizeof(response.type));
  strlcpy(response.payload, payload, sizeof(response.payload));

  esp_err_t result = esp_now_send(destination, reinterpret_cast<uint8_t*>(&response), sizeof(response));
  Serial.print(F("Response status: "));
  Serial.println(result == ESP_OK ? F("queued") : F("failed"));
}

void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len != sizeof(Packet) || packetReady) return;

  memcpy(&receivedPacket, data, sizeof(Packet));
  receivedPacket.type[sizeof(receivedPacket.type) - 1] = '\0';
  receivedPacket.payload[sizeof(receivedPacket.payload) - 1] = '\0';
  memcpy(receivedMac, info->src_addr, 6);
  packetReady = true;
}

void printMac(const uint8_t* mac) {
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(':');
  }
}

void processPacket() {
  Packet request = receivedPacket;
  uint8_t sender[6];
  memcpy(sender, receivedMac, 6);
  packetReady = false;

  Serial.print(F("Request from "));
  printMac(sender);
  Serial.print(F(" | type="));
  Serial.println(request.type);

  if (strcmp(request.type, "EVA_RESTORE") != 0) {
    sendPacket(sender, "EVA_DENIED", "Unknown recovery protocol.");
    return;
  }

  if (strcmp(request.payload, REQUIRED_KEY) != 0) {
    sendPacket(sender, "EVA_DENIED", "Key rejected. Recover the Stage 5 memory cache.");
    return;
  }

  char reply[64];
  snprintf(reply, sizeof(reply), "Hidden cache restored. Room 7. Fragment: %s", FINAL_FRAGMENT);
  sendPacket(sender, "EVA_RESTORED", reply);
  Serial.println(F("EVA hidden cache restored; final fragment sent."));
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println(F("\n=== STAGE 6: EVA HIDDEN SYNCHRONIZATION CACHE ==="));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.print(F("NODE 6 MAC: "));
  Serial.println(WiFi.macAddress());
  Serial.println(F("Give this MAC address to the participant terminal."));
  Serial.println(F("Expected request: EVA_RESTORE / 123456"));

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ERROR: ESP-NOW initialization failed."));
    while (true) delay(1000);
  }

  if (esp_now_register_recv_cb(onDataRecv) != ESP_OK) {
    Serial.println(F("ERROR: Could not register receive callback."));
    while (true) delay(1000);
  }

  Serial.println(F("Hidden cache online. Awaiting recovery request."));
}

void loop() {
  if (packetReady) processPacket();
  delay(10);
}
