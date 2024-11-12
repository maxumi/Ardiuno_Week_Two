#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

const char* ssid = "e307";
const char* password = "rockyCartoon544";

#define LED_PIN 2
#define BUTTON_PIN 4

volatile bool ledState = false;

AsyncWebServer server(80);

void IRAM_ATTR handleButtonPress() {
  // Toggle the LED state and update it immediately
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
}

String getLEDState() {
  return ledState ? "ON" : "OFF";
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Gives the button Pin a function for attachInterrupt
  attachInterrupt(BUTTON_PIN, handleButtonPress, FALLING);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connecting to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  // Show local ip address to connect. Note you need to be on the same ip address to work
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });

  //Loads style.css for index when needed.
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  //  toggle LED
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    request->send(200, "text/plain", getLEDState());
  });

  // Route to get the current LED state
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", getLEDState());
  });

  // Start server
  server.begin();
}

void loop() {

}
