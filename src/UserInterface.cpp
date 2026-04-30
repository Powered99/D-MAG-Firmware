/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#include "UserInterface.hpp"
#include "Datalogger.hpp"
#include <limits>
#include "math.h"
#include "config.hpp"
#include "Formats.hpp"

namespace ui{

uint8_t page = 0;

uint8_t status_bar_margin = 14;
uint8_t title_margin = 14;
uint8_t subtitle_margin = 12;
uint8_t line_margin = 12;

uint8_t text_size = 1;

uint16_t status_bar_text_color = disp::display.C_CYAN;
uint16_t title_text_color = disp::display.C_YELLOW;
uint16_t subtitle_text_color = disp::display.C_BLUE;
uint16_t base_text_color = disp::display.C_WHITE;
uint16_t positive_text_color = disp::display.C_GREEN;
uint16_t negative_text_color = disp::display.C_RED;
uint16_t inactive_text_color = disp::display.C_DGREEN;
uint16_t setting_text_color = disp::display.C_MAROON;
uint16_t setting_selected_text_color = disp::display.C_MAGENTA;

display_Font_name_e font = font_pico;
uint8_t font_width = 8;
uint8_t font_height = 12;

namespace settings{
    namespace controls{
        void add_controls();
        void set_controls(bool enabled);
        void remove_controls();
    };
    void exit_setting();
    void exit_settings();
    void next_setting();
    void prev_setting();
    void select_setting();
    void draw_settings_page();
    
    
    namespace fgm_conf{
        namespace modes{
            namespace controls{
                void add_controls();
                void remove_controls();
            };
            void init();
            void select_option();
            void next_option();
            void prev_option();
            void change_ch();
            void draw();
        };
        namespace calib{
            namespace controls{
                void add_controls();
                void remove_controls();
            };
            void init();
            void select_option();
            void next_option();
            void prev_option();
            void draw();
            void draw_calibration();
        };
        namespace sampling{
            namespace controls{
                void add_controls();
                void remove_controls();
            };
            void init();
            void select_option();
            void next_option();
            void prev_option();
            void increase_sample_count();
            void decrease_sample_count();
            void speed_up();
            void reset_speed();
            void enter_edit_sample_count();
            void exit_edit_sample_count();
            void draw();
        };
    };
    namespace rtc_conf{
        void draw_conf();
    };
    namespace log{
        namespace interval{
            void select_option();
            void next_option();
            void prev_option();
            void increase_interval();
            void decrease_interval();
            void speed_up();
            void reset_speed();
            void exit_edit_interval();
            void init();
            void draw();
        };
    };
    
    bool page_open;
};


void draw_all_page();
void draw_channel_page(size_t ch);
void draw_info_page();
void set_page(uint8_t page_to_set);
void next_page();
void prev_page();

namespace controls{
    size_t ctrl_prev_id;
    size_t ctrl_next_id;
    size_t ctrl_log_toggle_id;

    void add_controls(){
        ui::controls::ctrl_prev_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_PRESSED, prev_page);
        ui::controls::ctrl_next_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_PRESSED, next_page);
    }
    void set_controls(bool enabled){
        ctrl::set_enabled(ui::controls::ctrl_prev_id, enabled);
        ctrl::set_enabled(ui::controls::ctrl_next_id, enabled);
    }
};

namespace logging{
    void draw();
    void on_select_down();
    void on_select_up();
    void toggle_log();

    namespace controls{
        void add_controls();
        void remove_controls();
    }
};


struct page pages[PAGE_COUNT] = {
    {.title="ALL", .draw=draw_all_page},
    {.title="CH1", .draw=[]{draw_channel_page(0);}},
    {.title="CH2", .draw=[]{draw_channel_page(1);}},
    {.title="CH3", .draw=[]{draw_channel_page(2);}},
    {.title="CH4", .draw=[]{draw_channel_page(3);}},
    {.title="INFO", .draw=draw_info_page},
    {.title="LOGGER", .draw=logging::draw},
    {.title="SETTINGS", .draw=settings::draw_settings_page}
};


void init(){
    ui::controls::add_controls();
}


void set_page(uint8_t page_to_set){
    if(page == PAGE_COUNT - 2) logging::controls::remove_controls();
    page = page_to_set;
    gfx::clear();
    draw_page();
    // Disable page navigation controls and enable setting select controls
    if(page == PAGE_COUNT - 1){
        if(logger::logging_status == logger::LOG_STATUS::LOGGING) logger::stop_logging();
        ui::controls::set_controls(false);
        settings::controls::add_controls();
    // Add select control to start / stop log
    }else if(page == PAGE_COUNT - 2){
        logging::controls::add_controls();
    }
}

void next_page(){
    set_page((page + 1) % PAGE_COUNT);
}

void prev_page(){
    set_page((page + PAGE_COUNT - 1) % PAGE_COUNT);
}

void draw_status_bar(){
    char buf[20];
    static size_t prev_bar_len;
    static absolute_time_t last_rtc_update_time;
    absolute_time_t rtc_update_time = get_absolute_time();

    static ds3231_datetime_t dt;
    if(last_rtc_update_time - rtc_update_time >= rtc_update_time_delta) rtc::get_datetime(&dt);

    snprintf(buf, sizeof(buf), "%02d:%02d:%02d Log: %s", dt.hour, dt.minutes, dt.seconds, !fs::sd_available ? "NO SD" : ((logger::logging_status == logger::LOG_STATUS::LOGGING) ? "Run" : (logger::logging_status == logger::LOG_STATUS::IDLE) ? "Idle" : "Err"));

    prev_bar_len = gfx::smart_text(buf, 0, 0, font, status_bar_text_color, prev_bar_len, font_width, font_height);
    gfx::line(0, font_height, strlen(buf)*font_width, font_height, status_bar_text_color);
}

