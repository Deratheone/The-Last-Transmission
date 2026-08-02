#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

typedef struct {
  char text[32];
} Message;

Message incoming;

void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *data,
                int len)
{
  memcpy(&incoming, data, sizeof(incoming));

  Serial.println();
  Serial.println("========== PACKET ==========");

  Serial.print("From : ");

  for(int i=0;i<6;i++)
  {
    Serial.printf("%02X", info->src_addr[i]);

    if(i<5)
      Serial.print(":");
  }

  Serial.println();

  Serial.print("Length : ");
  Serial.println(len);

  Serial.print("Message : ");
  Serial.println(incoming.text);

  Serial.println("============================");
}

void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  // Same channel as transmitter
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Listening for Broadcasts...");
}

void loop() {
}