/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#include "Datalogger.hpp"
#include "Hardware.hpp"

namespace logger{
    LOG_STATUS logging_status = LOG_STATUS::IDLE;
    bool LOG_CHANNELS[fgm::SENSOR_CH_COUNT];
    uint32_t log_interval_ms = 1000;

    uint8_t log_start_hour;
    

    void set_log_interval(uint32_t interval_ms){
        log_interval_ms = interval_ms;
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

        // 1st row: eg. Time CH0 CH1 CH2...
        if(sd_status != fs::SD_STATUS::SD_ERR){
            snprintf(buf, sizeof(buf),(char*)"Time");
            for(int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++){
                if(fgm::SENSOR_MODES[ch] != fgm::SENSOR_MODE::DISABLED){
                    LOG_CHANNELS[ch] = true;
                    char chbuf[10];
                    snprintf(chbuf, sizeof(chbuf), "\tCH%d",ch);
                    strcat(buf, chbuf);
                }else{
                    LOG_CHANNELS[ch] = false;
                }
            }
            
            sd_status = fs::write_file(buf);
        }

        logging_status = (sd_status == fs::SD_STATUS::SD_ERR) ? LOG_STATUS::ERROR : LOG_STATUS::LOGGING;
        return sd_status;
    }
    void stop_logging(){
        fs::close_file();
        if(logging_status == LOG_STATUS::ERROR) return;
        logging_status = LOG_STATUS::IDLE;
        //fs::unmount_sd();
    }
    void log(){
        char buf[100];

        ds3231_datetime_t dt;
        rtc::get_datetime(&dt);
        
        snprintf(buf, sizeof(buf), "\n%02d:%02d:%02d:%03d", dt.hour, dt.minutes, dt.seconds, rtc::get_micros()/1000);
        for(int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++){
            if(LOG_CHANNELS[ch]){
                char ch_buf[16];
                if(fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::ACTIVE) snprintf(ch_buf, sizeof(ch_buf), "\t%f",fgm::get_nT(ch));
                else if(fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::INACTIVE) snprintf(ch_buf, sizeof(ch_buf), "\tINACTIVE");
                else snprintf(ch_buf, sizeof(ch_buf), "\tDISABLED");
                strcat(buf, ch_buf);
            }
        }

        // start new file when new hour starts
        rtc::get_datetime(&dt);
        if(dt.hour != log_start_hour){
            stop_logging();
            sleep_ms(100);
            start_logging();
        }
        
        

        fs::SD_STATUS status = fs::write_file(buf);
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