// Display page title
void draw_page_title(){
    char buf[20];
    // 'Page x/y - [title]'
    snprintf(buf, sizeof(buf), "Page %d/%d - %s", page+1, PAGE_COUNT, pages[page].title);
    gfx::text(buf, 0, status_bar_margin, font, title_text_color);
    gfx::line(0, status_bar_margin + font_height, strlen(buf) * font_width, status_bar_margin + font_height, title_text_color);
}
void draw_page_basics(){
    draw_status_bar();
    draw_page_title();
}

namespace settings{
    bool setting_open = false;
    uint8_t setting_count = SETTING_COUNT;
    uint8_t selected_setting = 0;

    // Setting options & executeables


    struct option settings[SETTING_COUNT] = {
        {.text="RTC - Set date/time", .exec=rtc_conf::draw_conf},
        {.text="FGM - Set modes", .exec=fgm_conf::modes::draw, .init=fgm_conf::modes::init},
        {.text="FGM - Calibrate", .exec=fgm_conf::calib::draw, .init=fgm_conf::calib::init},
        {.text="FGM - Sampling", .exec=fgm_conf::sampling::draw, .init=fgm_conf::sampling::init},
        {.text="LOG - Set interval", .exec=log::interval::draw, .init=log::interval::init},
        {.text="Exit settings", .exec=exit_settings}
    };

    // Setting option pages

    namespace rtc_conf{
        void draw_conf(){
            exit_settings();
        }
    };
    
    namespace fgm_conf{
        namespace modes{
            uint8_t selected_option = 0;
            uint8_t option_count = fgm::SENSOR_CH_COUNT + 2; // Channels + Submit + Cancel
            namespace controls{
                size_t ctrl_next_id;
                size_t ctrl_prev_id;
                size_t ctrl_select_id;
                void add_controls(){
                    settings::fgm_conf::modes::controls::ctrl_prev_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_PRESSED, prev_option);
                    settings::fgm_conf::modes::controls::ctrl_next_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_PRESSED, next_option);
                    settings::fgm_conf::modes::controls::ctrl_select_id = ctrl::connect(ctrl::BUTTON_t::BTN_SELECT, ctrl::BTN_EVENT_t::BTN_PRESSED, select_option);
                }
                void remove_controls(){
                    ctrl::disconnect(settings::fgm_conf::modes::controls::ctrl_next_id);
                    ctrl::disconnect(settings::fgm_conf::modes::controls::ctrl_prev_id);
                    ctrl::disconnect(settings::fgm_conf::modes::controls::ctrl_select_id);
                }
            }
            
            fgm::SENSOR_MODE SENSOR_MODES[fgm::SENSOR_CH_COUNT];
            void init(){
                settings::fgm_conf::modes::controls::add_controls();
                selected_option = 0;
                for(int i = 0; i < fgm::SENSOR_CH_COUNT; i++){
                    SENSOR_MODES[i] = fgm::SENSOR_MODES[i];
                }
            }

            inline fgm::SENSOR_MODE& operator++(fgm::SENSOR_MODE& mode) {
                using T = std::underlying_type_t<fgm::SENSOR_MODE>;

                T val = static_cast<T>(mode);
                switch (val) {
                    case -1: val = 0; break;
                    case 0: val = 1; break;
                    case 1: val = 2; break;
                    case 2: val = -1; break;
                    default: val = -1; break;
                }

                mode = static_cast<fgm::SENSOR_MODE>(val);
                return mode;
            }

            void change_ch(){
                uint8_t ch = selected_option;
                fgm::SENSOR_MODE sensor_mode = SENSOR_MODES[ch];
                ++sensor_mode;
                SENSOR_MODES[ch] = sensor_mode;
            }
            void apply_ch(){
                fgm::set_sensor_modes(SENSOR_MODES);
                for(int i = 0; i < fgm::SENSOR_CH_COUNT; i++){
                    if(fgm::SENSOR_MODES[i] == fgm::SENSOR_MODE::DISABLED) fgm::deactivate_sensor(i);
                    else fgm::init_sensor(i, fgm::SENSOR_MODES[i]);
                }
            }

            void exit(){
                settings::fgm_conf::modes::controls::remove_controls();
                selected_option = 0;
                exit_setting();
            }
            void submit(){
                apply_ch();
                settings::fgm_conf::modes::exit();
            }
            
            void set_selected_option(uint8_t option){
                selected_option = option;
            }

            void next_option(){
                set_selected_option((selected_option + 1) % option_count);
            }
            void prev_option(){
                set_selected_option((selected_option + option_count - 1) % option_count);
            }
            void select_option(){
                if(selected_option == fgm::SENSOR_CH_COUNT) submit();
                else if(selected_option == fgm::SENSOR_CH_COUNT + 1) exit();
                else change_ch();
            }
            

            void draw(){
                static size_t prev_ch_len[fgm::SENSOR_CH_COUNT];
                char buf[20];
                
                for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++){
                    int y = status_bar_margin + title_margin + ch * line_margin;
                    uint16_t ch_color;
                    char* mode_text;
                    switch(SENSOR_MODES[ch]){
                        case fgm::SENSOR_MODE::DISABLED: 
                            ch_color = negative_text_color; 
                            mode_text = (char*)"Disabled";
                            break;
                        case fgm::SENSOR_MODE::FREQ:
                            ch_color = positive_text_color;  
                            mode_text = (char*)"Frequency";
                            break;
                        case fgm::SENSOR_MODE::ANALOG:
                            ch_color = positive_text_color; 
                            mode_text = (char*)"Analog";
                            break;
                        case fgm::SENSOR_MODE::HARMONIC:
                            ch_color = inactive_text_color;
                            mode_text = (char*)"Harmonic (I2C)[WIP]";
                            break;
                        default:
                            ch_color = setting_text_color;
                            mode_text = (char*)"Unknown";
                            break;
                    }
                    if(ch == selected_option) ch_color = setting_selected_text_color;
                    // 'CHx: y.yy Hz'
                    snprintf(buf, sizeof(buf), " CH%d: %s", ch+1, mode_text);
                    prev_ch_len[ch] = gfx::smart_text(buf, 0, y, font, ch_color, prev_ch_len[ch], font_width, font_height);
                }
                int y = status_bar_margin + title_margin + (fgm::SENSOR_CH_COUNT + 1) * line_margin;
                uint16_t color = (selected_option == fgm::SENSOR_CH_COUNT) ? setting_selected_text_color : setting_text_color;
                gfx::text((char*)"Submit", 0, y, font, color);

