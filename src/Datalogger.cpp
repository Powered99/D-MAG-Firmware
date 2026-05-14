/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#include "Datalogger.hpp"
#include "Hardware.hpp"
#include "Config.hpp"
#include "Formats.hpp"
#include "Math.hpp"

namespace logger{
    LOG_STATUS logging_status = LOG_STATUS::IDLE;
    
    uint8_t log_start_hour;
    
    void set_log_interval(uint32_t interval_ms){
        log_interval_ms = interval_ms;
    }

    // Write entire IAGA2002 header
    void iaga_header() {
        char buf[72];
        fs::SD_STATUS status;

        // Header entry lines
        for(uint8_t i = 0; i < IAGA2002::HEADER_ENTRY_COUNT; i++){
            IAGA2002::make_header_line(buf, sizeof(buf), IAGA2002::HEADER[i]);
            status = fs::write_file(buf);
            if(status == fs::SD_STATUS::SD_ERR){
                logging_status = LOG_STATUS::ERROR;
                return;
            }
        }
        // Header comment lines
        for(uint8_t i = 0; i < IAGA2002::COMMENT_ENTRY_COUNT; i++){
            IAGA2002::make_header_line(buf, sizeof(buf), IAGA2002::COMMENTS[i]);
            status = fs::write_file(buf);
            if(status == fs::SD_STATUS::SD_ERR){
                logging_status = LOG_STATUS::ERROR;
                return;
            }
        }
        IAGA2002::make_column_line(buf, sizeof(buf));
        // Write column header line to file
        status = fs::write_file(buf);
        if(status == fs::SD_STATUS::SD_ERR){
            logging_status = LOG_STATUS::ERROR;
            return;
        }
    }


    // Write single IAGA2002 line
    fs::SD_STATUS iaga_line(ds3231_datetime_t dt) {
        char buf[72];
        uint8_t logged_element_count = (LOG_ELEMENT_COUNT < IAGA2002::max_sensor_channels) ? LOG_ELEMENT_COUNT : IAGA2002::max_sensor_channels;
        float sensor_values[logged_element_count];
        for(uint8_t i = 0; i < logged_element_count; i++){
            // Dummy value
            float value = 99999.0f;

            // Magnetometer data
            if(LOG_ELEMENTS[i].element == ELEMENTS::MAG && fgm::SENSOR_STATES[LOG_ELEMENTS[i].channel] == fgm::SENSOR_STATE::ACTIVE){
                value = fgm::get_nT(LOG_ELEMENTS[i].channel);   
            }
            // Magnetometer difference data
            else if(LOG_ELEMENTS[i].element == ELEMENTS::MAG_DIFF && fgm::SENSOR_STATES[LOG_ELEMENTS[i].channel] == fgm::SENSOR_STATE::ACTIVE && fgm::SENSOR_STATES[LOG_ELEMENTS[i].channel2] == fgm::SENSOR_STATE::ACTIVE){
                value = fgm::get_nT(LOG_ELEMENTS[i].channel) - fgm::get_nT(LOG_ELEMENTS[i].channel2);
            }
            // Raw frequency data
            else if(LOG_ELEMENTS[i].element == ELEMENTS::FREQ && fgm::SENSOR_STATES[LOG_ELEMENTS[i].channel] == fgm::SENSOR_STATE::ACTIVE){
                value = fgm::get_hz(LOG_ELEMENTS[i].channel);
            }
            // Raw voltage data
            else if(LOG_ELEMENTS[i].element == ELEMENTS::VOLTS && fgm::SENSOR_STATES[LOG_ELEMENTS[i].channel] == fgm::SENSOR_STATE::ACTIVE){
                value = fgm::get_volts(LOG_ELEMENTS[i].channel);
            // Voltage difference data
            }else if(LOG_ELEMENTS[i].element == ELEMENTS::VOLTS_DIFF && fgm::SENSOR_STATES[LOG_ELEMENTS[i].channel] == fgm::SENSOR_STATE::ACTIVE && fgm::SENSOR_STATES[LOG_ELEMENTS[i].channel] == fgm::SENSOR_STATE::ACTIVE){
                value = fgm::get_volts(LOG_ELEMENTS[i].channel) - fgm::get_volts(LOG_ELEMENTS[i].channel2);
            }
            // Temperature data
            else if(LOG_ELEMENTS[i].element == ELEMENTS::TEMP){
                value = rtc::get_temperature();
            }

            sensor_values[i] = value;
        }
        IAGA2002::make_data_line(buf, sizeof(buf), dt, sensor_values, logged_element_count);
        fs::SD_STATUS status = fs::write_file(buf);    
        return status;
    }



    fs::SD_STATUS start_logging(){
        fs::init_sd();

        if(!fs::sd_available){
            logging_status = LOG_STATUS::ERROR;
            return fs::SD_STATUS::SD_ERR;
        } 
        
        char buf[48];
        // Generate file name
        
        ds3231_datetime_t dt;
        rtc::get_datetime(&dt);

        log_start_hour = dt.hour;

        snprintf(buf, sizeof(buf), "%04d%02d%02d%02d%02d%02d.txt", dt.year, dt.month, dt.day, dt.hour, dt.minutes, dt.seconds);

        fs::SD_STATUS sd_status = fs::open_file(fs::SD_MODE::SD_WRITE_APPEND, buf);

        switch(DATA_FORMAT){
            case FORMATS::IAGA2002: iaga_header(); break;
            //case FORMATS::DMAG2026: dmag_header(); break;
        }
        //}
        logging_status = (sd_status == fs::SD_STATUS::SD_ERR) ? LOG_STATUS::ERROR : LOG_STATUS::LOGGING;
        nvm::save();
        return sd_status;
    }
    void stop_logging(){
        fs::close_file();
        if(logging_status != LOG_STATUS::ERROR) logging_status = LOG_STATUS::IDLE;
        nvm::save();
        //fs::unmount_sd();
    }
    
    void log(){  
        ds3231_datetime_t dt;
        rtc::get_datetime(&dt);

        fs::SD_STATUS status;

        switch(DATA_FORMAT){
            case FORMATS::IAGA2002: status = iaga_line(dt); break; // iaga_line
            //case FORMATS::DMAG2026: break; // dmag_line WIP
        };
        
        // start new file when new hour starts
        if(dt.hour != log_start_hour){
            stop_logging();
            sleep_ms(100);
            start_logging();
        }
        
        if(status == fs::SD_STATUS::SD_ERR) logging_status = LOG_STATUS::ERROR;
    }
    void loop(){
        static uint64_t last_log_time = get_absolute_time();
        if(logging_status == LOG_STATUS::LOGGING && get_absolute_time() - last_log_time >= log_interval_ms * 1000){
            status::set_led(true);
            log();
            status::set_led(false);
            last_log_time = get_absolute_time();
        }
    }
}