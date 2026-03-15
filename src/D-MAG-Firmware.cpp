/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */


// Standard libs
#include <stdio.h>
#include "pico/stdlib.h"

// System components
#include "Hardware.hpp"
#include "FileSystem.hpp"
#include "Graphics.hpp"
#include "UserInterface.hpp"
#include "Datalogger.hpp"
#include "Math.hpp"


void blink(size_t ms){
    static absolute_time_t last_led_toggle = get_absolute_time();
    static bool led_state = false;

    if(get_absolute_time() - last_led_toggle >= ms * 1000){
        last_led_toggle = get_absolute_time();
        led_state = !led_state;
        status::set_led(led_state);
    }
}

// Main function
int main()
{
    stdio_init_all();
    
    status::init_led();

    fgm::set_sensor_modes((fgm::SENSOR_MODE[fgm::SENSOR_CH_COUNT]){fgm::SENSOR_MODE::FREQ, fgm::SENSOR_MODE::FREQ, fgm::SENSOR_MODE::DISABLED, fgm::SENSOR_MODE::DISABLED});
    fgm::init_sensors();
    disp::init_display();
    rtc::init_rtc();
    
    /*ds3231_datetime_t dt = {
        .hour = 23,
        .minutes = 23,
        .seconds = 30,
        .day = 6,
        .dotw = 2,
        .month = 1,
        .year = 2026,
    };

    rtc::set_datetime(&dt);*/

    
    fs::init_sd();
    
    ctrl::init_btn();
    ui::init();
    
    while (true) {
        ctrl::handle_events();
        fgm::read_sensors();
        rtc::loop();
        ui::draw_page();
        logger::loop();
    }
}