                color = (selected_option == fgm::SENSOR_CH_COUNT + 1) ? setting_selected_text_color : setting_text_color;
                gfx::text((char*)"Cancel", 0, y + line_margin, font, color);
            }
        
        }
        
        namespace calib{
            uint8_t selected_option = 0;
            uint8_t option_count = fgm::SENSOR_CH_COUNT + 1; // Channel selection + Cancel
            bool calibrating = false;

            fgm::CALIB_DATA calib_data; // Calibrated data

            uint8_t calib_selected_option = 0;
            uint8_t calib_option_count = 2;

            namespace controls{
                size_t ctrl_next_id;
                size_t ctrl_prev_id;
                size_t ctrl_select_id;
                void add_controls(){
                    settings::fgm_conf::modes::controls::ctrl_prev_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_PRESSED, prev_option);
                    settings::fgm_conf::modes::controls::ctrl_next_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_PRESSED, next_option);
                    settings::fgm_conf::modes::controls::ctrl_select_id = ctrl::connect(ctrl::BUTTON_t::BTN_SELECT, ctrl::BTN_EVENT_t::BTN_PRESSED, select_option);
                }
                void remove_controls(){
                    ctrl::disconnect(settings::fgm_conf::modes::controls::ctrl_next_id);
                    ctrl::disconnect(settings::fgm_conf::modes::controls::ctrl_prev_id);
                    ctrl::disconnect(settings::fgm_conf::modes::controls::ctrl_select_id);
                }
            }
            
            void init(){
                settings::fgm_conf::calib::controls::add_controls();
            }

            void apply_ch(){
                fgm::set_sensor_calib(selected_option, calib_data);
            }
            void calib_enter(){
                calibrating = true;
                fgm::set_sample_count(selected_option, 1); // Set sample count to 1 for real-time calibration
                calib_data = {.MIN = std::numeric_limits<float>::max() , .MAX = std::numeric_limits<float>::min()};
                gfx::clear();
                draw_calibration();
            }
            void calib_exit(){
                calibrating = false;
                calib_selected_option = 0;
                fgm::load_sample_count(selected_option); // Restore configured sample count
                gfx::clear();
                draw();
            }
            void calib_submit(){
                apply_ch();
                calib_exit();
            }
            void exit(){
                settings::fgm_conf::modes::controls::remove_controls();
                selected_option = 0;
                exit_setting();
            }


            void set_selected_option(uint8_t option){
                selected_option = option;
            }

            void next_option(){
                if(!calibrating) set_selected_option((selected_option + 1) % option_count);
                else calib_selected_option = (calib_selected_option + 1) % calib_option_count;
            }
            void prev_option(){
                if(!calibrating) set_selected_option((selected_option + option_count - 1) % option_count);
                else calib_selected_option = (calib_selected_option + calib_option_count - 1) % calib_option_count;
            }
            void select_option(){
                if(!calibrating){
                    if(selected_option == fgm::SENSOR_CH_COUNT) exit();
                    else if(fgm::SENSOR_STATES[selected_option] == fgm::SENSOR_STATE::ACTIVE) calib_enter();
                }else{
                    if(calib_selected_option == 0) calib_submit();
                    else calib_exit();
                }
                
            }
            
            
            void calibrate(){
                uint8_t ch = selected_option;

                float value = fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::FREQ ? fgm::periods[ch] : (fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::ANALOG || fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::HARMONIC) ? fgm::voltages[ch] : 0.0;
                calib_data.MIN = std::min(value, calib_data.MIN);
                calib_data.MAX = std::max(value, calib_data.MAX);

                draw_calibration();
            }

            void draw_calibration(){
                static size_t tmin_len;
                static size_t tmax_len;
                uint8_t ch = selected_option;
                char buf[20];

                int y =  status_bar_margin + title_margin;
                snprintf(buf, sizeof(buf), "Calibrating CH%d", ch);
                gfx::text(buf, 0, y, font, subtitle_text_color);

                y += line_margin;
                snprintf(buf, sizeof(buf), "MIN: %e",calib_data.MIN);
                tmin_len = gfx::smart_text(buf, 0, y, font, positive_text_color, tmin_len, font_width, font_height);
                
                y += line_margin;
                snprintf(buf, sizeof(buf), "MAX: %e",calib_data.MAX);
                tmax_len = gfx::smart_text(buf, 0, y, font, positive_text_color, tmax_len, font_width, font_height);

                y += line_margin * 2;
                uint16_t color = calib_selected_option == 0 ? setting_selected_text_color : setting_text_color;
                gfx::text((char*)"Submit", 0, y, font, color);
                
                y += line_margin;
                color = calib_selected_option == 1 ? setting_selected_text_color : setting_text_color;
                gfx::text((char*)"Cancel", 0, y, font, color);
            }

            void draw(){
                if(calibrating){
                    calibrate();
                    return;
                }
                static size_t prev_ch_len[fgm::SENSOR_CH_COUNT];
                char buf[20];
                
                for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++){
                    int y = status_bar_margin + title_margin + ch * line_margin;
                    uint16_t ch_color;
                    char* ch_text;
                    switch(fgm::SENSOR_STATES[ch]){
                        case fgm::SENSOR_STATE::DISABLED:
                            ch_color = negative_text_color;
                            ch_text = (char*)"Disabled";
                            break;
                        case fgm::SENSOR_STATE::INACTIVE:
                            ch_color = inactive_text_color;
                            ch_text = (char*)"Inactive";
                            break;
                        case fgm::SENSOR_STATE::ACTIVE:
                            ch_color = positive_text_color;
                            ch_text = (char*)"Calibrate";
                            break;
                        default:
                            ch_color = disp::display.C_DGREY;
                            ch_text = (char*)"Unknown";
                    }              

                    if(ch == selected_option) ch_color = setting_selected_text_color;
                    // 'CHx: y.yy Hz'
                    snprintf(buf, sizeof(buf), " CH%d - %s", ch+1, ch_text);
                    prev_ch_len[ch] = gfx::smart_text(buf, 0, y, font, ch_color, prev_ch_len[ch], font_width, font_height);
                }
                int y = status_bar_margin + title_margin + (fgm::SENSOR_CH_COUNT + 1) * line_margin;
                uint16_t color = (selected_option == fgm::SENSOR_CH_COUNT) ? setting_selected_text_color : setting_text_color;
                gfx::text((char*)"Quit", 0, y, font, color);
            }

        };
        
        namespace sampling{
            uint8_t selected_option = 0;
            uint8_t option_count = fgm::SENSOR_CH_COUNT + 2; // Channels + Submit + Cancel
            int sample_count[fgm::SENSOR_CH_COUNT];
            bool editing_sample_count = false;
            int change_amount = 1;

            namespace controls{
                size_t ctrl_next_id;
                size_t ctrl_prev_id;
                size_t ctrl_select_id;
                // Sample count setting
                size_t ctrl_increase_id;
                size_t ctrl_decrease_id;
                // Sample count setting speed change when held
                size_t ctrl_held_increase_id;
                size_t ctrl_held_decrease_id;

                size_t ctrl_increase_hold_id;
                size_t ctrl_decrease_hold_id;
                // Reset speed when button released
                size_t ctrl_increase_release_id;
                size_t ctrl_decrease_release_id;

                void add_controls(){
                    ctrl_next_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_PRESSED, next_option);
                    ctrl_prev_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_PRESSED, prev_option);
                    ctrl_select_id = ctrl::connect(ctrl::BUTTON_t::BTN_SELECT, ctrl::BTN_EVENT_t::BTN_PRESSED, select_option);

                    ctrl_increase_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_PRESSED, increase_sample_count);
                    ctrl_decrease_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_PRESSED, decrease_sample_count);

                    ctrl_held_increase_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_DOWN, increase_sample_count);
                    ctrl_held_decrease_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_DOWN, decrease_sample_count);

                    ctrl_increase_hold_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_HOLD, speed_up);
                    ctrl_decrease_hold_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_HOLD, speed_up);

                    ctrl_increase_release_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_RELEASED, reset_speed);
                    ctrl_decrease_release_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_RELEASED, reset_speed);

                    ctrl::set_enabled(ctrl_increase_id, false);
                    ctrl::set_enabled(ctrl_decrease_id, false);
                    
                    ctrl::set_enabled(ctrl_increase_hold_id, false);
                    ctrl::set_enabled(ctrl_decrease_hold_id, false);
                    ctrl::set_enabled(ctrl_held_increase_id, false);
                    ctrl::set_enabled(ctrl_held_decrease_id, false);
                    ctrl::set_enabled(ctrl_increase_release_id, false);
                    ctrl::set_enabled(ctrl_decrease_release_id, false);
                }
                void remove_controls(){
                    ctrl::disconnect(ctrl_next_id);
                    ctrl::disconnect(ctrl_prev_id);
                    ctrl::disconnect(ctrl_select_id);

                    ctrl::disconnect(ctrl_increase_id);
                    ctrl::disconnect(ctrl_decrease_id);

                    ctrl::disconnect(ctrl_increase_hold_id);
                    ctrl::disconnect(ctrl_decrease_hold_id);
                    ctrl::disconnect(ctrl_held_increase_id);
                    ctrl::disconnect(ctrl_held_decrease_id);
                    ctrl::disconnect(ctrl_increase_release_id);
                    ctrl::disconnect(ctrl_decrease_release_id);
                }
            };
            void init(){
                settings::fgm_conf::sampling::controls::add_controls();
                for(int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++){
                    sample_count[ch] = fgm::SET_SAMPLE_COUNT[ch];
                }
                selected_option = 0;
            }
            void submit_sample_count(){
                for(int ch = 0 ; ch < fgm::SENSOR_CH_COUNT; ch++){
                    fgm::set_sample_count(ch, (uint)sample_count[ch]);
                    fgm::save_sample_count(ch);
                    fgm::MEDIAN_SAMPLE_OFFSET[ch] = (uint)sample_count[ch] / 4; // Temporarily the median will be taken from half of the configured sample count
                }
            }

            void speed_up(){
                change_amount = 5;
                ctrl::set_enabled(sampling::controls::ctrl_increase_id, false);
                ctrl::set_enabled(sampling::controls::ctrl_decrease_id, false);
                ctrl::set_enabled(sampling::controls::ctrl_select_id, false);

                ctrl::set_enabled(sampling::controls::ctrl_held_increase_id, true);
                ctrl::set_enabled(sampling::controls::ctrl_held_decrease_id, true);
                ctrl::set_enabled(sampling::controls::ctrl_increase_release_id, true);
                ctrl::set_enabled(sampling::controls::ctrl_decrease_release_id, true);
            }
            void reset_speed(){
                change_amount = 1;
                ctrl::set_enabled(sampling::controls::ctrl_increase_id, true);
                ctrl::set_enabled(sampling::controls::ctrl_decrease_id, true);
                ctrl::set_enabled(sampling::controls::ctrl_select_id, true);

                ctrl::set_enabled(sampling::controls::ctrl_held_increase_id, false);
                ctrl::set_enabled(sampling::controls::ctrl_held_decrease_id, false);
                ctrl::set_enabled(sampling::controls::ctrl_increase_release_id, false);
                ctrl::set_enabled(sampling::controls::ctrl_decrease_release_id, false);
            }

            void increase_sample_count(){
                sample_count[selected_option] += change_amount;
                if(sample_count[selected_option] >= fgm::MAX_SAMPLE_COUNT) sample_count[selected_option] = fgm::MAX_SAMPLE_COUNT;
            }
            void decrease_sample_count(){
                sample_count[selected_option] -= change_amount;
                if(sample_count[selected_option] <= 0) sample_count[selected_option] = 1; // TODO: Temporarily 4 to avoid median problems
            }  
            void enter_edit_sample_count(){
                ctrl::set_enabled(sampling::controls::ctrl_next_id, false);
                ctrl::set_enabled(sampling::controls::ctrl_prev_id, false);

                ctrl::set_enabled(sampling::controls::ctrl_increase_id, true);
                ctrl::set_enabled(sampling::controls::ctrl_decrease_id, true);

                ctrl::set_enabled(sampling::controls::ctrl_increase_hold_id, true);
                ctrl::set_enabled(sampling::controls::ctrl_decrease_hold_id, true);

                editing_sample_count = true;
            }
            void exit_edit_sample_count(){
                ctrl::set_enabled(sampling::controls::ctrl_next_id, true);
                ctrl::set_enabled(sampling::controls::ctrl_prev_id, true);

                ctrl::set_enabled(sampling::controls::ctrl_increase_id, false);
                ctrl::set_enabled(sampling::controls::ctrl_decrease_id, false);
                ctrl::set_enabled(sampling::controls::ctrl_increase_hold_id, false);
                ctrl::set_enabled(sampling::controls::ctrl_decrease_hold_id, false);

                editing_sample_count = false;
            }
            void exit(){
                settings::fgm_conf::sampling::controls::remove_controls();
                selected_option = 0;
                exit_setting();
            }
            void submit(){
                submit_sample_count();
                exit();
            }
            void set_selected_option(uint8_t option){
                selected_option = option;
            }
            void next_option(){
                set_selected_option((selected_option + 1) % option_count);
            }
            void prev_option(){
                set_selected_option((selected_option + option_count - 1) % option_count);
            }
            void select_option(){
                if(editing_sample_count){
                    exit_edit_sample_count();
                }else{
                    if(selected_option == option_count - 2) submit();
                    else if(selected_option == option_count - 1) exit();
                    else enter_edit_sample_count();
                }
            };
            void draw(){ // TODO: Implement median configuration
                char buf[20];
                uint16_t color;
                static uint8_t prev_sample_count_len[fgm::SENSOR_CH_COUNT];
                int y =  status_bar_margin + title_margin;

                gfx::text((char*)" CH  SAMPLE MEDIAN", 0, y, font, subtitle_text_color);
                y += line_margin;
                

                // Draw each channel sample count
                for(int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++){
                    snprintf(buf, sizeof(buf), " CH%d: %d Samples", ch + 1, sample_count[ch]);
                    
                    uint16_t color;
                    if(selected_option == ch){
                        if(editing_sample_count) color = disp::display.C_LBLUE;
                        else color = setting_selected_text_color;
                    }else color = fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::ACTIVE ? positive_text_color : fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::INACTIVE ? inactive_text_color : negative_text_color;

                    prev_sample_count_len[ch] = gfx::smart_text(buf, 0, y, font, color, prev_sample_count_len[ch], font_width, font_height);
                    y += line_margin;
                }

                y = status_bar_margin + subtitle_margin + line_margin * 6;
                color = selected_option == fgm::SENSOR_CH_COUNT ? setting_selected_text_color : setting_text_color;
                gfx::text((char*)"Submit", 0, y, font, color);
                
                y += line_margin;
                color = selected_option == fgm::SENSOR_CH_COUNT + 1 ? setting_selected_text_color : setting_text_color;
                gfx::text((char*)"Cancel", 0, y, font, color);
            }
        };
    };
    namespace log{
        namespace interval{
            namespace controls{
                // Option selection
                size_t ctrl_next_id;
                size_t ctrl_prev_id;
                size_t ctrl_select_id;
                // Interval setting
                size_t ctrl_increase_id;
                size_t ctrl_decrease_id;

                size_t ctrl_held_increase_id;
                size_t ctrl_held_decrease_id;

                size_t ctrl_increase_hold_id;
                size_t ctrl_decrease_hold_id;

                size_t ctrl_increase_release_id;
                size_t ctrl_decrease_release_id;

                void add_controls(){
                    ctrl_prev_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_PRESSED, prev_option);
                    ctrl_next_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_PRESSED, next_option);
                    ctrl_select_id = ctrl::connect(ctrl::BUTTON_t::BTN_SELECT, ctrl::BTN_EVENT_t::BTN_PRESSED, select_option);

                    ctrl_increase_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_PRESSED, increase_interval);
                    ctrl_decrease_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_PRESSED, decrease_interval);

                    ctrl_increase_hold_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_HOLD, speed_up);
                    ctrl_decrease_hold_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_HOLD, speed_up);

                    ctrl_held_increase_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_DOWN, increase_interval);
                    ctrl_held_decrease_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_DOWN, decrease_interval);

                    ctrl_increase_release_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_RELEASED, reset_speed);
                    ctrl_decrease_release_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_RELEASED, reset_speed);

                    ctrl::set_enabled(ctrl_increase_id, false);
                    ctrl::set_enabled(ctrl_decrease_id, false);
                    
                    ctrl::set_enabled(ctrl_increase_hold_id, false);
                    ctrl::set_enabled(ctrl_decrease_hold_id, false);
                    ctrl::set_enabled(ctrl_held_increase_id, false);
                    ctrl::set_enabled(ctrl_held_decrease_id, false);
                    ctrl::set_enabled(ctrl_increase_release_id, false);
                    ctrl::set_enabled(ctrl_decrease_release_id, false);
                }
                void remove_controls(){
                    ctrl::disconnect(ctrl_next_id);
                    ctrl::disconnect(ctrl_prev_id);
                    ctrl::disconnect(ctrl_select_id);

                    ctrl::disconnect(ctrl_increase_id);
                    ctrl::disconnect(ctrl_decrease_id);

                    ctrl::disconnect(ctrl_increase_hold_id);
                    ctrl::disconnect(ctrl_decrease_hold_id);
                    ctrl::disconnect(ctrl_held_increase_id);
                    ctrl::disconnect(ctrl_held_decrease_id);
                    ctrl::disconnect(ctrl_increase_release_id);
                    ctrl::disconnect(ctrl_decrease_release_id);
                    
                }
            }

            const uint8_t option_count = 3;
            uint8_t selected_option = 0;
            bool editing_interval = false;

            int interval;
            uint32_t interval_change_amount = 100;
            
            void speed_up(){
                interval_change_amount = 500;
                ctrl::set_enabled(settings::log::interval::controls::ctrl_increase_id, false);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_decrease_id, false);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_select_id, false);

                ctrl::set_enabled(settings::log::interval::controls::ctrl_held_increase_id, true);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_held_decrease_id, true);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_increase_release_id, true);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_decrease_release_id, true);
            }
            void reset_speed(){
                interval_change_amount = 100;
                ctrl::set_enabled(settings::log::interval::controls::ctrl_increase_id, true);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_decrease_id, true);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_select_id, true);

                ctrl::set_enabled(settings::log::interval::controls::ctrl_held_increase_id, false);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_held_decrease_id, false);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_increase_release_id, false);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_decrease_release_id, false);
                
            }

            void apply_interval(){
                logger::set_log_interval((uint32_t)interval);
            }

            void increase_interval(){
                interval += interval_change_amount;
                
            }
            void decrease_interval(){
                interval -= interval_change_amount;
                interval = std::max(interval, 100);
            }
            void enter_edit_interval(){
                ctrl::set_enabled(settings::log::interval::controls::ctrl_next_id, false);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_prev_id, false);

                ctrl::set_enabled(settings::log::interval::controls::ctrl_increase_id, true);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_decrease_id, true);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_increase_hold_id, true);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_decrease_hold_id, true);
                editing_interval = true;
            }
            void exit_edit_interval(){
                ctrl::set_enabled(settings::log::interval::controls::ctrl_next_id, true);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_prev_id, true);

                ctrl::set_enabled(settings::log::interval::controls::ctrl_increase_id, false);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_decrease_id, false);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_increase_hold_id, false);
                ctrl::set_enabled(settings::log::interval::controls::ctrl_decrease_hold_id, false);
                editing_interval = false;
            }

            void exit(){
                settings::log::interval::controls::remove_controls();
                selected_option = 0;
                exit_setting();
            }
            void submit(){
                apply_interval();
                exit();
            }

            void set_selected_option(uint8_t option){
                selected_option = option;
            }

            void next_option(){
                set_selected_option((selected_option + 1) % option_count);
            }
            void prev_option(){
                set_selected_option((selected_option + option_count - 1) % option_count);
            }
            void select_option(){
                if(editing_interval){
                    exit_edit_interval();
                }else{
                    if(selected_option == option_count - 2) submit();
                    else if(selected_option == option_count - 1) exit();
                    else enter_edit_interval();
                }
            }

            void draw(){
                char buf[20];
                static size_t interval_len;

                int y =  status_bar_margin + title_margin;
                gfx::text((char*)"Set log interval", 0, y, font, subtitle_text_color);

                y += line_margin;
                snprintf(buf, sizeof(buf), "Interval: %dms",interval);

                uint16_t color;
                if(selected_option == 0){
                    if(editing_interval) color = disp::display.C_LBLUE;
                    else color = setting_selected_text_color;
                }else color = positive_text_color;

                interval_len = gfx::smart_text(buf, 0, y, font, color, interval_len, font_width, font_height);

                y += line_margin * 3;
                color = selected_option == 1 ? setting_selected_text_color : setting_text_color;
                gfx::text((char*)"Submit", 0, y, font, color);
                
                y += line_margin;
                color = selected_option == 2 ? setting_selected_text_color : setting_text_color;
                gfx::text((char*)"Cancel", 0, y, font, color);
            }
            void init(){
                controls::add_controls();
                interval = logger::log_interval_ms;
            }
        };
    };


    // Settings page

    void draw_setting_title(){
        char buf[20];
        snprintf(buf, sizeof(buf), "(S) %s", settings[selected_setting].text);
        gfx::text(buf, 0, status_bar_margin, font, title_text_color);
        gfx::line(0, status_bar_margin + font_height, strlen(buf) * font_width, status_bar_margin + font_height, title_text_color);
    }

    void draw_setting_list(){
        uint8_t base_margin = status_bar_margin + title_margin;
        for(int i = 0; i < SETTING_COUNT; i++){
            gfx::text((char*)settings[i].text, 0, base_margin + line_margin * i+1, font, i == selected_setting ? setting_selected_text_color : setting_text_color);
        }
    }

    void draw_settings_page(){
        if(!setting_open) draw_setting_list();
        else{
            draw_setting_title();
            settings[selected_setting].exec();
        } 
    }


    // Exiting functions

    void exit_setting(){
        selected_setting = 0;
        setting_open = false;
        settings::controls::set_controls(true);
        gfx::clear();
    }

    void exit_settings(){
        selected_setting = 0;
        setting_open = false;
        settings::controls::remove_controls();
        ui::controls::set_controls(true);
        set_page(0);
    }

    // User input
    namespace controls{
        size_t ctrl_next_id;
        size_t ctrl_prev_id;
        size_t ctrl_select_id;

        void add_controls(){
            settings::controls::ctrl_prev_id = ctrl::connect(ctrl::BUTTON_t::BTN_LEFT, ctrl::BTN_EVENT_t::BTN_PRESSED, prev_setting);
            settings::controls::ctrl_next_id = ctrl::connect(ctrl::BUTTON_t::BTN_RIGHT, ctrl::BTN_EVENT_t::BTN_PRESSED, next_setting);
            settings::controls::ctrl_select_id = ctrl::connect(ctrl::BUTTON_t::BTN_SELECT, ctrl::BTN_EVENT_t::BTN_PRESSED, select_setting);
        }
        void set_controls(bool enabled){
            ctrl::set_enabled(settings::controls::ctrl_next_id, enabled);
            ctrl::set_enabled(settings::controls::ctrl_prev_id, enabled);
            ctrl::set_enabled(settings::controls::ctrl_select_id, enabled);
        }
        void remove_controls(){
            ctrl::disconnect(settings::controls::ctrl_next_id);
            ctrl::disconnect(settings::controls::ctrl_prev_id);
            ctrl::disconnect(settings::controls::ctrl_select_id);
        }
    };
    

    void select_setting(){
        if(!setting_open){
            if(selected_setting != SETTING_COUNT - 1){
                setting_open = true;
                settings::controls::set_controls(false);
                if(settings[selected_setting].init) settings[selected_setting].init();
                gfx::clear();
            }else{
                settings[selected_setting].exec(); // Execute exit settings command without the setting_open trigger
            }
        }
    }
    void next_setting(){
        selected_setting = (selected_setting + 1) % SETTING_COUNT;
    }
    void prev_setting(){
        selected_setting = (selected_setting + SETTING_COUNT - 1) % SETTING_COUNT;
    }

};


