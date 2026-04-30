/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#pragma once
#include "FileSystem.hpp"
#include "Hardware.hpp"
#include "Formats.hpp"

namespace logger{
    enum class LOG_STATUS{
        LOGGING = 0,
        IDLE = 1,
        ERROR = 2
    };
    extern LOG_STATUS logging_status;

    void set_log_interval(uint32_t interval_ms);
    fs::SD_STATUS start_logging();
    void stop_logging();
    void loop();
}