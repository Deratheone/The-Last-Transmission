#include <esp_now.h>
#include <WiFi.h>

#define LED_PIN 2

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct {
  char text[32];
} message;

message myMessage;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  strcpy(myMessage.text, "Pass123456RoomARCHIVE2");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  esp_now_send(broadcastAddress, (uint8_t *)&myMessage, sizeof(myMessage));
  digitalWrite(LED_PIN, LOW);

  delay(1000);
}