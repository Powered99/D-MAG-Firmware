/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#include "Hardware.hpp"
#include "Graphics.hpp"
#include "Math.hpp"
#include "Config.hpp"

#include <functional>
#include <vector>
#include <algorithm>

// ST7735 TFT LCD Display
namespace disp{


// st7735 driver object & configs
ST7735_TFT display;
ST7735_TFT::TFT_PCBtype_e display_driver = display.TFT_ST7735S_Black;

void init_display(){
    display.TFTInitSPIType(8000, TFT_SPI_PORT);
    display.setupGPIO(TFT_SPI_RST, TFT_SPI_DC, TFT_SPI_CS, TFT_SPI_SCK, TFT_SPI_TX);
    display.TFTInitScreenSize(0, 0, 128, 160);
    display.TFTInitPCBType(display_driver);
    display.setRotation(display.Degrees_90);
    gfx::clear();
}

}

// Flux-Gate-Magnetometer-Sensors
namespace fgm{

// Sensor modes & states
SENSOR_STATE SENSOR_STATES[SENSOR_CH_COUNT];

// Initialized flags for sensor channels
bool FREQ_INITIALIZED[SENSOR_CH_COUNT]; // frequency (PWM) driver
bool ANALOG_INITIALIZED[SENSOR_CH_COUNT]; // fast (onboard ADC) analog driver
bool I2C_INITIALIZED[SENSOR_CH_COUNT]; // 3rd-harmonic based sensor driver (via I2C)

bool adc_initialized = false; // ADC initialized flag

uint SET_SAMPLE_COUNT[SENSOR_CH_COUNT]; // Configured sample count buffer
uint SET_MEDIAN_SAMPLE_OFFSET[SENSOR_CH_COUNT]; // Configuration buffer for median sample offset
absolute_time_t last_pwm_read_timestamp[SENSOR_CH_COUNT];
uint16_t sample_index[SENSOR_CH_COUNT] = {0};

// Raw sensor output data
double periods[SENSOR_CH_COUNT];
double frequencies[SENSOR_CH_COUNT];
float voltages[SENSOR_CH_COUNT];

// Sensor magnetic output data (in nT)
float readings[SENSOR_CH_COUNT]; 

// Initializes the sensor unless it's disabled or already initialized in the same mode.
void init_sensor(size_t ch, SENSOR_MODE mode){
    SENSOR_STATES[ch] = SENSOR_MODES[ch] != SENSOR_MODE::DISABLED ? SENSOR_STATE::INACTIVE : SENSOR_STATE::DISABLED;
    if (SENSOR_MODES[ch] == SENSOR_MODE::DISABLED) return; // Don't initialize disabled sensors
    
    // Frequency driver
    if (SENSOR_MODES[ch] == SENSOR_MODE::FREQ){
        if (FREQ_INITIALIZED[ch]) return; // Don't reinitialize pwm counters
        const uint PIN = SENSOR_PINS_FREQ[ch];

        gpio_set_function(PIN, GPIO_FUNC_PWM); // Set GPIO function

        // Configure PWM
        uint slice_num = pwm_gpio_to_slice_num(PIN);
        pwm_config config = pwm_get_default_config();
        pwm_config_set_clkdiv(&config, 1.0f);
        pwm_config_set_clkdiv_mode(&config, PWM_DIV_B_RISING);
        assert(pwm_gpio_to_channel(PIN) == PWM_CHAN_B);

        // Initialize PWM
        pwm_init(slice_num, &config, true);
        pwm_set_counter(slice_num, 0);

        last_pwm_read_timestamp[ch] = get_absolute_time();
        FREQ_INITIALIZED[ch] = true;
    
    // Fast (onboard ADC) analog driver
    }else if (SENSOR_MODES[ch] == SENSOR_MODE::ANALOG){
        if(!adc_initialized) adc_init(); adc_initialized = true;
        if (ANALOG_INITIALIZED[ch]) return; // Don't reinitialize ADC
        
        const int8_t PIN = SENSOR_PINS_ANALOG[ch];
        
        if (PIN == -1) return; // ADC pin unavailable / unsupported. Don't initialize
        
        adc_gpio_init(PIN);
        ANALOG_INITIALIZED[ch] = true;
    
    // Precise (ADS1115) analog driver
    }else if (SENSOR_MODES[ch] == SENSOR_MODE::HARMONIC){
        return; // Precise W.I.P.
    }
    load_sample_count(ch); // Load default or configured sample count
    
}

// Initializes all non-disabled sensors
void init_sensors(){
    for(int ch = 0; ch < SENSOR_CH_COUNT; ch++){
        init_sensor(ch, SENSOR_MODES[ch]);
    }
}

// Deactivates sensor (doesn't deinitialize PWM/ADC/I2C!)
void deactivate_sensor(size_t ch){
    SENSOR_MODES[ch] = SENSOR_MODE::DISABLED;
    SENSOR_STATES[ch] = SENSOR_STATE::DISABLED;
}

// Frequency sensor driver
void read_sensor_freq(uint8_t ch){

    static freq_sample freq_samples[SENSOR_CH_COUNT][MAX_SAMPLE_COUNT]; // Samples in delta us
    const uint PIN = SENSOR_PINS_FREQ[ch];

    uint slice_num = pwm_gpio_to_slice_num(PIN);
    uint16_t count = pwm_get_counter(slice_num); // Read counter
    
    pwm_set_counter(slice_num, 0); // Reset counter
    
    absolute_time_t now = get_absolute_time(); // Get now timestamp

    if(count != 0){
        uint difference_us = absolute_time_diff_us(last_pwm_read_timestamp[ch], now); // calculate time difference
        
        // Save sample (time delta, count)
        freq_sample sample = {difference_us, count};
        freq_samples[ch][sample_index[ch]++] = sample;

        SENSOR_STATES[ch] = SENSOR_STATE::ACTIVE;

        //float frequency_sample = difference_us > 0 ? (float)count / ((float)difference_us / 1000000.0f) : 0.0f; // calculate frequency sample in Hz

        if(sample_index[ch] >= SAMPLE_COUNT[ch]){ // Once we reach the configured sample count
            static double period_samples[SENSOR_CH_COUNT][MAX_SAMPLE_COUNT];

            for(size_t i = 0; i < SAMPLE_COUNT[ch]; i++)
                period_samples[ch][i] = (double)freq_samples[ch][i].delta_t / (double)freq_samples[ch][i].count;


            periods[ch] = SAMPLE_COUNT[ch] > 1 ? math::get_filtered_average(period_samples[ch], SAMPLE_COUNT[ch], MEDIAN_SAMPLE_OFFSET[ch]) : period_samples[ch][0]; // Get filtered time delta

            frequencies[ch] = (1000000.0f / periods[ch]);
            calculate_nT(ch);
            sample_index[ch] = 0;
        }
    }else{ // No signal from sensor detected / sensor uninitialized
        SENSOR_STATES[ch] = SENSOR_STATE::INACTIVE;
        sample_index[ch] = 0;
    }

    last_pwm_read_timestamp[ch] = now;  
}

void read_sensor_adc(uint8_t ch){
    const int8_t PIN = SENSOR_PINS_ANALOG[ch];
    if(PIN == -1) return; // If analog pin unavailable, skip

    adc_select_input(ch);
    uint32_t total = 0;
    for (int i = 0; i < SAMPLE_COUNT[ch]; i++) total += adc_read();
    voltages[ch] = (total / (float)SAMPLE_COUNT[ch]) * ANALOG_CONVERSION_FACTOR;
    if(voltages[ch] > 0.5){
        SENSOR_STATES[ch] = SENSOR_STATE::ACTIVE;
        calculate_nT(ch);
    }else{
        SENSOR_STATES[ch] = SENSOR_STATE::INACTIVE;
    }
}

// Get sensor frequencies
void read_sensors(){
    for (int ch = 0; ch < SENSOR_CH_COUNT; ch++){
        if (SENSOR_STATES[ch] == SENSOR_STATE::DISABLED) continue; // Dont read from disabled sensors
        
        if (SENSOR_MODES[ch] == SENSOR_MODE::FREQ && FREQ_INITIALIZED[ch]) read_sensor_freq(ch); // Frequency driver   
        else if (SENSOR_MODES[ch] == SENSOR_MODE::ANALOG && ANALOG_INITIALIZED[ch]) read_sensor_adc(ch); // Analog driver
        else if (SENSOR_MODES[ch] == SENSOR_MODE::HARMONIC && I2C_INITIALIZED[ch]) continue; // TODO: 3rd harmonic driver W.I.P. (via I2C)
    }
    
}

/*void calculate_nT(uint8_t ch){
    CALIB_DATA ch_data = SENSOR_CALIBRATIONS[ch];
    float result_tesla;
    float slope = (B_MAX - B_MIN) / (ch_data.MAX - ch_data.MIN);
    switch(SENSOR_MODES[ch]){
        case SENSOR_MODE::FREQ:
            result_tesla = B_MIN + (periods[ch] - ch_data.MIN) * slope; // Result in tesla
            break;
        case SENSOR_MODE::ANALOG:
            result_tesla = B_MIN + (voltages[ch] - ch_data.MIN) * slope; // Result in tesla
            break;
        case SENSOR_MODE::HARMONIC: // TODO: 3rd harmonic driver W.I.P. (via I2C)
            break;
        };
    float result_nT = result_tesla * 1e9f; // Result in nT
    readings[ch] = result_nT;
}*/

void calculate_calib(uint8_t ch){
    CALIB_DATA& ch_data = SENSOR_CALIBRATIONS[ch];
    ch_data.offset = (ch_data.MAX + ch_data.MIN) / 2.0;
    ch_data.slope = (B_MAX - B_MIN) / (ch_data.MAX - ch_data.MIN);
}

void calculate_nT(uint8_t ch){
    CALIB_DATA ch_data = SENSOR_CALIBRATIONS[ch];

    double B_measured = 0.0f;
    switch(SENSOR_MODES[ch]){
        case SENSOR_MODE::FREQ:
            B_measured = periods[ch];
            break;
        case SENSOR_MODE::ANALOG:
            B_measured = voltages[ch];
            break;
        case SENSOR_MODE::HARMONIC: // TODO: 3rd harmonic driver W.I.P. (via I2C)
            break;
        default:
            break;
    }
    double result_tesla = (B_measured - ch_data.offset) * ch_data.slope;

    double result_nT = result_tesla * 1e9f; // Result in nT
    readings[ch] = result_nT;
}

float get_nT(uint8_t ch){
    return readings[ch];
}

// Target sensor(s) have to be initialized after!
void set_sensor_mode(uint8_t ch, SENSOR_MODE mode){
    SENSOR_MODES[ch] = mode;
}
// Target sensor(s) have to be initialized after!
void set_sensor_modes(SENSOR_MODE modes[SENSOR_CH_COUNT]){
    for(size_t i = 0; i < SENSOR_CH_COUNT; i++){
        SENSOR_MODES[i] = modes[i];
    }
}

void set_sensor_calib(uint8_t ch, CALIB_DATA calib_data){
    SENSOR_CALIBRATIONS[ch] = calib_data;
    calculate_calib(ch);
}
void set_sensor_calibs(CALIB_DATA calib_data[SENSOR_CH_COUNT]){
    for(int ch = 0; ch < SENSOR_CH_COUNT; ch++){
        SENSOR_CALIBRATIONS[ch] = calib_data[ch];
        calculate_calib(ch);
    }
}

// Sets the used sample count to the given value.
void set_sample_count(uint8_t ch, uint sample_count){
    SAMPLE_COUNT[ch] = sample_count;
}
// Sets all used sample counts to the given array of counts.
void set_sample_counts(uint sample_counts[SENSOR_CH_COUNT]){
    for(size_t i = 0; i < SENSOR_CH_COUNT; i++){
        SAMPLE_COUNT[i] = sample_counts[i];
    }
}
// Sets the used sample count to the corresponding value from the SET_SAMPLE_COUNT buffer.
void load_sample_count(uint8_t ch){
    SAMPLE_COUNT[ch] = SET_SAMPLE_COUNT[ch];
}
// Sets all the used sample counts to the corresponding values from the SET_SAMPLE_COUNT buffer.
void load_sample_counts(){
    for(size_t i = 0; i < SENSOR_CH_COUNT; i++){
        load_sample_count(i);
    }
}
// Saves the current sample count to the SET_SAMPLE_COUNT buffer.
void save_sample_count(uint8_t ch){
    SET_SAMPLE_COUNT[ch] = SAMPLE_COUNT[ch];
}
// Saves all the current sample counts to the SET_SAMPLE_COUNT buffer.
void save_sample_counts(){
    for(size_t i = 0; i < SENSOR_CH_COUNT; i++){
        save_sample_count(i);
    }
}

// Median offsets:


// Sets the used median sample offset to the given value.
void set_median_offset(uint8_t ch, uint offset){
    MEDIAN_SAMPLE_OFFSET[ch] = offset;
}
// Sets the used median sample offsets to the given array of offsets.
void set_median_offsets(uint sample_offsets[SENSOR_CH_COUNT]){
    for(size_t i = 0; i < SENSOR_CH_COUNT; i++){
        MEDIAN_SAMPLE_OFFSET[i] = sample_offsets[i];
    }
}
// Sets the used median sample offset to the corresponding value from the SET_MEDIAN_SAMPLE_OFFSET buffer.
void load_median_offset(uint8_t ch){
    MEDIAN_SAMPLE_OFFSET[ch] = SET_MEDIAN_SAMPLE_OFFSET[ch];
}
// Sets all the used median sample offsets to the corresponding values from the SET_MEDIAN_SAMPLE_OFFSET buffer.
void load_median_offsets(){
    for(size_t i = 0; i < SENSOR_CH_COUNT; i++){
        load_median_offset(i);
    }
}
// Saves the current median sample offset to the SET_MEDIAN_SAMPLE_OFFSET buffer.
void save_median_offset(uint8_t ch){
    SET_MEDIAN_SAMPLE_OFFSET[ch] = MEDIAN_SAMPLE_OFFSET[ch];
}
// Saves all current median sample offsets to the SET_MEDIAN_SAMPLE_OFFSET buffer.
void save_median_offsets(){
    for(size_t i = 0; i < SENSOR_CH_COUNT; i++){
        save_median_offset(i);
    }
}

}

