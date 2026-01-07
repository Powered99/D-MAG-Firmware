#pragma once
#include "FileSystem.hpp"
#include "Hardware.hpp"

namespace logger{
    enum class LOG_STATUS{
        LOGGING,
        IDLE,
        ERROR
    };
    extern LOG_STATUS logging_status;
    extern uint32_t log_interval_ms;
    extern bool LOG_CHANNELS[fgm::SENSOR_CH_COUNT];

    void set_log_interval(uint32_t interval_ms);
    fs::SD_STATUS start_logging();
    void stop_logging();
    void loop();
}