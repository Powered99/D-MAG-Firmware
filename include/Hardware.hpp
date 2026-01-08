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
#include "pins.h"
#include <functional>


namespace disp{
    extern ST7735_TFT display;
    extern ST7735_TFT::TFT_PCBtype_e display_driver;

    void init_display();

}

namespace fgm{
    enum class SENSOR_MODE: int8_t {
        DISABLED = -1,
        FREQ = 0,
        ANALOG_PRECISE = 1,
        ANALOG_FAST = 2
    };
    enum class SENSOR_STATE : int8_t {
        DISABLED = -1,
        INACTIVE = 0,
        ACTIVE = 1
    };
    
    constexpr float B_MIN = -50e-6f; // Lowest value in earth's magnetic field (-50uT)
    constexpr float B_MAX = 50e-6f; // Highest value in earth's magnetic field (50uT)
    struct CALIB_DATA{
        float MIN = 8.5e-6f;
        float MAX = 25e-6f;
    };
    struct freq_sample{
        uint64_t delta_t = 0;
        uint16_t count = 0;
    };

    constexpr float ANALOG_CONVERSION_FACTOR = 3.274f / (1 << 12); // system voltage / 12bit max value

    constexpr uint8_t SENSOR_CH_COUNT = 4; // Edit to change sensor count

    constexpr uint MAX_SAMPLE_COUNT = 2048; // Max sample count for frequency measurement

    extern uint SET_SAMPLE_COUNT[SENSOR_CH_COUNT]; // Configuration buffer for sample count
    extern uint SAMPLE_COUNT[SENSOR_CH_COUNT]; // Actual used sample count (set with set_sample_count)
    extern uint MEDIAN_SAMPLE_OFFSET[SENSOR_CH_COUNT];

    extern SENSOR_MODE SENSOR_MODES[SENSOR_CH_COUNT];
    extern SENSOR_STATE SENSOR_STATES[SENSOR_CH_COUNT];
    extern CALIB_DATA SENSOR_CALIBRATIONS[SENSOR_CH_COUNT];

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
    void set_sample_count(uint8_t ch, uint sample_count);
    void load_sample_count(uint8_t ch);
    void save_sample_count(uint8_t ch);

    void calculate_nT(uint8_t ch);
    float get_nT(uint8_t ch);
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
}

namespace status{
    void init_led();
    void set_led(bool state);
}