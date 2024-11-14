/**
 * @file WiFiUtils.h
 * @brief Header file for Wi-Fi initialization and management.
 *
 * Provides functions to initialize Wi-Fi connections and handle access point setup if necessary.
 */

#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include "Globals.h"

/**
 * @brief Initializes Wi-Fi connection in station mode.
 *
 * Attempts to connect to the specified Wi-Fi network using the provided SSID and password.
 * If the connection is successful within the timeout period, it returns true.
 *
 * @param ssid The SSID of the Wi-Fi network.
 * @param pass The password of the Wi-Fi network.
 * @return True if connected successfully, false otherwise.
 */
bool initWiFi(const String& ssid, const String& pass);

#endif // WIFI_UTILS_H
