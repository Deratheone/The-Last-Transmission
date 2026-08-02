#include <esp_now.h>
#include <WiFi.h>

#define LED_PIN 2  // Built-in LED on most ESP32 boards

// YOUR RECEIVER MAC ADDRESS
uint8_t receiverAddress[] = {0xF4, 0x65, 0x0B, 0x56, 0x1B, 0xA8};

typedef struct message {
  char text[32];
} message;

message myMessage;

// Callback when data is sent - NEW SIGNATURE for v3.x
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Success");
    // Blink LED on success
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
  } else {
    Serial.println("Fail");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give Serial time to initialize
  
  // Setup LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Set device as WiFi station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Disconnect from any previous connection
  delay(100);  // Give WiFi time to initialize
  
  // Print this device's MAC address
  Serial.print("Transmitter MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));  // Clear structure
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;  // Added: specify interface
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("Transmitter ready!");
  Serial.print("Sending to MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", receiverAddress[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  Serial.println("---");
}

void loop() {
  // Prepare message
  strcpy(myMessage.text, "Hello from ESP32!");
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &myMessage, sizeof(myMessage));
   
  if (result == ESP_OK) {
    Serial.println("Message queued for sending...");
  } else {
    Serial.print("Error sending: ");
    Serial.println(result);
  }
  
  delay(2000);  // Send every 2 seconds
}