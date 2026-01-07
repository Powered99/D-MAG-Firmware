/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#include "Math.hpp"


namespace math{
    // Extracts the median values from a sorted array - target array should have [2x offset] elements.
    void filter_median(int* target_array, int* source_array, size_t source_array_len, size_t offset) {
        if (offset * 2 > source_array_len) return;

        size_t middle_index = source_array_len / 2;
        size_t lower = middle_index - offset;

        for (size_t i = 0; i < offset * 2; i++) {
            target_array[i] = source_array[lower + i];
        }
    }

    // Extracts the median values from a sorted array - target array should have [2x offset] elements.
    void filter_median(double *target_array, double *source_array, size_t source_array_len, size_t offset) {
        if (offset * 2 > source_array_len) return;

        size_t middle_index = source_array_len / 2;
        size_t lower = middle_index - offset;

        for (size_t i = 0; i < offset * 2; i++) {
            target_array[i] = source_array[lower + i];
        }
    }


    // Returns filtered median average.
    double get_filtered_average(int *array, size_t array_length, size_t median_offset) {
        std::sort(array, array + array_length);
        const size_t median_element_count = median_offset * 2;

        int filtered_values[MAX_FILTERED_COUNT] = { 0 };
        if (median_element_count > MAX_FILTERED_COUNT)
            return 0.0; // Error: too large offset

        filter_median(filtered_values, array, array_length, median_offset);

        double sum = 0.0;
        for (int i = 0; i < median_element_count; i++) {
            sum += filtered_values[i];
        }
        return sum / (double) median_element_count;
    }

    // Returns filtered median average.
    double get_filtered_average(double* array, size_t array_length, size_t median_offset) {
        std::sort(array, array + array_length);
        const size_t median_element_count = median_offset * 2;

        double filtered_values[MAX_FILTERED_COUNT] = { 0 };
        if (median_element_count > MAX_FILTERED_COUNT)
            return 0.0; // Error: too large offset

        filter_median(filtered_values, array, array_length, median_offset);

        double sum = 0.0;
        for (int i = 0; i < median_element_count; i++) {
            sum += filtered_values[i];
        }
        return sum / (double) median_element_count;
    }
}