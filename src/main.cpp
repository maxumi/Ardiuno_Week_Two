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
int touchEventsInInterval = 0;
int peakTouchRate = 0;

const int ledPin = 2;
unsigned long lastSaveTime = 0;
unsigned long previousMillis = 0;
const unsigned long saveInterval = 60000;      // Save every 60 seconds
const unsigned long analysisInterval = 60000;  // Analyze every 60 seconds
unsigned long lastAnalysisTime = 0;
const long interval = 10000;                   // Wi-Fi connection interval

const char* dataFilePath = "/data.csv";

String ledState;

// Function to convert CSV data to JSON
String csvToJson(fs::FS &fs, const char * path) {
  File file = fs.open(path);
  if (!file) {
    Serial.println("- failed to open file for reading");
    return "[]";
  }
  String json = "[";
  bool firstLine = true;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      if (!firstLine) {
        json += ",";
      } else {
        firstLine = false;
      }
      int commaIndex1 = line.indexOf(',');
      int commaIndex2 = line.indexOf(',', commaIndex1 + 1);
      if (commaIndex1 > 0 && commaIndex2 > commaIndex1) {
        String timestamp = line.substring(0, commaIndex1);
        String touchCount = line.substring(commaIndex1 + 1, commaIndex2);
        String touchRate = line.substring(commaIndex2 + 1);
        json += "{\"timestamp\":" + timestamp + ",\"touchCount\":" + touchCount + ",\"touchRate\":" + touchRate + "}";
      }
    }
  }
  json += "]";
  file.close();
  return json;
}

// Function to append data to a file
void appendToFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.println(message)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}

// Function to limit the number of entries in the CSV file
void limitCSVEntries(fs::FS &fs, const char * path, int maxEntries) {
  File file = fs.open(path, FILE_READ);
  if (!file) {
    Serial.println("- failed to open file for reading");
    return;
  }

  // Read all lines into an array
  String lines[101]; // Max entries + 1
  int lineCount = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      if (lineCount < 101) {
        lines[lineCount] = line;
        lineCount++;
      }
    }
  }
  file.close();

  // If more than maxEntries, remove oldest entries
  int start = lineCount > maxEntries ? lineCount - maxEntries : 0;
  int count = lineCount - start;

  // Write back the latest entries to the file
  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  for (int i = start; i < lineCount; i++) {
    file.println(lines[i]);
  }
  file.close();
}

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
  file.close();
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
  file.close();
}

// Initialize WiFi
bool initWiFi() {
  if (ssid == "") {
    Serial.println("Undefined SSID.");
    return false;
  }
  WiFi.mode(WIFI_STA);

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
  Serial.println(WiFi.localIP()); // This will show the dynamically assigned IP
  return true;
}

// WebSocket event handler
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
}

const unsigned long wsUpdateInterval = 1000; // 1 second WebSocket update interval
unsigned long lastWsUpdate = 0;

// Perform periodic analysis and save results
void performAnalysis() {
  if (millis() - lastAnalysisTime >= analysisInterval) {
    lastAnalysisTime = millis();

    // Calculate touch rate (touch events per minute)
    int touchRate = touchEventsInInterval;
    if (touchRate > peakTouchRate) {
      peakTouchRate = touchRate;
    }

    // Get current timestamp
    unsigned long timestamp = millis();

    // Format data as CSV line: timestamp,touchCounter,touchRate
    String dataLine = String(timestamp) + "," + String(touchCounter) + "," + String(touchRate);

    // Append data to CSV file
    appendToFile(SPIFFS, dataFilePath, dataLine.c_str());

    // Limit the CSV file to the last 100 entries
    limitCSVEntries(SPIFFS, dataFilePath, 100);

    Serial.println("Analysis saved to CSV.");

    // Reset touchEventsInInterval for the next interval
    touchEventsInInterval = 0;
  }
}

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

  // Initialize Wi-Fi in STA mode or configure as Access Point
  if (initWiFi()) {
    // Serve main HTML page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html");
    });
    server.serveStatic("/", SPIFFS, "/");

    // Serve data as JSON
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
      String json = csvToJson(SPIFFS, dataFilePath);
      request->send(200, "application/json", json);
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
    touchEventsInInterval++;
    Serial.print("Touch detected! Counter: ");
    Serial.println(touchCounter);
    delay(500); // Debounce delay
  }

  // WebSocket update logic
  if (ws.count() > 0 && (millis() - lastWsUpdate >= wsUpdateInterval)) {
    lastWsUpdate = millis();
    ws.textAll(String(touchCounter));
  }

  // Perform periodic analysis and save results
  performAnalysis();

  // Periodically save touchCounter to SPIFFS
  if (millis() - lastSaveTime > saveInterval) {
    lastSaveTime = millis();
    writeFile(SPIFFS, "/touchcount.txt", String(touchCounter).c_str());
    Serial.println("touchCounter saved to SPIFFS.");
  }

  delay(50);
}
