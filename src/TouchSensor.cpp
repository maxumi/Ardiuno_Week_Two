// TouchSensor.cpp
#include "TouchSensor.h"
#include "DataAnalysis.h"
#include "FileUtils.h" // Include this to access SPIFFS functions
#include "Globals.h"   // Ensure Globals.h is included to access ssidPath and passPath

void setupTouchSensor() {
  // Initialize the button pin
  pinMode(buttonPin, INPUT_PULLUP); // Assumes the button is connected to GND when pressed
  // Setsup the interupt for buttonISR function
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, CHANGE);
}

void readTouchSensor() {
  // Touch sensor reading
  int touchValue = touchRead(touchPin);
  if (touchValue < touchThreshold) {
    touchCounter++;
    touchEventsInInterval++;
    Serial.printf("Touch detected! Counter: %d\n", touchCounter);
    delay(500); // Debounce delay
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

  // Remove data file
  if (SPIFFS.remove(dataFilePath)) {
    Serial.println("Data file has been reset.");
  } else {
    Serial.println("Failed to reset data file or file does not exist.");
  }

  // Remove Wi-Fi configuration files
  bool ssidRemoved = SPIFFS.remove(ssidPath);
  bool passRemoved = SPIFFS.remove(passPath);

  if (ssidRemoved && passRemoved) {
    Serial.println("Wi-Fi credentials have been reset.");
  } else {
    Serial.println("Failed to reset Wi-Fi credentials or files do not exist.");
  }

  // Notify connected WebSocket clients
  if (ws.count() > 0) {
    ws.textAll("reset");
  }

  // Restart the ESP to apply changes
  Serial.println("Restarting ESP...");
  delay(1000);
  ESP.restart();
}
