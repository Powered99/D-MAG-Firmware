/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#include "Formats.hpp"
#include "Hardware.hpp"
#include "Math.hpp"
#include <functional>
#include <cstring>

namespace formats{
    bool sensor_reported[fgm::SENSOR_CH_COUNT] = {false};
    uint8_t reported_sensor_count = 0;

    void init_sensor_metadata(){
        for(uint8_t i = 0; i < LOG_ELEMENT_COUNT; i++){
            ELEMENTS element = LOG_ELEMENTS[i].element;
            bool element_invalid = (element == ELEMENTS::TEMP);
            if(element_invalid) continue; // Skip non-channel-magnetometer related elements

            uint8_t ch = LOG_ELEMENTS[i].channel;
            fgm::SENSOR_MODE mode = fgm::SENSOR_MODES[ch];
            
            if(mode != fgm::SENSOR_MODE::DISABLED && !sensor_reported[ch]){
                sensor_reported[ch] = true;
                reported_sensor_count++;
            }

            if(!(element == ELEMENTS::MAG_DIFF || element == ELEMENTS::VOLTS_DIFF)) continue; // Only applies to elements with 2nd channel

            ch = LOG_ELEMENTS[i].channel2;
            mode = fgm::SENSOR_MODES[ch];

            if(mode != fgm::SENSOR_MODE::DISABLED && !sensor_reported[ch]){
                sensor_reported[ch] = true;
                reported_sensor_count++;
            }
        }
    }

    uint8_t get_active_magnetometer_count(){
        return reported_sensor_count;
    }

    std::string get_str_magnetometer_count(){
        return std::to_string(get_active_magnetometer_count());
    }

    std::string get_sensor_samples(){
        char buf[64];
        int offset = 0;
        offset += snprintf(buf + offset, sizeof(buf) - offset, "Samples: ");

        uint8_t j = 0;
        for (uint8_t ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++) {
            if(!sensor_reported[ch]) continue;

            fgm::SENSOR_MODE mode = fgm::SENSOR_MODES[ch];
            bool mode_invalid = (mode == fgm::SENSOR_MODE::ANALOG_ADS1115 || mode == fgm::SENSOR_MODE::HARMONIC);
            if(mode_invalid) continue;

            std::string state_str = (fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::INACTIVE) ? "inactive" : "active";
            offset += snprintf(buf + offset, sizeof(buf) - offset, "%sCH%d: %d", j++ == 0 ? "" : ", ", ch + 1, fgm::SAMPLE_COUNT[ch]);
        }
        return std::string(buf);
    }

    std::string get_sensor_median_samples(){
        char buf[64];
        int offset = 0;
        offset += snprintf(buf + offset, sizeof(buf) - offset, "Median samples: ");

        uint8_t j = 0;
        for (uint8_t ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++) {
            if(!sensor_reported[ch]) continue;

            fgm::SENSOR_MODE mode = fgm::SENSOR_MODES[ch];
            bool mode_invalid = (mode == fgm::SENSOR_MODE::ANALOG_ADS1115 || mode == fgm::SENSOR_MODE::HARMONIC);
            if(mode_invalid) continue;

            std::string state_str = (fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::INACTIVE) ? "inactive" : "active";
            offset += snprintf(buf + offset, sizeof(buf) - offset, "%sCH%d: %d", j++ == 0 ? "" : ", ", ch + 1, fgm::MEDIAN_SAMPLE_OFFSET[ch] * 2);
        }
        return std::string(buf);
    }

    std::string get_element_channels(){
        std::string buffer = "";
        uint8_t upper_bound = logger::DATA_FORMAT == logger::FORMATS::IAGA2002 ? IAGA2002::max_data_columns : LOG_ELEMENT_COUNT;
        for(uint8_t i = 0; i < upper_bound; i++){
            ELEMENTS element = LOG_ELEMENTS[i].element;
            if(element == ELEMENTS::TEMP) continue;

            if(i > 0) buffer += " ";
            buffer += LOG_ELEMENTS[i].label;
            buffer += "->CH"+std::to_string(LOG_ELEMENTS[i].channel);
            if(element == ELEMENTS::MAG_DIFF || element == ELEMENTS::VOLTS_DIFF)
                buffer += ",CH"+std::to_string(LOG_ELEMENTS[i].channel2);
            
        }
        return buffer;
    }

