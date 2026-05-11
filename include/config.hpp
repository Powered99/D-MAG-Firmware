/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#pragma once

#include <stdlib.h>
#include "pico/stdlib.h"

#define FIRMWARE_VERSION "1.9 BETA"

namespace fgm{
    enum class SENSOR_MODE: int8_t {
        DISABLED = -1,
        FREQ = 0,
        HARMONIC = 1,
        ANALOG = 2
    };
        
    constexpr float B_MIN = -50e-6f; // Lowest value in earth's magnetic field (-50uT)
    constexpr float B_MAX = 50e-6f; // Highest value in earth's magnetic field (50uT)
    struct CALIB_DATA{
        float MIN = 8.5e-6f;
        float MAX = 25e-6f;
        double offset = 0.0f;
        double slope = 0.0f;
    };

    constexpr uint8_t SENSOR_CH_COUNT = 4; // Edit to change sensor count
    constexpr uint16_t MIN_IMPULSES_COUNT = 100; // Required impulses per sample - higher numbers can improve precision at the cost of speed.
    constexpr uint64_t FREQ_TIMEOUT = 50000; // If no impulse after 50ms, set to inactive.

    extern volatile SENSOR_MODE SENSOR_MODES[SENSOR_CH_COUNT];
    extern volatile uint SAMPLE_COUNT[SENSOR_CH_COUNT]; // Actual used sample count (set with set_sample_count)
    extern volatile uint MEDIAN_SAMPLE_OFFSET[SENSOR_CH_COUNT]; // Offset of median samples (MEDIAN_SAMPLE_OFFSET <- center -> MEDIAN_SAMPLE_OFFSET), total median samples: 2x offset
    extern CALIB_DATA SENSOR_CALIBRATIONS[SENSOR_CH_COUNT];
    
}
namespace logger{
    extern uint32_t log_interval_ms;
    enum class FORMATS{
        IAGA2002,
        DMAG2026
    };
    extern FORMATS DATA_FORMAT;
}

