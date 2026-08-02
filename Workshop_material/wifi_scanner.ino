#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

typedef struct {
  char text[32];
} Message;

Message msg;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  // Force both ESP32s to the same channel
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Broadcast Peer");
    return;
  }

  strcpy(msg.text, "Hello how are you? I am an ESP32");

  Serial.println("Broadcast Transmitter Ready");
}

void loop() {

  esp_err_t result = esp_now_send(
      broadcastAddress,
      (uint8_t *)&msg,
      sizeof(msg));

  if(result == ESP_OK)
      Serial.println("Broadcast Sent");
  else
      Serial.printf("Send Error: %d\n", result);

  delay(2000);
}