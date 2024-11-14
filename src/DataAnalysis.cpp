// DataAnalysis.cpp
#include "DataAnalysis.h"
#include "FileUtils.h"

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

    Serial.println("saved to CSV.");

    // Reset touchEventsInInterval for the next interval
    touchEventsInInterval = 0;
  }
}
