#include <Arduino.h>

int ledPin = 2;

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  digitalWrite(ledPin, HIGH);   // LED ON
  delay(1000);                 // wait 1 second

  digitalWrite(ledPin, LOW);    // LED OFF
  delay(1000);                 // wait 1 second
}