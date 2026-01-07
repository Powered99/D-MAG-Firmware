#pragma once

#include "pico/stdlib.h"
#include "displaylib_16/st7735.hpp"

namespace gfx{
    extern uint16_t background_color;
    constexpr uint8_t display_width = 160;
    constexpr uint8_t display_height = 128;

    void set_bg_color(uint16_t color);
    void clear();
    void line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color);
    void text(char* text, uint16_t x, uint16_t y, uint8_t font, uint16_t color);
    size_t smart_text(char* text, uint16_t x, uint16_t y, display_Font_name_e font, uint16_t color, size_t prev_text_length, uint8_t prev_font_width, uint8_t prev_font_height);
    uint8_t progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, float progress, uint8_t prev_bar_width, uint16_t outline_color, uint16_t value_color, bool inverted);
    uint8_t progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, float value, float min_value, float max_value, uint8_t prev_bar_width, uint16_t outline_color, uint16_t value_color, bool inverted);
}