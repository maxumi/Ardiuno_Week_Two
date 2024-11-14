/**
 * @file DataAnalysis.h
 * @brief Header file for data analysis functions.
 *
 * Contains the declaration of the function responsible for performing periodic data analysis and saving results.
 */

#ifndef DATA_ANALYSIS_H
#define DATA_ANALYSIS_H

#include "Globals.h"

/**
 * @brief Performs periodic data analysis and saves results.
 *
 * This function calculates touch rates, appends data to a CSV file, limits the number of entries,
 * and resets the touch event counter for the next interval.
 */
void performAnalysis();

#endif // DATA_ANALYSIS_H
