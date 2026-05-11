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

namespace IAGA2002{
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

    std::string get_reported(){
        std::string reported = "";
        uint8_t upper_bound = (LOG_ELEMENT_COUNT < max_sensor_channels) ? LOG_ELEMENT_COUNT : max_sensor_channels;
        for (uint8_t i = 0; i < upper_bound; i++) {
            reported += LOG_ELEMENTS[i].label;
            reported += " ";
        }
        return reported;
    }

    uint8_t get_active_magnetometer_count(){
        uint8_t count = 0;
        for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++) {
            if (fgm::SENSOR_MODES[ch] != fgm::SENSOR_MODE::DISABLED) count++;
        }
        return count;
    }

    std::string get_str_magnetometer_count(){
        return std::to_string(get_active_magnetometer_count());
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

        uint8_t upper_bound = (LOG_ELEMENT_COUNT < max_sensor_channels) ? LOG_ELEMENT_COUNT : max_sensor_channels;

        for (uint8_t i = 0; i < upper_bound; i++) {
            if (pos >= (int)sizeof(content_buf)) break;

            char label[32];
            snprintf(label, sizeof(label), "%s%s", IAGA_CODE, LOG_ELEMENTS[i].label);

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
    
    std::string get_sensor_samples(){
        char buf[format_line_width];
        int offset = 0;
        offset += snprintf(buf + offset, sizeof(buf) - offset, "Samples: ");

        for (uint8_t i = 0; i < LOG_ELEMENT_COUNT; i++) {
            if (LOG_ELEMENTS[i].element != ELEMENTS::MAG || fgm::SENSOR_MODES[LOG_ELEMENTS[i].channel] == fgm::SENSOR_MODE::DISABLED) continue;
            std::string state_str = (fgm::SENSOR_STATES[LOG_ELEMENTS[i].channel] == fgm::SENSOR_STATE::INACTIVE) ? "inactive" : "active";
            offset += snprintf(buf + offset, sizeof(buf) - offset, "%sX%s: %s", i == 0 ? "" : ", ", std::to_string(LOG_ELEMENTS[i].channel + 1).c_str(), std::to_string(fgm::SAMPLE_COUNT[LOG_ELEMENTS[i].channel]).c_str());
        }
        return std::string(buf);
    }
}