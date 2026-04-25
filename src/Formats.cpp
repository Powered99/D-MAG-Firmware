/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#include "Formats.hpp"
#include "Hardware.hpp"
#include <functional>
#include <cstring>

namespace IAGA2002{
    format_line_t make_header_entry(const char* name, std::function<std::string()> callback){
        format_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "%-*s %s", header_entry_name_length, name, "%s");
        entry.callback = callback;
        entry.dynamic = true;
        return entry;
    }
    format_line_t make_header_entry(const char* name, const char* value){
        format_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "%-*s %s", header_entry_name_length, name, value);
        return entry;
    }

    format_line_t make_header_comment(const char* value){
        format_line_t comment;
        char buf[format_line_width];
        snprintf(buf, sizeof(buf), "# %s", value);
        strncpy(comment.content, buf, sizeof(comment.content));
        return comment;
    }
    format_line_t make_header_comment_entry(const char* comment, const char* value){
        format_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "# %-*s %s", header_entry_name_length, comment, value);
        return entry;
    }
    format_line_t make_header_comment(const char* comment, std::function<std::string()> callback){
        format_line_t entry;
        snprintf(entry.content, sizeof(entry.content), "# %-*s", header_entry_name_length, comment);
        entry.callback = callback;
        entry.dynamic = true;
        return entry;
    }

    std::string get_reported(){
        std::string reported = "";
        for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++) {
            if (fgm::SENSOR_MODES[ch] != fgm::SENSOR_MODE::DISABLED) {
                if(ch > max_sensor_channels) continue; // Limit sensor channels to fit inside IAGA line width.
                reported += 'X';
                reported += ('1' + ch);
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
    void make_header_line(char* buf, size_t buf_size, format_line_t line){
        if(line.dynamic){
            std::string value = line.callback();
            snprintf(buf, buf_size, line.content, value.c_str());
        }else{
            snprintf(buf, buf_size, "%s", line.content);
        }
        buf[format_line_width -1] = '|'; // Pipe at the end of line
        buf[format_line_width] = '\n'; // Add new line character
    }

    // buf must be at least 72 bytes
    // Create column header line, eg.: DATE      TIME        DOY     X1      X2      X3     X4
    void make_column_line(char* buf, size_t buf_size){
        snprintf(buf, buf_size, "%-27s", "DATE       TIME         DOY");
        int pos = 32; // Start position of channel columns, after "DATE TIME DOY" part
        uint8_t upper_bound = (fgm::SENSOR_CH_COUNT > IAGA2002::max_sensor_channels) ? IAGA2002::max_sensor_channels : fgm::SENSOR_CH_COUNT;
        for(int ch = 0; ch < upper_bound; ch++){
            if(fgm::SENSOR_MODES[ch] != fgm::SENSOR_MODE::DISABLED){
                pos += snprintf(buf + pos, buf_size - pos, "X%d      ", ch);
            }
        }
        buf[format_line_width -1] = '|'; // Pipe at the end of line
        buf[format_line_width] = '\n'; // Add new line character
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