    std::string get_datetime(){
        ds3231_datetime_t dt;
        rtc::get_datetime(&dt);
        char buf[64];
        int millis = static_cast<int>((rtc::get_micros() / 1000) % 1000);
        int doy = math::day_of_year(dt);
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d (day %d)",
             dt.year, dt.month, dt.day, dt.hour, dt.minutes, dt.seconds, millis, doy);
        return std::string(buf);
    }

    std::string get_reported(){
        std::string reported = "";
        uint8_t upper_bound = logger::DATA_FORMAT == logger::FORMATS::IAGA2002 ? IAGA2002::max_data_columns : LOG_ELEMENT_COUNT;
        for (uint8_t i = 0; i < upper_bound; i++) {
            reported += formats::LOG_ELEMENTS[i].label;
            reported += " ";
        }
        return reported;
    }
}

namespace formats::IAGA2002{
    header_line_t make_header_entry(const char* name, std::function<std::string()> callback){
        header_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "%-*s %s", header_entry_name_length, name, "%s");
        entry.callback = callback;
        entry.dynamic = true;
        return entry;
    }
    header_line_t make_header_entry(const char* name, const char* value){
        header_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "%-*s %s", header_entry_name_length, name, value);
        return entry;
    }

    header_line_t make_header_comment(const char* value){
        header_line_t comment;
        char buf[format_line_width];
        snprintf(buf, sizeof(buf), "# %s", value);
        strncpy(comment.content, buf, sizeof(comment.content));
        return comment;
    }
    header_line_t make_header_comment_entry(const char* comment, const char* value){
        header_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "# %-*s %s", header_entry_name_length, comment, value);
        return entry;
    }
    header_line_t make_header_comment(const char* comment, std::function<std::string()> callback){
        header_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "# %-*s", header_entry_name_length, comment);
        entry.callback = callback;
        entry.dynamic = true;
        return entry;
    }
    // buf must be at least 72 bytes
    void make_header_line(char* buf, size_t buf_size, header_line_t line){
        char content_buf[header_content_width];
        if(line.dynamic){
            std::string value = line.callback();
            snprintf(content_buf, sizeof(content_buf), line.content, value.c_str());
        }else{
            snprintf(content_buf, sizeof(content_buf), "%s", line.content);
        }
        
        snprintf(buf, buf_size, " %-*s|\n", header_content_width, content_buf); // Construct full string (content, pipe, newline)
    }

    // buf must be at least 72 bytes
    void make_column_line(char* buf, size_t buf_size) {
        if (buf_size < 72) return;

        char content_buf[header_content_width + 2] = {}; // + 2 for pipe and null terminator

        int pos = snprintf(content_buf, sizeof(content_buf), "%-32s", "DATE       TIME         DOY");
        if (pos < 0 || pos >= (int)sizeof(content_buf)) return;

        uint8_t upper_bound = (formats::LOG_ELEMENT_COUNT < max_data_columns) ? formats::LOG_ELEMENT_COUNT : max_data_columns;

        for (uint8_t i = 0; i < upper_bound; i++) {
            if (pos >= (int)sizeof(content_buf)) break;

            char label[32];
            snprintf(label, sizeof(label), "%s%s", IAGA_CODE, formats::LOG_ELEMENTS[i].label);

            int written = snprintf(content_buf + pos, sizeof(content_buf) - pos, "%-10s", label);
            if (written < 0 || written >= (int)(sizeof(content_buf) - pos)) break;
            pos += written;
        }

        snprintf(buf, buf_size, "%-*s|\n", header_content_width + 1, content_buf); // header_content_width to account for the extra space, which in this case is left out.
    }

    void make_data_line(char* buf, size_t buf_size, ds3231_datetime_t dt, float* element_values, uint8_t element_count) {
        int doy = math::day_of_year(dt);

        char prefix[64];
        int millis = static_cast<int>((rtc::get_micros() / 1000) % 1000);
        int doy_int = static_cast<int>(doy);
        snprintf(prefix, sizeof(prefix), "%04d-%02d-%02d %02d:%02d:%02d.%03d %03d   ",
             dt.year, dt.month, dt.day, dt.hour, dt.minutes, dt.seconds, millis, doy_int);

        char columns[128] = {};
        int pos = 0;
        float val;

        for (uint8_t i = 0; i < element_count; i++) {
            val = element_values[i];
            pos += snprintf(columns + pos, sizeof(columns) - pos, "%10.2f", val);
        }

        std::string data = std::string(prefix) + columns;
        snprintf(buf, buf_size, "%-70s\n", data.c_str());
    }
}

