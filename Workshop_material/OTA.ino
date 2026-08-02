OTA upload:
#include <WiFi.h>
#include <ArduinoOTA.h>

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

const int LED_PIN = 2;     // Built-in LED on most ESP32 boards

int blinkDelay = 1000;

void setup() {

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  WiFi.begin(ssid, password);

  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("ESP32-BLINK");

  ArduinoOTA.onStart([]() {
    Serial.println("OTA Update Started");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update Complete");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", progress * 100 / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]\n", error);
  });

  ArduinoOTA.begin();

  Serial.println("OTA Ready");
}

void loop() {

  ArduinoOTA.handle();

  digitalWrite(LED_PIN, HIGH);
  delay(blinkDelay);

  digitalWrite(LED_PIN, LOW);
  delay(blinkDelay);
}