// User input (eg. buttons)
namespace ctrl{


std::vector<BTN_EVENT> events;

int get_event_index(size_t id){
    for(int i = 0; i < events.size(); i++){
        if(events[i].id == id) return i; 
    }
    return -1;
}

bool id_exists(size_t id){
    for(auto event : events){
        if(event.id == id) return true;
    }
    return false;
}

size_t generate_id(){
    srand(time_us_32());
    size_t id = rand() % 100; // 0-99 ; max 100 events possible at once
    return id;
}

size_t connect(BUTTON_t button, BTN_EVENT_t event_t, std::function<void()> callback){
    size_t id;
    do{id = generate_id();} while (id_exists(id)); // Get unique id for event
    BTN_EVENT event = {id, true, button, event_t, std::move(callback)};
    events.push_back(event);
    return id;
}

void emit(BTN_EVENT &event){
    event.callback();
}

void set_enabled(size_t id, bool enabled){
    events[get_event_index(id)].enabled = enabled;
}

void disconnect(size_t id){
    events.erase(events.begin() + get_event_index(id));
}

const uint8_t btn_count = 3;
bool btn_states[btn_count] = {false, false, false};
bool prev_btn_states[btn_count] = {false, false, false};
absolute_time_t btn_states_time[btn_count] = {0, 0, 0};

void update_btn_states(){
    for(int i = 0; i < btn_count; i++){
        btn_states[i] = !get_state((BUTTON_t) i); // Reacts to falling edge
    }
}
void update_prev_btn_states(){
    for(int i = 0; i < btn_count; i++){
        prev_btn_states[i] = btn_states[i];
    }
}

void update_btn_states_time(){
    for(int i = 0; i < btn_count; i++){
        if(btn_states[i] != prev_btn_states[i]){
            btn_states_time[i] = get_absolute_time();
        }
    }
}

void handle_events(){ // call in the main loop
    update_btn_states();
    update_btn_states_time();
    for(auto &event : events){
        if(!event.enabled) continue;
        switch(event.event){
            case BTN_EVENT_t::BTN_DOWN:
                if(btn_states[(size_t)event.button]) emit(event);
                break;
            case BTN_EVENT_t::BTN_UP:
                if(!btn_states[(size_t)event.button]) emit(event);
                break;
            case BTN_EVENT_t::BTN_PRESSED:
                if(btn_states[(size_t)event.button] && !prev_btn_states[(size_t)event.button]) emit(event);
                break;
            case BTN_EVENT_t::BTN_RELEASED:
                if(!btn_states[(size_t)event.button] && prev_btn_states[(size_t)event.button]) emit(event);
                break;
            case BTN_EVENT_t::BTN_HOLD:
                if(btn_states[(size_t)event.button] && get_absolute_time() - btn_states_time[(size_t)event.button] >= BTN_HOLD_TRIGGER_TIME){
                    emit(event);
                    btn_states_time[(size_t)event.button] = get_absolute_time();
                }
                break;
            default: break;
        }
    }
    update_prev_btn_states();
}



void init_btn(){
    // Configure & pull GPIO-pins
    for(auto pin : BTN_PINS){
        gpio_set_function(pin, GPIO_FUNC_SIO);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }
    // Set states for events
    update_btn_states();
    update_prev_btn_states();
    update_btn_states_time();
}

int get_state(BUTTON_t button){
    return gpio_get(BTN_PINS[(size_t)button]);
}
int get_state(size_t button_index){
    return gpio_get(BTN_PINS[button_index]);
}


}