void draw_info_page(){
    char buf[20];
    snprintf(buf, sizeof(buf), "D-MAG-Firmware: v%s", FIRMWARE_VERSION);
    gfx::text(buf, 0, status_bar_margin + title_margin, font, base_text_color);
    gfx::text((char*)"-by Dominik Kultys", 0, status_bar_margin + title_margin + line_margin, font, base_text_color);
    gfx::text((char*)"Sensors: Freq/Volt", 0, status_bar_margin + title_margin + 2 * line_margin, font, base_text_color);
    gfx::text((char*)"Data Format: IAGA-2002", 0, status_bar_margin + title_margin + 3 * line_margin, font, base_text_color);
}

void draw_all_page(){
    static size_t prev_ch_len[fgm::SENSOR_CH_COUNT];
    char buf[20];

    for (int ch = 0; ch < fgm::SENSOR_CH_COUNT; ch++){
        int y = status_bar_margin + title_margin + ch * line_margin;

        // handle inactive channels
        if (fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::DISABLED){
            // 'CHx: Disabled'
            snprintf(buf, sizeof(buf), "CH%d: Disabled", ch+1);
            gfx::text(buf, 0, y, font, negative_text_color);
            continue;
        }
        if(fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::INACTIVE){
            // 'CHx: Inactive'
            snprintf(buf, sizeof(buf), "CH%d: Inactive", ch+1);
            prev_ch_len[ch] = gfx::smart_text(buf, 0, y, font, inactive_text_color, prev_ch_len[ch], font_width, font_height);
            continue;
        }
        // 'CHx: y.yy Hz'
        snprintf(buf, sizeof(buf), "CH%d: %.2f nT", ch+1, fgm::get_nT(ch));
        prev_ch_len[ch] = gfx::smart_text(buf, 0, y, font, positive_text_color, prev_ch_len[ch], font_width, font_height);
    }
}

