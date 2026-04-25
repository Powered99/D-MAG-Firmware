/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#pragma once

#include "Hardware.hpp"
#include <cstring>
#include <functional>
#include "config.hpp"

// IAGA2002 header metadata
#define SOURCE_OF_DATA "HvF Braunschweig"
#define STATION_NAME "HvF-BS 3G-MAG-2"
#define IAGA_CODE "HVF"
// IAGA2002 header geodetic location
#define GEODETIC_LATITUDE 52.26638129304967
#define GEODETIC_LONGITUDE 10.498487938605944
#define ELEVATION 72
// IAGA2002 header sensor information
#define SENSOR_ORIENTATION "X-Component"
#define DATA_TYPE "definitive"

enum class FORMATS{
    IAGA2002,
    DMAG2026
};

namespace IAGA2002{
    const uint8_t header_entry_name_length = 22;
    const uint8_t header_entry_value_length = 48;
    const uint8_t format_line_width = 70;
    const uint8_t max_sensor_channels = 4; // Max. number of sensor channels that can be reported in the IAGA file

    struct format_line_t{
        char content[format_line_width]; // %s marker will get replaced by returned value from callback.
        std::function<std::string()> callback;
        bool dynamic = false;
    };

    format_line_t make_header_entry(const char* name, std::function<std::string()> callback);
    format_line_t make_header_entry(const char* name, const char* value);
    format_line_t make_header_comment(const char* value);
    format_line_t make_header_comment_entry(const char* comment, const char* value);
    format_line_t make_header_comment(const char* comment, std::function<std::string()> callback);

    std::string get_reported();
    std::string get_sensor_count();
    //std::string make_sensor_info();

    void make_header_line(char* buf, size_t buf_size, format_line_t line);
    void make_column_line(char* buf, size_t buf_size);

    const uint8_t HEADER_ENTRY_COUNT = 9;
    const uint8_t COMMENT_ENTRY_COUNT = 6;

    static format_line_t HEADER[HEADER_ENTRY_COUNT] = {
        make_header_entry("Format", "IAGA-2002"),
        make_header_entry("Source of Data", SOURCE_OF_DATA),
        make_header_entry("Station Name", STATION_NAME),
        make_header_entry("IAGA Code", IAGA_CODE),
        make_header_entry("Geodetic Latitude", std::to_string(GEODETIC_LATITUDE).c_str()),
        make_header_entry("Geodetic Longitude", std::to_string(GEODETIC_LONGITUDE).c_str()),
        make_header_entry("Elevation", std::to_string(ELEVATION).c_str()),
        make_header_entry("Sensor Orientation", SENSOR_ORIENTATION),
        make_header_entry("Data Type", DATA_TYPE),
    };
    static format_line_t COMMENTS[COMMENT_ENTRY_COUNT] = {
        make_header_comment("Elements: %s", IAGA2002::get_reported),
        make_header_comment("Units: X (north+)[nT]"),
        make_header_comment("Accuracy: 1nT"),
        make_header_comment("Sensor count: %s", IAGA2002::get_sensor_count),
        make_header_comment("D-MAG-Firmware v1.8"),//FIRMWARE_VERSION),
        make_header_comment("https://www.github.com/Powered99/D-MAG-Firmware"),
        //make_header_comment("%s", IAGA2002::make_sensor_info),
    };
}