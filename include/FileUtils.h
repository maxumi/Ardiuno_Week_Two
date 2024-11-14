/**
 * @file FileUtils.h
 * @brief Header file for file utility functions using SPIFFS.
 *
 * Provides functions to initialize SPIFFS, read from files, write to files, append data,
 * and limit the number of entries in a CSV file.
 */

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "Globals.h"

/**
 * @brief Initializes the SPIFFS file system.
 *
 * Mounts the SPIFFS file system and prints the status to the serial monitor.
 */
void initSPIFFS();

/**
 * @brief Reads the first line from a file in SPIFFS.
 *
 * @param path The path to the file.
 * @return The content of the first line as a String.
 *
 * If the file cannot be opened, an empty String is returned.
 */
String readFile(const char* path);

/**
 * @brief Writes a message to a file in SPIFFS.
 *
 * @param path The path to the file.
 * @param message The message to write.
 *
 * Overwrites any existing content in the file.
 */
void writeFile(const char* path, const String& message);

/**
 * @brief Appends a message to a file in SPIFFS.
 *
 * @param path The path to the file.
 * @param message The message to append.
 *
 * Adds the message to the end of the file with a newline.
 */
void appendToFile(const char* path, const String& message);

/**
 * @brief Limits the number of entries in a CSV file by removing the oldest entries.
 *
 * @param path The path to the CSV file.
 * @param maxEntries The maximum number of entries to keep.
 *
 * Keeps only the last `maxEntries` lines in the file.
 */
void limitCSVEntries(const char* path, int maxEntries);

#endif // FILE_UTILS_H
