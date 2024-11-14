// main.cpp
#include "Globals.h"
#include "FileUtils.h"
#include "WiFiUtils.h"
#include "WebServer.h"
#include "TouchSensor.h"
#include "DataAnalysis.h"

// Initialize global variables
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // WebSocket server on endpoint "/ws"

const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";

const int touchPin = T0;
const int touchThreshold = 40;
int touchCounter = 0;
int touchEventsInInterval = 0;

unsigned long lastAnalysisTime = 0;
const unsigned long analysisInterval = 60000; // Analyze every 60 seconds

const char* dataFilePath = "/data.csv";

const int buttonPin = 21; // GPIO pin for reset button
volatile bool resetDataFlag = false;
const unsigned long resetHoldTime = 5000; // 5 seconds to trigger reset
unsigned long buttonPressTime = 0;

const unsigned long wsUpdateInterval = 1000; // 1 second
unsigned long lastWsUpdate = 0;

void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS
  initSPIFFS();

  // Initialize touch sensor and button
  setupTouchSensor();

  // Load Wi-Fi credentials from SPIFFS
  String ssid = readFile(ssidPath);
  String pass = readFile(passPath);

  // Initialize Wi-Fi or set up Access Point
  if (initWiFi(ssid, pass)) {
    // Set up the web server routes and handlers
    setupWebServer();
  } else {
    Serial.println("Setting AP (Access Point)");
    WiFi.softAP("ESP-WIFI-MANAGER_MAX");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Serve WiFi manager HTML page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });

    // Handle form POST request to save Wi-Fi credentials
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
        String ssid = request->getParam("ssid", true)->value();
        String pass = request->getParam("pass", true)->value();
        writeFile(ssidPath, ssid);
        writeFile(passPath, pass);
        request->send(200, "text/plain", "Settings saved. Rebooting...");
        delay(1000);
        ESP.restart();
      } else {
        request->send(400, "text/plain", "Bad Request");
      }
    });

    // Start the server
    server.begin();
  }
}

void loop() {
  readTouchSensor();

  // WebSocket update logic
  if (ws.count() > 0 && (millis() - lastWsUpdate >= wsUpdateInterval)) {
    lastWsUpdate = millis();
    ws.textAll(String(touchCounter));
  }

  // Perform periodic analysis and save results
  performAnalysis();

  // Check if reset is requested
  if (resetDataFlag) {
    resetDataFlag = false; // Clear the flag
    resetData();
  }

  delay(50);
}
