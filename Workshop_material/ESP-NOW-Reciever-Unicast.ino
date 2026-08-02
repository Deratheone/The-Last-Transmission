#include <esp_now.h>
#include <WiFi.h>

typedef struct {
  char text[32];
} message;

message receivedMessage;

void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));
  Serial.print("Received: ");
  Serial.println(receivedMessage.text);
}

void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  delay(100);           // Give WiFi time to initialize

  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receiver ready");
}

void loop() {
  // nothing needed here, all handled in callback
}
