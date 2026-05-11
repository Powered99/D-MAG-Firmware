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

// Temporary solution to set RTC time until the setting gets implemented, uncomment the function call, change the time data, and compile & flash. 
// Make sure to then re-comment the call and flash again to prevent the rtc being reset to this time on each boot!
void set_rtc(){
    ds3231_datetime_t dt = {
        .hour = 21,
        .minutes = 22,
        .seconds = 40,
        .day = 11,
        .dotw = 1,
        .month = 5,
        .year = 2026,
    };
    rtc::set_datetime(&dt);
}

inline void dont(){
    ui::set_page(6); // Go to Datalogger page if logging was active before reboot
    logger::start_logging(); // Recover logging state after reboot
}

// Main function
int main()
{
    // Hardware & Driver initialization
    stdio_init_all();
    status::init_led(); // nvm uses status, initialize beforehand
    if (!nvm::load()) nvm::load_defaults(); // Load from NVM or defaults when failed
    if(logger::logging_status == logger::LOG_STATUS::ERROR){
        logger::logging_status = logger::LOG_STATUS::IDLE; // Reset logging status if it was in error state, to prevent lockout from logging
    }
    fgm::init_sensors();
    disp::init_display();
    rtc::init_rtc();
    fs::init_sd();
    ctrl::init_btn();
    ui::init();
    
    bool die = logger::logging_status == logger::LOG_STATUS::LOGGING;
    
    if(die){
        dont();
    }


    //set_rtc();
    
    fgm::launch_polling();

    // Main loop
    while (true) {
        ctrl::handle_events();
        //fgm::read_sensors(); // Now handled on core1, started in fgm::launch_polling();
        rtc::loop();
        ui::draw_page();
        logger::loop();
        status::loop();
    }
}
