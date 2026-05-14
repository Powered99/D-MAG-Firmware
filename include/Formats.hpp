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
#include "Config.hpp"

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
#define DATA_TYPE "reported"


namespace formats {
    enum class ELEMENTS{
        MAG,
        MAG_DIFF,
        FREQ,
        VOLTS,
        VOLTS_DIFF,
        TEMP
    };

    struct element_t{
        ELEMENTS element;
        uint8_t channel;
        char label[9];
        uint8_t channel2 = 0; // for elements that require a second channel, eg. difference
    };

    // IAGA2002 allows for a maximum of 4 elements.
    // For more value columns use DMAG2026

    const uint8_t LOG_ELEMENT_COUNT = 3;

    const element_t LOG_ELEMENTS[LOG_ELEMENT_COUNT] = {
        {ELEMENTS::FREQ, 0, "FREQ"},
        {ELEMENTS::VOLTS, 1, "V1"},
        {ELEMENTS::VOLTS, 2, "V2"},
        //{ELEMENTS::VOLTS_DIFF, 1, "DV", 2}
    };

    std::string get_reported();
    std::string get_str_magnetometer_count();
    std::string get_sensor_samples();
    std::string get_sensor_median_samples();
    std::string get_datetime();
}


namespace formats::IAGA2002{
    const uint8_t header_entry_name_length = 22;
    const uint8_t header_entry_value_length = 48;
    const uint8_t format_line_width = 70;
    const uint8_t header_content_width = format_line_width - 2; // SPACE + content + PIPE inside of the 70 character limit, so 68 characters for content.
    const uint8_t max_data_columns = 4; // Max. number of data columns that can be reported in the IAGA file

    struct header_line_t{
        char content[header_content_width]; // %s marker will get replaced by returned value from callback.
        std::function<std::string()> callback;
        bool dynamic = false;
    };

    header_line_t make_header_entry(const char* name, std::function<std::string()> callback);
    header_line_t make_header_entry(const char* name, const char* value);
    header_line_t make_header_comment(const char* value);
    header_line_t make_header_comment_entry(const char* comment, const char* value);
    header_line_t make_header_comment(const char* comment, std::function<std::string()> callback);

    void make_header_line(char* buf, size_t buf_size, header_line_t line);
    void make_column_line(char* buf, size_t buf_size);
    void make_data_line(char* buf, size_t buf_size, ds3231_datetime_t dt, float* sensor_values, uint8_t sensor_count);

    const uint8_t HEADER_ENTRY_COUNT = 10;
    const uint8_t COMMENT_ENTRY_COUNT = 7;

    static header_line_t HEADER[HEADER_ENTRY_COUNT] = {
        make_header_entry("Format", "IAGA-2002"),
        make_header_entry("Source of Data", SOURCE_OF_DATA),
        make_header_entry("Station Name", STATION_NAME),
        make_header_entry("IAGA Code", IAGA_CODE),
        make_header_entry("Geodetic Latitude", std::to_string(GEODETIC_LATITUDE).c_str()),
        make_header_entry("Geodetic Longitude", std::to_string(GEODETIC_LONGITUDE).c_str()),
        make_header_entry("Elevation", std::to_string(ELEVATION).c_str()),
        make_header_entry("Reported", formats::get_reported),
        make_header_entry("Sensor Orientation", SENSOR_ORIENTATION),
        make_header_entry("Data Type", DATA_TYPE),
    };
    static header_line_t COMMENTS[COMMENT_ENTRY_COUNT] = {
        make_header_comment("Units: X (north+)[nT]"),
        make_header_comment("Accuracy: 1nT"),
        make_header_comment("Magnetometer count: %s", formats::get_str_magnetometer_count),
        make_header_comment("%s", formats::get_sensor_samples),
        make_header_comment("%s", formats::get_sensor_median_samples),
        make_header_comment("D-MAG-Firmware v" FIRMWARE_VERSION),
        make_header_comment("https://www.github.com/Powered99/D-MAG-Firmware"),
    };
}

namespace formats::DMAG2026{
    const uint8_t header_entry_name_length = 22;
    const uint8_t header_entry_value_length = 48;
    const uint8_t header_format_line_width = 70; // TODO: Temporary quick solution to reuse IAGA2002 methods
    const uint8_t header_content_width = header_format_line_width - 2; // SPACE + content + PIPE inside of the 70 character limit, so 68 characters for content.
    IAGA2002::header_line_t make_header_entry(const char* name, std::function<std::string()> callback);
    IAGA2002::header_line_t make_header_entry(const char* name, const char* value);
    IAGA2002::header_line_t make_header_comment(const char* value);
    IAGA2002::header_line_t make_header_comment_entry(const char* comment, const char* value);
    IAGA2002::header_line_t make_header_comment(const char* comment, std::function<std::string()> callback);

    void make_header_line(char* buf, size_t buf_size, IAGA2002::header_line_t line);
    void make_column_line(char* buf, size_t buf_size);
    std::string make_data_line(ds3231_datetime_t dt, float* data_values, uint8_t data_column_count);

    const uint8_t HEADER_ENTRY_COUNT = 10;
    const uint8_t COMMENT_ENTRY_COUNT = 8;

    static IAGA2002::header_line_t HEADER[HEADER_ENTRY_COUNT] = {
        make_header_entry("Format", "DMAG-2026"),
        make_header_entry("Source of Data", SOURCE_OF_DATA),
        make_header_entry("Station Name", STATION_NAME),
        make_header_entry("IAGA Code", IAGA_CODE),
        make_header_entry("Geodetic Latitude", std::to_string(GEODETIC_LATITUDE).c_str()),
        make_header_entry("Geodetic Longitude", std::to_string(GEODETIC_LONGITUDE).c_str()),
        make_header_entry("Elevation", std::to_string(ELEVATION).c_str()),
        make_header_entry("Reported", formats::get_reported),
        make_header_entry("Sensor Orientation", SENSOR_ORIENTATION),
        make_header_entry("Data Type", DATA_TYPE),
    };
    static IAGA2002::header_line_t COMMENTS[COMMENT_ENTRY_COUNT] = {
        make_header_comment("Units: X (north+)[nT]"),
        make_header_comment("Accuracy: 1nT"),
        make_header_comment("Magnetometer count: %s", formats::get_str_magnetometer_count),
        make_header_comment("%s", formats::get_sensor_samples),
        make_header_comment("%s", formats::get_sensor_median_samples),
        make_header_comment("Log start: %s", formats::get_datetime),
        make_header_comment("D-MAG-Firmware v" FIRMWARE_VERSION),
        make_header_comment("https://www.github.com/Powered99/D-MAG-Firmware"),
    };

};