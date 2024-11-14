// WebServer.cpp
#include "WebServer.h"
#include "FileUtils.h"
#include "TouchSensor.h"

// Function to convert CSV data to JSON
String csvToJson(const char* path) {
  File file = SPIFFS.open(path);
  if (!file) {
    Serial.println("- failed to open file for reading");
    return "[]";
  }

  JsonDocument doc;
  JsonArray dataArray = doc.to<JsonArray>();

  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      int commaIndex1 = line.indexOf(',');
      int commaIndex2 = line.indexOf(',', commaIndex1 + 1);
      if (commaIndex1 > 0 && commaIndex2 > commaIndex1) {
        String timestamp = line.substring(0, commaIndex1);
        String touchCount = line.substring(commaIndex1 + 1, commaIndex2);
        String touchRate = line.substring(commaIndex2 + 1);

        JsonObject dataPoint = dataArray.createNestedObject();
        dataPoint["timestamp"] = timestamp.toInt();
        dataPoint["touchCount"] = touchCount.toInt();
        dataPoint["touchRate"] = touchRate.toInt();
      }
    }
  }
  file.close();

  // Serialize JSON to a String
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
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

void setupWebServer() {
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

  // Serve static files
  server.serveStatic("/", SPIFFS, "/");

  // WebSocket setup and event handling
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  // Start the server
  server.begin();
}
