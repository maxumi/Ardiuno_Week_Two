/**
 * @file WebServer.h
 * @brief Header file for web server setup and handling.
 *
 * Contains declarations for setting up the web server, defining routes,
 * and handling WebSocket events.
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "Globals.h"

/**
 * @brief Sets up the web server routes and handlers.
 *
 * Configures HTTP endpoints, serves files, and initializes WebSocket event handling.
 */
void setupWebServer();

/**
 * @brief Converts CSV data from a file to a JSON string.
 *
 * @param path The path to the CSV file.
 * @return JSON-formatted string representing the CSV data.
 *
 * Reads CSV data line by line and constructs a JSON array of objects.
 */
String csvToJson(const char* path);

#endif // WEB_SERVER_H
