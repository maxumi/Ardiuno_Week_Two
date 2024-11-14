/**
 * @file TouchSensor.h
 * @brief Header file for touch sensor and button handling.
 *
 * Provides functions to initialize the touch sensor, read touch inputs, handle button interrupts,
 * and reset data upon request.
 */

#ifndef TOUCH_SENSOR_H
#define TOUCH_SENSOR_H

#include "Globals.h"

/**
 * @brief Initializes the touch sensor and button inputs.
 *
 * Sets up the touch sensor pin and configures the reset button with an interrupt.
 */
void setupTouchSensor();

/**
 * @brief Reads the touch sensor and updates touch counters.
 *
 * Checks if the touch sensor is activated based on the threshold and increments counters accordingly.
 */
void readTouchSensor();

/**
 * @brief Interrupt Service Routine for the reset button.
 *
 * Handles the button press and release events to determine if a data reset is requested.
 */
void IRAM_ATTR buttonISR();

/**
 * @brief Resets touch data and notifies connected clients.
 *
 * Clears touch counters, removes data files, and sends a reset message via WebSocket.
 */
void resetData();

#endif // TOUCH_SENSOR_H
