/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#pragma once

#include "Graphics.hpp"
#include "Hardware.hpp"
#include <functional>

namespace ui{
    extern uint8_t page;
    extern uint8_t title_margin;
    extern uint8_t subtitle_margin;
    extern uint8_t line_margin;

    extern uint8_t text_size;

    extern uint16_t title_text_color;
    extern uint16_t subtitle_text_color;
    extern uint16_t base_text_color;
    extern uint16_t positive_text_color;
    extern uint16_t negative_text_color;
    extern uint16_t inactive_text_color;

    extern display_Font_name_e font;
    extern uint8_t font_width;
    extern uint8_t font_height;

    constexpr uint8_t PAGE_COUNT = 8;
    constexpr uint8_t SETTING_COUNT = 8;
    
    struct page{
        const char* title;
        std::function<void()> draw;
    };

    extern struct page pages[PAGE_COUNT];


    constexpr uint64_t rtc_update_time_delta = 50 * 1000; // 50 ms
    
    void init();
    void draw_page();

    namespace settings{
        struct option{
            const char* text;
            std::function<void()> exec;
            std::function<void()> init;
        };
    };
};
