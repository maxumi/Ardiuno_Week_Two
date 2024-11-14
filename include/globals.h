/**
 * @file Globals.h
 * @brief Global variables and definitions for the project.
 *
 * Contains declarations of global variables, constants, and external objects used across multiple modules.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <vector>
#include <ArduinoJson.h>

/** @brief Asynchronous web server object on port 80. */
extern AsyncWebServer server;

/** @brief WebSocket server object on endpoint "/ws". */
extern AsyncWebSocket ws;

/** @brief Path to the Wi-Fi SSID file in SPIFFS. */
extern const char* ssidPath;

/** @brief Path to the Wi-Fi password file in SPIFFS. */
extern const char* passPath;

/** @brief GPIO pin connected to the touch sensor (T0). */
extern const int touchPin;

/** @brief Threshold value for touch detection. */
extern const int touchThreshold;

/** @brief Counter for total touch events detected. */
extern int touchCounter;

/** @brief Counter for touch events within the current analysis interval. */
extern int touchEventsInInterval;

/** @brief Timestamp of the last analysis performed. */
extern unsigned long lastAnalysisTime;

/** @brief Interval (in milliseconds) between data analyses. */
extern const unsigned long analysisInterval;

/** @brief Path to the CSV data file in SPIFFS. */
extern const char* dataFilePath;

/** @brief GPIO pin connected to the reset button. */
extern const int buttonPin;

/** @brief Flag indicating if a data reset has been requested. */
extern volatile bool resetDataFlag;

/** @brief Time (in milliseconds) the reset button must be held to trigger a reset. */
extern const unsigned long resetHoldTime;

/** @brief Timestamp when the reset button was pressed. */
extern unsigned long buttonPressTime;

/** @brief Interval (in milliseconds) between WebSocket updates. */
extern const unsigned long wsUpdateInterval;

/** @brief Timestamp of the last WebSocket update sent. */
extern unsigned long lastWsUpdate;

#endif // GLOBALS_H