namespace status{
    void init_led(){
        // Status LED
        gpio_set_function(STATUS_LED, GPIO_FUNC_SIO);
        gpio_set_dir(STATUS_LED, GPIO_OUT);
    }
    void set_led(bool state){
        gpio_put(STATUS_LED, state);
    }
}



namespace rtc{
    ds3231_rtc ds3231;
    uint64_t microseconds;
    
    // microsecond tracking, since RTC doesn't
    void init_micros(){
        ds3231_datetime_t dt;
        get_datetime(&dt);
        uint prev_seconds = dt.seconds;
        do{
            get_datetime(&dt);
        }while(prev_seconds == dt.seconds);
        microseconds = 0;
    }
    uint64_t get_micros(){
        return microseconds;
    }
    void track_micros(){
        static absolute_time_t last_track = get_absolute_time();

        absolute_time_t time = get_absolute_time();
        microseconds += (time - last_track);

        if(microseconds >= 1000000){ // One second passed
            uint64_t micros_overshoot = microseconds - 1000000;
            microseconds = micros_overshoot;
        }

        last_track = time;
    }

    void loop(){
        track_micros();
    }

    void init_rtc(){
        ds3231_init(RTC_I2C_PORT, RTC_I2C_SDA, RTC_I2C_SCL, &ds3231);
        init_micros();
    }

    void set_datetime(ds3231_datetime_t *datetime){
        ds3231_set_datetime(datetime, &ds3231);
    }
    
    void get_datetime(ds3231_datetime_t *datetime){
        ds3231_get_datetime(datetime, &ds3231);
    }

    float get_temperature(){
        float temperature;
        ds3231_get_temperature(&temperature, &ds3231);
        return temperature;
    }
}