/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#pragma once

#include "displaylib_16/st7735.hpp"
#include "ds3231.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "pico/multicore.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pins.hpp"
#include <functional>
#include "Config.hpp"
#include "Datalogger.hpp"


namespace disp{
    extern ST7735_TFT display;
    extern ST7735_TFT::TFT_PCBtype_e display_driver;

    void init_display();

}

namespace fgm{
    enum class SENSOR_STATE : int8_t {
        DISABLED = -1,
        INACTIVE = 0,
        ACTIVE = 1
    };
    struct freq_sample{
        uint64_t delta_t = 0;
        uint16_t count = 0;
    };

    constexpr float ANALOG_CONVERSION_FACTOR = 3.274f / (1 << 12); // system voltage / 12bit max value

    constexpr uint MAX_SAMPLE_COUNT = 2048; // Max sample count for frequency measurement

    extern uint SET_SAMPLE_COUNT[SENSOR_CH_COUNT]; // Configuration buffer for sample count
    extern uint SET_MEDIAN_SAMPLE_OFFSET[SENSOR_CH_COUNT]; // Configuration buffer for median sample offset
    
    extern SENSOR_STATE SENSOR_STATES[SENSOR_CH_COUNT];
    
    extern uint16_t sample_index[SENSOR_CH_COUNT];

    extern double periods[SENSOR_CH_COUNT]; // delta between sensor output ticks
    extern double frequencies[SENSOR_CH_COUNT]; // sensor output frequencies (in Hz)
    extern float voltages[SENSOR_CH_COUNT]; // sensor output voltages (in V)
    extern float readings[SENSOR_CH_COUNT]; // Sensor outputs (in nT)
    
    void init_sensor(size_t ch, SENSOR_MODE mode);
    void init_sensors();
    void read_sensors();
    void deactivate_sensor(size_t ch);

    void set_sensor_mode(uint8_t ch, SENSOR_MODE mode); // Will only work before initialization!
    void set_sensor_modes(SENSOR_MODE modes[SENSOR_CH_COUNT]); // Will only work before initialization!
    void set_sensor_calib(uint8_t ch, CALIB_DATA calib_data);
    void set_sensor_calibs(CALIB_DATA calib_data[SENSOR_CH_COUNT]);

    void set_sample_count(uint8_t ch, uint sample_count); // Sets the used sample count to the given value.
    void set_sample_counts(uint sample_counts[SENSOR_CH_COUNT]); // Sets all used sample counts to the given array of counts.
    void load_sample_count(uint8_t ch); // Sets the used sample count to the corresponding value from the SET_SAMPLE_COUNT buffer.
    void load_sample_counts(); // Sets all the used sample counts to the corresponding values from the SET_SAMPLE_COUNT buffer.
    void save_sample_count(uint8_t ch); // Saves the current sample count to the SET_SAMPLE_COUNT buffer.
    void save_sample_counts(); // Saves all the current sample counts to the SET_SAMPLE_COUNT buffer.

    void set_median_offset(uint8_t ch, uint sample_count); // Sets the used median sample offset to the given value.
    void set_median_offsets(uint sample_counts[SENSOR_CH_COUNT]); // Sets the used median sample offsets to the given array of offsets.
    void load_median_offset(uint8_t ch); // Sets the used median sample offset to the corresponding value from the SET_MEDIAN_SAMPLE_OFFSET buffer.
    void load_median_offsets(); // Sets all the used median sample offsets to the corresponding values from the SET_MEDIAN_SAMPLE_OFFSET buffer.
    void save_median_offset(uint8_t ch); // Saves the current median sample offset to the SET_MEDIAN_SAMPLE_OFFSET buffer.
    void save_median_offsets(); // Saves all the current median sample offsets to the SET_MEDIAN_SAMPLE_OFFSET buffer.

    void calculate_nT(uint8_t ch);
    float get_nT(uint8_t ch);
}

namespace non_volatile_memory{

}

namespace ctrl{
    enum class BUTTON_t{
        BTN_LEFT,
        BTN_SELECT,
        BTN_RIGHT
    };
    enum class BTN_EVENT_t{
        BTN_DOWN,
        BTN_UP,
        BTN_PRESSED,
        BTN_RELEASED,
        BTN_HOLD
    };

    struct BTN_EVENT{
        size_t id;
        bool enabled;
        BUTTON_t button;
        BTN_EVENT_t event;
        std::function<void()> callback;
    };

    constexpr uint64_t BTN_HOLD_TRIGGER_TIME = 1000 * 1000; // 1s

    size_t connect(BUTTON_t button, BTN_EVENT_t event, std::function<void()> callback); // Returns id of event
    void emit(BTN_EVENT &event);
    void set_enabled(size_t id, bool enabled);
    void disconnect(size_t id);

    void handle_events();
    void update_btn_states();
    void update_prev_btn_states();

    void init_btn();
    int get_state(BUTTON_t button);
    int get_state(size_t button_index);

}
namespace rtc{
    extern ds3231_rtc ds3231;
    void init_rtc();
    void set_datetime(ds3231_datetime_t *datetime);
    void get_datetime(ds3231_datetime_t *datetime);
    uint64_t get_micros();
    void loop();
    float get_temperature();
}

namespace status{
    void init_led();
    void set_led(bool state);
    void set_led(bool state, absolute_time_t duration);
    void loop();
}

namespace nvm { // Non-volatile-memory (flash) storage for settings / configurations / calibrations

    constexpr uint32_t FLASH_OFFSET = PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
    constexpr uint32_t MAGIC        = 0xD4A60001;
    constexpr uint16_t VERSION      = 2;
    constexpr uint8_t  MAX_CH       = 6;

    struct CalibEntry {
        double offset, slope;
        float  min, max;
    };

    struct Block {
        uint32_t magic;
        uint16_t version;
        uint8_t  channel_count;
        uint8_t  _pad;
        CalibEntry calibrations[MAX_CH];
        int8_t     sensor_modes[MAX_CH];
        uint32_t   sample_counts[MAX_CH];
        uint32_t   median_sample_offsets[MAX_CH];
        uint8_t    data_format;
        uint8_t    _pad2[3];
        uint32_t   log_interval_ms;
        uint32_t   checksum;
        uint8_t    logging_status;
    };

    static_assert(sizeof(Block) <= FLASH_PAGE_SIZE, "nvm::Block exceeds one flash page");

    void save();
    bool check();
    bool load();
    void load_defaults();

}