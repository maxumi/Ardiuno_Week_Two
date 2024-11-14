// TouchSensor.cpp
#include "TouchSensor.h"
#include "DataAnalysis.h"

void setupTouchSensor() {
  // Initialize the button pin
  pinMode(buttonPin, INPUT_PULLUP); // Assumes the button is connected to GND when pressed
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
  SPIFFS.remove(dataFilePath);
  Serial.println("Data has been reset.");

  // Notify connected WebSocket clients
  if (ws.count() > 0) {
    ws.textAll("reset");
  }
}