// Single channel page
void draw_channel_page(size_t ch){
    static uint8_t prev_mag_len = 0;
    static uint8_t prev_freq_len = 0;
    static uint8_t prev_bar_width = 0;
    static fgm::SENSOR_STATE prev_ch_states[fgm::SENSOR_CH_COUNT];
    char buf[20];

    // Redraw page on mode change
    if (prev_ch_states[ch] != fgm::SENSOR_STATES[ch]){ 
        gfx::clear(); 
        draw_page_basics(); 
    }
    
    // 'Channel {x} (@GPIO{y})
    uint PIN = (fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::ANALOG || fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::HARMONIC) ? SENSOR_PINS_ANALOG[ch] : fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::FREQ ? SENSOR_PINS_FREQ[ch] : -1;
    if(PIN == -1){
        snprintf(buf, sizeof(buf), "Channel %d (N/C)", ch+1);
    }else{
        snprintf(buf, sizeof(buf), "Channel %d (@GPIO%d)", ch+1, PIN);
    }
    gfx::text(buf, 0, status_bar_margin + title_margin, font, subtitle_text_color);

    
    if (fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::DISABLED){
        // Big 'Disabled'
        gfx::text((char*)"Disabled", 0, status_bar_margin + title_margin + subtitle_margin, font_arialBold, negative_text_color);

    }else if(fgm::SENSOR_STATES[ch] == fgm::SENSOR_STATE::INACTIVE){
        // Big 'Inactive'
        gfx::text((char*)"Inactive", 0, status_bar_margin + title_margin + subtitle_margin, font_arialBold, inactive_text_color);

    } else {
        // 'Mag: {x.xx} nT'
        
        snprintf(buf, sizeof(buf), "Mag: %.2f nT", fgm::get_nT(ch));

        prev_mag_len = gfx::smart_text(buf, 0, status_bar_margin + title_margin + subtitle_margin, font, positive_text_color, prev_mag_len, font_width, font_height);

        char* value_text = fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::FREQ ? (char*)"Freq: %.4f kHz" : (fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::ANALOG || fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::HARMONIC) ? (char*)"Sample.: %.4f" : (char*)"No value";
        float value = fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::FREQ ? fgm::frequencies[ch] / 1000.0f : (fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::ANALOG || fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::HARMONIC) ? fgm::voltages[ch] : -1.0f;
        // 'Freq: {x.xx} Hz'
        snprintf(buf, sizeof(buf), value_text, value);

        prev_freq_len = gfx::smart_text(buf, 0, status_bar_margin + title_margin + subtitle_margin + line_margin, font, positive_text_color, prev_freq_len, font_width, font_height);

        if(fgm::SENSOR_MODES[ch] == fgm::SENSOR_MODE::FREQ){
            // Frequency sample count progress bar
            uint8_t y = status_bar_margin + title_margin + subtitle_margin + line_margin * 3;
            uint8_t bar_width = 96;

            prev_bar_width = gfx::progress_bar(0, y, bar_width, 10, fgm::sample_index[ch], 0, fgm::SAMPLE_COUNT[ch], prev_bar_width, disp::display.C_WHITE, disp::display.C_ORANGE, false);
            static size_t prev_bar_label_len = 0;
            snprintf(buf, sizeof(buf), "%d/%d", fgm::sample_index[ch], fgm::SAMPLE_COUNT[ch]);
            prev_bar_label_len = gfx::smart_text(buf, bar_width + 4, y, font, disp::display.C_ORANGE, prev_bar_label_len, font_width, font_height);
        }  
        
    }
    prev_ch_states[ch] = fgm::SENSOR_STATES[ch];


    // Display calibration data
    snprintf(buf, sizeof(buf), "MIN: %e", fgm::SENSOR_CALIBRATIONS[ch].MIN);
    gfx::text(buf, 0, status_bar_margin + title_margin + subtitle_margin + line_margin * 5, font, inactive_text_color);
    snprintf(buf, sizeof(buf), "MAX: %e", fgm::SENSOR_CALIBRATIONS[ch].MAX);
    gfx::text(buf, 0, status_bar_margin + title_margin + subtitle_margin + line_margin * 6, font, inactive_text_color);
}

