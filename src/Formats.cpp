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
        uint8_t active_sensors = 0;
        for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++) {
            if (fgm::SENSOR_MODES[ch] != fgm::SENSOR_MODE::DISABLED) {
                reported += SENSOR_LABELS[ch];
                active_sensors++;
                if(active_sensors >= max_sensor_channels) break; // Limit sensor channels to fit inside IAGA line width.
            }
        }
        return reported;
    }
    std::string get_sensor_count(){
        int count = 0;
        for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++) {
            if (fgm::SENSOR_MODES[ch] != fgm::SENSOR_MODE::DISABLED) count++;
        }
        return std::to_string(count);
    }
    uint8_t get_active_sensor_count(){
        uint8_t count = 0;
        for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++) {
            if (fgm::SENSOR_MODES[ch] != fgm::SENSOR_MODE::DISABLED) count++;
        }
        return count;
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

    // buf must be at least (header_content_width + 3) bytes
    void make_column_line(char* buf, size_t buf_size) {
        if (buf_size < header_content_width + 3u) return;

        char content_buf[header_content_width] = {}; // zero-init to avoid gaps being uninitialized

        int pos = snprintf(content_buf, sizeof(content_buf), "%-32s", "DATE       TIME         DOY");
        if (pos < 0 || pos >= (int)sizeof(content_buf)) return;

        uint8_t active_sensors = 0;
        for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++) {
            if (fgm::SENSOR_MODES[ch] != fgm::SENSOR_MODE::DISABLED) {
                if (pos >= (int)sizeof(content_buf)) break;

                char label[32];
                snprintf(label, sizeof(label), "%s%s", IAGA_CODE, SENSOR_LABELS[ch]);

                int written = snprintf(content_buf + pos, sizeof(content_buf) - pos, "%-10s", label);
                if (written < 0 || written >= (int)(sizeof(content_buf) - pos)) break;
                pos += written;

                active_sensors++;
                if (active_sensors >= max_sensor_channels) break;
            }
        }

        snprintf(buf, buf_size, "%-*s|\n", header_content_width + 1, content_buf);
    }

    void make_data_line(char* buf, size_t buf_size, ds3231_datetime_t dt, float* sensor_values, uint8_t sensor_count) {
        int doy = math::day_of_year(dt);

        char prefix[32];
        snprintf(prefix, sizeof(prefix), "%04d-%02d-%02d %02d:%02d:%02d.000 %03d   ", dt.year, dt.month, dt.day, dt.hour, dt.minutes, dt.seconds, doy);

        char columns[128] = {};
        int pos = 0;
        for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++) {
            if (fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::DISABLED) continue;
            float val;
            if (fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::INACTIVE)
                val = 99999.00f;
            else
                val = fgm::get_nT(ch);

            pos += snprintf(columns + pos, sizeof(columns) - pos, "%10.2f", val);
        }

        std::string data = std::string(prefix) + columns;
        snprintf(buf, buf_size, "%-70s\n", data.c_str());
    }
    // WIP
    /*std::string make_sensor_info(){
        char buf[get_active_sensor_count() * (format_line_width + 1)];
        int offset = 0;
        for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++) {
            if (fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::DISABLED) continue;
            std::string state_str = (fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::INACTIVE) ? "inactive" : "active";
            offset += snprintf(buf + offset, sizeof(buf) - offset, " # Sensor X%s: Fluxgate, parallel X-axis orientation, %s Samples%s", std::to_string(ch + 1).c_str(), std::to_string(fgm::SAMPLE_COUNT[ch]).c_str(), ch == fgm::SENSOR_CH_COUNT - 1 ? "" : "\n");
        }
        return std::string(buf);
    }*/
}