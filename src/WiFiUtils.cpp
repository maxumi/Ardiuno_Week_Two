// WiFiUtils.cpp
#include "WiFiUtils.h"

bool initWiFi(const String& ssid, const String& pass) {
  if (ssid.isEmpty()) {
    Serial.println("Undefined SSID.");
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 10000; // 10 seconds timeout

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    delay(100);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Failed to connect.");
    return false;
  }
}
