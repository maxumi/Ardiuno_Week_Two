// FileUtils.cpp
#include "FileUtils.h"

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  } else {
    Serial.println("SPIFFS mounted successfully");
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

// Append data to a file
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
