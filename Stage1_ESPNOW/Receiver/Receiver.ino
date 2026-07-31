#include <esp_now.h>
#include <WiFi.h>

typedef struct {
  char text[32];
} message;

message incoming;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(&incoming, data, sizeof(incoming));
  Serial.print("Received: ");
  Serial.println(incoming.text);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Listening for ESP-NOW broadcasts...");
}

void loop() {
  // nothing needed here — receiving happens via callback
}