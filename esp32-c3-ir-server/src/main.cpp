#include <Arduino.h>

#include <FastLED.h>
#define NEOPIXEL 4
#define NUM_LEDS 3
CRGB leds[NUM_LEDS];

#include <ESPmDNS.h>
#include <WiFi.h>
#include <WebServer.h>
WebServer server(80);

void setup_server() {
  server.begin();
}

void setup() {
  FastLED.addLeds<WS2812, NEOPIXEL>(leds, NUM_LEDS);
  Serial.begin(250000);

  WiFi.begin();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.println("Connecting to Wifi...");
  }
  MDNS.begin("esp32");
  Serial.println("esp32.local");
  setup_server();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected. Restarting.");
    ESP.restart();
  }
  server.handleClient();
}
