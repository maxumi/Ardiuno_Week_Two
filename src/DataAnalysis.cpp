// DataAnalysis.cpp
#include "DataAnalysis.h"
#include "FileUtils.h"
#include "WebServer.h"
#include "globals.h"

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

    Serial.println("Data saved to CSV.");

    // Reset touchEventsInInterval for the next interval
    touchEventsInInterval = 0;

    // **Send the new data point over WebSocket**
    DynamicJsonDocument doc(256); // Adjust size as needed
    doc["timestamp"] = timestamp;
    doc["touchCount"] = touchCounter;
    doc["touchRate"] = touchRate;

    String jsonString;
    serializeJson(doc, jsonString);
    jsonString = "New_Data:" + jsonString;


    // Send to all connected clients
    ws.textAll(jsonString);
  }
}
