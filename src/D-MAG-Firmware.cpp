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
    rtc::init_rtc(); // Already initializes i2c, call before other i2c devices (like fgm's ADS1115 if enabled)
    fgm::init_sensors();
    disp::init_display();
    fs::init_sd();
    ctrl::init_btn();
    ui::init();
    fgm::launch_polling();
    
    bool die = logger::logging_status == logger::LOG_STATUS::LOGGING;
    
    if(die){
        dont();
    }

    // Main loop
    while (true) {
        ctrl::handle_events();
        fgm::read_channels_ads1115();
        rtc::loop();
        ui::draw_page();
        logger::loop();
        status::loop();
    }
}
