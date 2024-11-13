#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // WebSocket server on endpoint "/ws"

// Wi-Fi parameters
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

String ssid, pass, ip, gateway;
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);

const int touchPin = T0;
int touchThreshold = 40;
int touchCounter = 0;

const int ledPin = 2;
unsigned long lastSaveTime = 0;
unsigned long previousMillis = 0;
const unsigned long saveInterval = 60000; // Save every 60 seconds
const long interval = 10000; // Wi-Fi connection interval

String ledState;

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  } else {
    Serial.println("SPIFFS mounted successfully");
  }
}

// Read file from SPIFFS
String readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String();
  }
  String fileContent;
  while (file.available()) {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if (ssid == "" || ip == "") {
    Serial.println("Undefined SSID or IP address.");
    return false;
  }
  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());

  if (!WiFi.config(localIP, localGateway, subnet)) {
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }
  Serial.println(WiFi.localIP());
  return true;
}

// Replaces placeholder with values
String processor(const String& var) {
  if (var == "STATE") {
    return digitalRead(ledPin) ? "ON" : "OFF";
  } else if (var == "TOUCHCOUNT") {
    return String(touchCounter);
  }
  return String();
}

// WebSocket event handler
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
}

const unsigned long wsUpdateInterval = 1000; // 1 second WebSocket update interval
unsigned long lastWsUpdate = 0;

void setup() {
  Serial.begin(115200);
  initSPIFFS();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Load Wi-Fi credentials from SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  pass = readFile(SPIFFS, passPath);
  ip = readFile(SPIFFS, ipPath);
  gateway = readFile(SPIFFS, gatewayPath);

  // Load touchCounter from SPIFFS
  String touchCountStr = readFile(SPIFFS, "/touchcount.txt");
  touchCounter = touchCountStr != "" ? touchCountStr.toInt() : 0;

  // Initialize Wi-Fi in STA mode or configure as Access Point
  if (initWiFi()) {
    // Serve main HTML page with WebSocket processor
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", SPIFFS, "/");

    // Serve ON and OFF endpoints to control LED
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, HIGH);
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, LOW);
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
    });
  } else {
    Serial.println("Setting AP (Access Point)");
    WiFi.softAP("ESP-WIFI-MANAGER_MAX", NULL);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Serve WiFi manager HTML page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });

    // Handle form POST request to save Wi-Fi credentials
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for (int i = 0; i < params; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isPost()) {
          if (p->name() == "ssid") {
            ssid = p->value().c_str();
            writeFile(SPIFFS, ssidPath, ssid.c_str());
          }
          if (p->name() == "pass") {
            pass = p->value().c_str();
            writeFile(SPIFFS, passPath, pass.c_str());
          }
          if (p->name() == "ip") {
            ip = p->value().c_str();
            writeFile(SPIFFS, ipPath, ip.c_str());
          }
          if (p->name() == "gateway") {
            gateway = p->value().c_str();
            writeFile(SPIFFS, gatewayPath, gateway.c_str());
          }
        }
      }
      request->send(200, "text/plain", "Settings saved. Rebooting...");
      delay(3000);
      ESP.restart();
    });
  }

  // Serve the CSS file
  server.serveStatic("/style.css", SPIFFS, "/style.css");

  // WebSocket setup and event handling
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  // Start the server
  server.begin();
}


void loop() {
  int touchValue = touchRead(touchPin);
  if (touchValue < touchThreshold) {
    touchCounter++;
    Serial.print("Touch detected! Counter: ");
    Serial.println(touchCounter);
    delay(500); // Debounce delay
  }

  // Check if there's a connected WebSocket client and send message if interval has passed
  if (ws.count() > 0 && (millis() - lastWsUpdate >= wsUpdateInterval)) {
    lastWsUpdate = millis();
    ws.textAll(String(touchCounter));
  }

  // Periodically save touchCounter to SPIFFS
  if (millis() - lastSaveTime > saveInterval) {
    lastSaveTime = millis();
    writeFile(SPIFFS, "/touchcount.txt", String(touchCounter).c_str());
    Serial.println("touchCounter saved to SPIFFS.");
  }

  delay(50);
}