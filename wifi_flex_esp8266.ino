#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// WiFi credentials
const char* ssid = "YOUR_SSID";      // Replace with your WiFi name
const char* password = "PASSWORD";  // Replace with your WiFi password

// UDP settings
const char* udpAddress = "192.168.0.112";  // Replace with receiver PC IP address
const int udpPort = 1337;                   // Port number
WiFiUDP udp;

// Flex sensor settings
#define FLEX_PIN A0
#define SAMPLES 10
int rawMin = 4095;
int rawMax = 0;

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // Connect to WiFi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Begin UDP
  udp.begin(udpPort);
  Serial.println("UDP started");
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nReconnected!");
  }
  
  // Read flex sensor with averaging
  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(FLEX_PIN);
    delay(2);
  }
  int rawAvg = sum / SAMPLES;
  
  // Auto-calibration
  rawMin = min(rawMin, rawAvg);
  rawMax = max(rawMax, rawAvg);
  
  // Normalize
  float norm = (rawAvg - rawMin) / (float)(rawMax - rawMin);
  norm = constrain(norm, 0.0, 1.0);
  
  // Sensitivity boost
  float curved = pow(norm, 1.5);
  
  // Scale to 0–1000
  int flex = (int)(curved * 1000);
  
  // Create message
  String message = String(rawAvg) + "," + String(flex);
  
  // Send via UDP
  udp.beginPacket(udpAddress, udpPort);
  udp.print(message);
  udp.endPacket();
  
  // Debug print
  Serial.print("Sent: RawAvg: ");
  Serial.print(rawAvg);
  Serial.print(" | Flex: ");
  Serial.print(flex);
  Serial.print(" | To: ");
  Serial.print(udpAddress);
  Serial.print(":");
  Serial.println(udpPort);
  
  delay(50);
}