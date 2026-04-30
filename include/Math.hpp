/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#pragma once

#include <algorithm>
#include "pico/stdlib.h"
#include "Hardware.hpp"

namespace math{
    void filter_median(int* target_array, int* source_array, size_t source_array_len, size_t offset); // Extracts the median values from a sorted array - target array should have [2x offset] elements.
    void filter_median(double *target_array, double *source_array, size_t source_array_len, size_t offset); // Extracts the median values from a sorted array - target array should have [2x offset] elements.

    double get_filtered_average(int *array, size_t array_length, size_t median_offset); // Returns filtered median average.
    double get_filtered_average(double* array, size_t array_length, size_t median_offset); // Returns filtered median average.

    constexpr size_t MAX_FILTERED_COUNT = 128;

    int day_of_year(int year, int month, int day);
    int day_of_year(ds3231_datetime_t dt);
};

