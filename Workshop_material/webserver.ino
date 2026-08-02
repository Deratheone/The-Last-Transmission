#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32-AP";
const char* password = "12345678";

WebServer server(80);

// Compact HTML stored in flash memory
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Server</title>
  <style>
    body{font-family:Arial;text-align:center;background:#667eea;margin:0;padding:20px}
    .box{background:#fff;border-radius:10px;padding:30px;max-width:400px;margin:50px auto}
    h1{color:#333}
    .wave{font-weight:bold;color:#FF8C00;font-size:24px;margin:20px 0}
  </style>
</head>
<body>
  <div class="box">
    <h1>ESP32 Web Server</h1>
    <div class="wawe">THE LAST TRANSMISSION</div>
  </div>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", webpage);  // Send HTML page
}

void setup() {
  Serial.begin(115200);
  
  WiFi.softAP(ssid, password);  // Start Access Point
  Serial.println("AP IP: " + WiFi.softAPIP().toString());
  
  server.on("/", handleRoot);  // Route: root page
  
  server.begin();  // Start server
  Serial.println("Server started");
}

void loop() {
  server.handleClient();  // Handle incoming requests
}