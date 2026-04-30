/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#include "Graphics.hpp"
#include "Hardware.hpp"

namespace gfx{

uint16_t background_color = disp::display.C_BLACK;

void set_bg_color(uint16_t color){
    background_color = color;
}

void clear(){
    disp::display.fillScreen(background_color);
}

void text(char* text, uint16_t x, uint16_t y, uint8_t font, uint16_t color){
    display_Font_name_e compatible_font = static_cast<display_Font_name_e>(font);
    disp::display.setFont(compatible_font);
    disp::display.setTextColor(color, background_color);
    disp::display.writeCharString(x, y, text);
}

// Rewrites previous text and removes ghost characters, returns length of current text
size_t smart_text(char* text,
        uint16_t x, uint16_t y, // Position
        display_Font_name_e font, uint16_t color, // Coordinates
        size_t prev_text_length, uint8_t prev_font_width, uint8_t prev_font_height // Previous text reference
){
    disp::display.setFont(font);
    disp::display.setTextColor(color, background_color);

    size_t text_length = strlen(text);

    if(text_length < prev_text_length){
        disp::display.fillRect(x, y, prev_font_width * prev_text_length, prev_font_height, background_color);
    }
    disp::display.writeCharString(x, y, text);
    return text_length;
}

void scrolling_text(char* text,
        uint16_t x, uint16_t y, uint16_t width,
        display_Font_name_e font, uint16_t color,
        uint8_t font_width, uint8_t font_height,
        int32_t& pixel_offset,
        absolute_time_t& last_update,
        uint32_t update_interval_ms,
        uint32_t scroll_speed_px,
        uint8_t gap_px
){
    // update pixel offset only when interval passed
    absolute_time_t now = get_absolute_time();
    if(absolute_time_diff_us(last_update, now) < (int64_t)update_interval_ms * 1000) return;
    last_update = now;
    pixel_offset += (int32_t)scroll_speed_px; // move specified pixels to the left

    size_t text_length = strlen(text);
    if(text_length == 0) return;

    int32_t loop_pixels = (int32_t)(text_length * font_width) + gap_px;
    if(pixel_offset >= loop_pixels) pixel_offset = 0;

    // how many characters we need to render (add one for partial glyph)
    size_t chars_visible = (width / font_width) + 1;
    char window[chars_visible + 1];

    int32_t char_offset = pixel_offset / font_width;
    int32_t sub_offset  = pixel_offset % font_width;

    size_t gap_chars = gap_px / font_width;
    for(size_t i = 0; i < chars_visible; ++i){
        size_t idx = (char_offset + i) % (text_length + gap_chars);
        window[i] = (idx < text_length) ? text[idx] : ' ';
    }
    window[chars_visible] = '\0';

    disp::display.fillRect(x, y, width, font_height, background_color);
    disp::display.setFont(font);
    disp::display.setTextColor(color, background_color);
    disp::display.writeCharString(x - sub_offset, y, window);
    int16_t rect_x = x - font_width;
    if(rect_x < 0) rect_x = 0;
    disp::display.fillRect(rect_x, y, font_width, font_height, background_color); // Clear ghosting on the left edge
}


void line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color){
    disp::display.drawLine(x1, y1, x2, y2, color);
}

// Draws progress bar, value ranges from 0.0 to 1.0
uint8_t progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, float progress, uint8_t prev_bar_width, uint16_t outline_color, uint16_t value_color, bool inverted){
    progress = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress); // Clamp progress between 0.0 and 1.0

    if(inverted) progress = 1.0 - progress; // Invert progress if enabled

    uint8_t bar_width = static_cast<uint8_t>(width * progress);
    if(bar_width < prev_bar_width){
        disp::display.fillRect(x + bar_width, y, prev_bar_width - bar_width, height, background_color); // Clear previous bar excess
    }
    uint8_t rect_width = bar_width > 2 ? bar_width - 2 : 0;
    disp::display.fillRect(x + 1, y + 1, rect_width, height - 2, value_color); // Draw progress

    disp::display.drawRectWH(x, y, width, height, outline_color); // Draw outline
    return bar_width;
}

// Draws progress bar with specified min and max values
uint8_t progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, float value, float min_value, float max_value, uint8_t prev_bar_width, uint16_t outline_color, uint16_t value_color, bool inverted){
    float progress = (value - min_value) / (max_value - min_value);
    return progress_bar(x, y, width, height, progress, prev_bar_width, outline_color, value_color, inverted);
}
};