namespace logging{
    namespace controls{
        size_t ctrl_toggle_hold_id;
        size_t ctrl_toggle_down_id;
        size_t ctrl_toggle_released_id;
        
        void add_controls(){
            ctrl_toggle_hold_id = ctrl::connect(ctrl::BUTTON_t::BTN_SELECT, ctrl::BTN_EVENT_t::BTN_HOLD, toggle_log);
            ctrl_toggle_down_id = ctrl::connect(ctrl::BUTTON_t::BTN_SELECT, ctrl::BTN_EVENT_t::BTN_DOWN, on_select_down);
            ctrl_toggle_released_id = ctrl::connect(ctrl::BUTTON_t::BTN_SELECT, ctrl::BTN_EVENT_t::BTN_RELEASED, on_select_up);
        }
        void remove_controls(){
            ctrl::disconnect(ctrl_toggle_hold_id);
            ctrl::disconnect(ctrl_toggle_down_id);
            ctrl::disconnect(ctrl_toggle_released_id);
        }
    };

    bool toggle_held = false;

    void on_select_down(){
        toggle_held = true;
    }
    void on_select_up(){
        toggle_held = false;
    }
    void toggle_log(){
        if(logger::logging_status == logger::LOG_STATUS::LOGGING) logger::stop_logging();
        else if(logger::logging_status == logger::LOG_STATUS::IDLE) logger::start_logging();
        gfx::clear();
    }
    void draw(){
        static size_t startstop_len;

        char buf[32];
        int y = status_bar_margin + title_margin;
        char* status_text = (char*)((logger::logging_status == logger::LOG_STATUS::LOGGING) ? "Logging elements:" : (logger::logging_status == logger::LOG_STATUS::IDLE && fs::sd_available) ? "Logger idle" : "SD error; restart required");
        gfx::text(status_text, 0, y, font, subtitle_text_color);

        y += subtitle_margin;
        if(logger::logging_status == logger::LOG_STATUS::LOGGING){
            
            snprintf(buf, sizeof(buf), "");
            for(uint8_t i = 0; i < LOG_ELEMENT_COUNT; i++){
                char ch_buf[7];
                strcat(buf,(i == 0) ? "" : ", ");
                snprintf(ch_buf, sizeof(ch_buf), "%s", LOG_ELEMENTS[i].label);
                strcat(buf, ch_buf);
            }
            gfx::text(buf, 0, y, font, positive_text_color);
            y += line_margin;
            snprintf(buf, sizeof(buf), "Interval: %dms",logger::log_interval_ms);
            gfx::text(buf, 0, y, font, subtitle_text_color);
            y += line_margin;
            snprintf(buf, sizeof(buf), "Format: %s", logger::DATA_FORMAT == logger::FORMATS::IAGA2002 ? "IAGA-2002" : logger::DATA_FORMAT == logger::FORMATS::DMAG2026 ? "DMAG-2026" : "Unknown");
            gfx::text(buf, 0, y, font, subtitle_text_color);
            y += line_margin;
        }

        y += line_margin;
        if(logger::logging_status != logger::LOG_STATUS::ERROR && fs::sd_available) snprintf(buf, sizeof(buf), "%s log? (hold)", (logger::logging_status == logger::LOG_STATUS::LOGGING) ? "Stop" : "Start");
        
        uint16_t color = toggle_held ? setting_selected_text_color : setting_text_color;

        if(logger::logging_status != logger::LOG_STATUS::ERROR && fs::sd_available) startstop_len = gfx::smart_text(buf, 0, y, font, color, startstop_len, font_width, font_height);
    }
};


void draw_page(){
    char buf[20];
    draw_status_bar();
    if (!settings::setting_open) draw_page_title();
    pages[page].draw();
}
};