namespace formats::DMAG2026{
    header_line_t make_header_entry(const char* name, std::function<std::string()> callback){
        header_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "%-*s %s", header_entry_name_length, name, "%s");
        entry.callback = callback;
        entry.dynamic = true;
        return entry;
    }
    header_line_t make_header_entry(const char* name, const char* value){
        header_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "%-*s %s", header_entry_name_length, name, value);
        return entry;
    }
    header_line_t make_header_comment(const char* value){
        header_line_t comment;
        char buf[header_line_width];
        snprintf(buf, sizeof(buf), "# %s", value);
        strncpy(comment.content, buf, sizeof(comment.content));
        return comment;
    }
    header_line_t make_header_comment_entry(const char* comment, const char* value){
        header_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "# %-*s %s", header_entry_name_length, comment, value);
        return entry;
    }
    header_line_t make_header_comment(const char* comment, std::function<std::string()> callback){
        header_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "# %-*s", std::min(strlen(comment), (size_t)header_entry_name_length), comment);
        entry.callback = callback;
        entry.dynamic = true;
        return entry;
    }
    // buf must be at least header_line_width + 2 bytes
    void make_header_line(char* buf, size_t buf_size, header_line_t line){
        char content_buf[header_content_width];
        if(line.dynamic){
            std::string value = line.callback();
            snprintf(content_buf, sizeof(content_buf), line.content, value.c_str());
        }else{
            snprintf(content_buf, sizeof(content_buf), "%s", line.content);
        }
        
        snprintf(buf, buf_size, "|%-*s\n", std::min(strlen(content_buf), (size_t)header_content_width), content_buf); // Construct full string (pipe, content, newline)
    }

    // buf must be at least header_line_width + 2 bytes bytes
    void make_column_line(char* buf, size_t buf_size) {
        if (buf_size < header_line_width + 2) return;

        char content_buf[header_content_width + 1] = {}; // + 1 for null terminator

        int pos = snprintf(content_buf, sizeof(content_buf), "DOY\tTIME");
        if (pos < 0 || pos >= (int)sizeof(content_buf)) return;

        for (uint8_t i = 0; i < formats::LOG_ELEMENT_COUNT; i++) {
            if (pos >= (int)sizeof(content_buf)) break;

            char label[32];
            snprintf(label, sizeof(label), "%s%s", IAGA_CODE, formats::LOG_ELEMENTS[i].label);

            int written = snprintf(content_buf + pos, sizeof(content_buf) - pos, "\t%s", label);
            if (written < 0 || written >= (int)(sizeof(content_buf) - pos)) break;
            pos += written;
        }

        snprintf(buf, buf_size, "%-*s\n", std::min(strlen(content_buf), (size_t)header_content_width), content_buf); // header_content_width to account for the extra space, which in this case is left out.
    }

    // buf must be at least data_line_width + 2 in size.
    void make_data_line(char* buf, size_t buf_size, ds3231_datetime_t dt, float* data_values, uint8_t data_column_count){
        
        int millis = static_cast<int>((rtc::get_micros() / 1000) % 1000);
        int doy = math::day_of_year(dt);

        char data_line[data_line_width + 2]; // 128 data line characters + new line + null terminator
        snprintf(data_line, sizeof(data_line), "%03d\t%02d:%02d:%02d.%03d", doy, dt.hour, dt.minutes, dt.seconds, millis);
        
        char columns[data_line_width] = {};
        int pos = 0;
        float val;

        for (uint8_t i = 0; i < data_column_count; i++) {
            val = data_values[i];
            pos += snprintf(columns + pos, sizeof(columns) - pos, "\t%.4f", val);
        }

        std::strcat(data_line, columns);
        snprintf(buf, buf_size, "%-*s\n", std::min((size_t)data_line_width, std::strlen(data_line)), data_line); // Crop text into 128 characters, add newline.
    }
}