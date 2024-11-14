#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <vector>


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // WebSocket server on endpoint "/ws"

// Wi-Fi credentials file paths
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";

// Touch sensor parameters
const int touchPin = T0;
const int touchThreshold = 40;
int touchCounter = 0;
int touchEventsInInterval = 0;

// Timing parameters
unsigned long lastAnalysisTime = 0;
const unsigned long analysisInterval = 60000; // Analyze every 60 seconds

// Data storage
const char* dataFilePath = "/data.csv";

// Reset button parameters
const int buttonPin = 21; // GPIO pin for reset button
volatile bool resetDataFlag = false;
const unsigned long resetHoldTime = 5000; // 5 seconds to trigger reset
unsigned long buttonPressTime = 0;

// WebSocket update interval
const unsigned long wsUpdateInterval = 1000; // 1 second
unsigned long lastWsUpdate = 0;

// Function to convert CSV data to JSON
String csvToJson(const char* path) {
  File file = SPIFFS.open(path);
  if (!file) {
    Serial.println("- failed to open file for reading");
    return "[]";
  }
  String json = "[";
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      int commaIndex1 = line.indexOf(',');
      int commaIndex2 = line.indexOf(',', commaIndex1 + 1);
      if (commaIndex1 > 0 && commaIndex2 > commaIndex1) {
        String timestamp = line.substring(0, commaIndex1);
        String touchCount = line.substring(commaIndex1 + 1, commaIndex2);
        String touchRate = line.substring(commaIndex2 + 1);
        json += "{\"timestamp\":" + timestamp + ",\"touchCount\":" + touchCount + ",\"touchRate\":" + touchRate + "},";
      }
    }
  }
  if (json.endsWith(",")) json.remove(json.length() - 1);
  json += "]";
  file.close();
  return json;
}

// Function to append data to a file
void appendToFile(const char* path, const String& message) {
  File file = SPIFFS.open(path, FILE_APPEND);
  if (file) {
    file.println(message);
    file.close();
  } else {
    Serial.println("- failed to open file for appending");
  }
}

// Limit the number of entries in the CSV file
void limitCSVEntries(const char* path, int maxEntries) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.println("- failed to open file for reading");
    return;
  }
  std::vector<String> lines;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      lines.push_back(line);
    }
  }
  file.close();

  // Keep only the last maxEntries lines
  if (lines.size() > maxEntries) {
    lines.erase(lines.begin(), lines.end() - maxEntries);
  }

  // Write back the latest entries to the file
  file = SPIFFS.open(path, FILE_WRITE);
  if (file) {
    for (const auto& line : lines) {
      file.println(line);
    }
    file.close();
  } else {
    Serial.println("- failed to open file for writing");
  }
}

// Read file from SPIFFS
String readFile(const char* path) {
  File file = SPIFFS.open(path);
  if (!file) {
    Serial.printf("- failed to open %s for reading\n", path);
    return String();
  }
  String content = file.readStringUntil('\n');
  file.close();
  return content;
}

// Write file to SPIFFS
void writeFile(const char* path, const String& message) {
  File file = SPIFFS.open(path, FILE_WRITE);
  if (file) {
    file.print(message);
    file.close();
  } else {
    Serial.printf("- failed to open %s for writing\n", path);
  }
}

// Initialize WiFi
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

// WebSocket event handler
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
}

// Perform periodic analysis and save results
void performAnalysis() {
  if (millis() - lastAnalysisTime >= analysisInterval) {
    lastAnalysisTime = millis();

    // Calculate touch rate (touch events per minute)
    int touchRate = touchEventsInInterval;
    unsigned long timestamp = millis();

    // Format data as CSV line: timestamp,touchCounter,touchRate
    String dataLine = String(timestamp) + "," + String(touchCounter) + "," + String(touchRate);

    // Append data to CSV file
    appendToFile(dataFilePath, dataLine);

    // Limit the CSV file to the last 100 entries
    limitCSVEntries(dataFilePath, 100);

    Serial.println("Analysis saved to CSV.");

    // Reset touchEventsInInterval for the next interval
    touchEventsInInterval = 0;
  }
}

// Interrupt Service Routine (ISR) for the button
void IRAM_ATTR buttonISR() {
  if (digitalRead(buttonPin) == LOW) {
    buttonPressTime = millis();
  } else {
    if (millis() - buttonPressTime >= resetHoldTime) {
      resetDataFlag = true; // Set the flag to reset data
    }
  }
}

// Reset data function
void resetData() {
  touchCounter = 0;
  touchEventsInInterval = 0;
  SPIFFS.remove(dataFilePath);
  Serial.println("Data has been reset.");

  // Notify connected WebSocket clients
  if (ws.count() > 0) {
    ws.textAll("reset");
  }
}

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  } else {
    Serial.println("SPIFFS mounted successfully");
  }

  // Initialize the button pin
  pinMode(buttonPin, INPUT_PULLUP); // Assumes the button is connected to GND when pressed
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, CHANGE);

  // Load Wi-Fi credentials from SPIFFS
  String ssid = readFile(ssidPath);
  String pass = readFile(passPath);

  // Initialize Wi-Fi or set up Access Point
  if (initWiFi(ssid, pass)) {
    // Serve main HTML page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", "text/html");
    });

    // Serve data as JSON
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
      String json = csvToJson(dataFilePath);
      request->send(200, "application/json", json);
    });

    // Handle reset data request
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
      resetData();
      request->send(200, "text/plain", "Data has been reset.");
    });
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
  }

  // Serve static files
  server.serveStatic("/", SPIFFS, "/");

  // WebSocket setup and event handling
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  // Start the server
  server.begin();
}

void loop() {
  // Touch sensor reading
  int touchValue = touchRead(touchPin);
  if (touchValue < touchThreshold) {
    touchCounter++;
    touchEventsInInterval++;
    Serial.printf("Touch detected! Counter: %d\n", touchCounter);
    delay(500); // Debounce delay
  }

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
