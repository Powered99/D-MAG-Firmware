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
#include "Datalogger.hpp"

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

volatile uint SET_SAMPLE_COUNT[SENSOR_CH_COUNT]; // Configured sample count buffer
volatile uint SET_MEDIAN_SAMPLE_OFFSET[SENSOR_CH_COUNT]; // Configuration buffer for median sample offset
absolute_time_t last_pwm_read_timestamp[SENSOR_CH_COUNT];
absolute_time_t last_pwm_read_timeout[SENSOR_CH_COUNT];
uint16_t sample_index[SENSOR_CH_COUNT] = {0};

static double period_samples[SENSOR_CH_COUNT][MAX_SAMPLE_COUNT];
static int adc_samples[SENSOR_CH_COUNT][MAX_SAMPLE_COUNT];

double periods[SENSOR_CH_COUNT];

double frequencies_local[SENSOR_CH_COUNT]; // Local sensor raw frequency output data (in Hz) on Core1
float voltages_local[SENSOR_CH_COUNT]; // Local sensor raw voltage output data (in V) on Core1
float readings_local[SENSOR_CH_COUNT]; // Local sensor magnetic output data (in nT) on Core1

volatile double frequencies[SENSOR_CH_COUNT]; // Synced sensor raw frequency output data (in Hz)
volatile float voltages[SENSOR_CH_COUNT];   // Synced sensor raw voltage output data (in V)
volatile float readings[SENSOR_CH_COUNT]; // Synced sensor magnetic output data (in nT)
static spin_lock_t *readings_lock;

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

        absolute_time_t now = get_absolute_time();

        last_pwm_read_timestamp[ch] = now;
        last_pwm_read_timeout[ch] = now;

        FREQ_INITIALIZED[ch] = true;
    
    // Fast (onboard ADC) analog driver
    }else if (SENSOR_MODES[ch] == SENSOR_MODE::ANALOG){
        if(!adc_initialized) adc_init(); adc_initialized = true;
        if (ANALOG_INITIALIZED[ch]) return; // Don't reinitialize ADC
        
        const int8_t PIN = SENSOR_PINS_ANALOG[ch];
        
        if (PIN == -1) return; // ADC pin unavailable / unsupported. Don't initialize
        
        adc_gpio_init(PIN);
        ANALOG_INITIALIZED[ch] = true;
    
    
    }else if (SENSOR_MODES[ch] == SENSOR_MODE::HARMONIC){
        return; // W.I.P.
    
    // Precise (ADS1115) analog driver (doesn't use configured sample count, uses an onboard hardware sampling solution instead)
    }else if (SENSOR_MODES[ch] == SENSOR_MODE::ANALOG_ADS1115){
        if(ads1115::ENABLE_ADS1115)
            ads1115::init_ads1115(); // Make sure ads1115 is intialized. Note: Reinitialization safety already in init_ads1115
        else deactivate_sensor(ch); // If ADS1115 is disabled in config, disable the sensor channel.
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

void polling_thread(){
    multicore_lockout_victim_init();
    while(true) read_sensors();
}

// Starts Core 1 sensor polling thread
void launch_polling(){
    readings_lock = spin_lock_init(spin_lock_claim_unused(true));
    multicore_launch_core1(polling_thread);
}

// Frequency sensor driver
void read_sensor_freq(uint8_t ch){
    const uint PIN = SENSOR_PINS_FREQ[ch];

    uint slice_num = pwm_gpio_to_slice_num(PIN);
    uint16_t count = pwm_get_counter(slice_num); // Read counter
    
    absolute_time_t now = get_absolute_time(); // Get now timestamp


    if(count >= MIN_IMPULSES_COUNT){
        uint difference_us = absolute_time_diff_us(last_pwm_read_timestamp[ch], now); // calculate time difference
        last_pwm_read_timestamp[ch] = now; 
        last_pwm_read_timeout[ch] = now;
        // Save sample (time delta, count)
        period_samples[ch][sample_index[ch]++] = (double)difference_us / (double)count;

        SENSOR_STATES[ch] = SENSOR_STATE::ACTIVE;

        pwm_set_counter(slice_num, 0); // Reset counter
        //float frequency_sample = difference_us > 0 ? (float)count / ((float)difference_us / 1000000.0f) : 0.0f; // calculate frequency sample in Hz

        if(sample_index[ch] >= SAMPLE_COUNT[ch]){ // Once we reach the configured sample count
            periods[ch] = SAMPLE_COUNT[ch] > 1 ? math::get_filtered_average(period_samples[ch], SAMPLE_COUNT[ch], MEDIAN_SAMPLE_OFFSET[ch]) : period_samples[ch][0]; // Get filtered time delta

            frequencies_local[ch] = (1000000.0f / periods[ch]);
            calculate_nT(ch);
            sample_index[ch] = 0;
        }
    }else if(absolute_time_diff_us(last_pwm_read_timeout[ch], now) >= FREQ_TIMEOUT){ // No signal from sensor detected / sensor uninitialized
        SENSOR_STATES[ch] = SENSOR_STATE::INACTIVE;
        sample_index[ch] = 0;
        
    }
}

void read_sensor_adc(uint8_t ch){
    const int8_t PIN = SENSOR_PINS_ANALOG[ch];

    if(PIN == -1) return; // If analog pin unavailable, skip
    
    SENSOR_STATES[ch] = SENSOR_STATE::ACTIVE;

    adc_select_input(ch);
    adc_samples[ch][sample_index[ch]++] = adc_read();

    if(sample_index[ch] >= SAMPLE_COUNT[ch]){
        double filter_result = SAMPLE_COUNT[ch] > 1 ? math::get_filtered_average(adc_samples[ch], SAMPLE_COUNT[ch], MEDIAN_SAMPLE_OFFSET[ch]) : (double)adc_samples[ch][0];

        voltages_local[ch] = filter_result * ANALOG_CONVERSION_FACTOR;
        calculate_nT(ch);
        sample_index[ch] = 0;
    }
}

void read_channels_ads1115(){
    static absolute_time_t last_poll_timestamp = 0;
    if(!ads1115::ENABLE_ADS1115) return;

    absolute_time_t now = get_absolute_time();
    if(absolute_time_diff_us(last_poll_timestamp, now) >= ads1115::ADS1115_POLLING_RATE_US){
        for(uint8_t ch = 0; ch < SENSOR_CH_COUNT; ch++){
            if(SENSOR_MODES[ch] != SENSOR_MODE::ANALOG_ADS1115) continue;
            SENSOR_STATES[ch] = SENSOR_STATE::ACTIVE;

            float voltage = ads1115::read_volts(ch);

            uint32_t save = spin_lock_blocking(readings_lock);
            voltages[ch] = voltage;
            spin_unlock(readings_lock, save);

        }
        last_poll_timestamp = now;
    }
}

// Get sensor frequencies
void read_sensors(){
    for (uint8_t ch = 0; ch < SENSOR_CH_COUNT; ch++){
        if (SENSOR_STATES[ch] == SENSOR_STATE::DISABLED) continue; // Dont read from disabled sensors
        
        if (SENSOR_MODES[ch] == SENSOR_MODE::FREQ && FREQ_INITIALIZED[ch]) read_sensor_freq(ch); // Frequency driver   
        else if (SENSOR_MODES[ch] == SENSOR_MODE::ANALOG && ANALOG_INITIALIZED[ch]) read_sensor_adc(ch); // Analog driver
        else if (SENSOR_MODES[ch] == SENSOR_MODE::HARMONIC && I2C_INITIALIZED[ch]) continue; // TODO: 3rd harmonic driver W.I.P. (via I2C)
    }

    // Sync to shared buffer
    uint32_t save = spin_lock_blocking(readings_lock);
    for(uint8_t ch = 0; ch < SENSOR_CH_COUNT; ch++){
        readings[ch] = readings_local[ch];
        if(SENSOR_MODES[ch] == SENSOR_MODE::FREQ) frequencies[ch] = frequencies_local[ch];
        else if(SENSOR_MODES[ch] == SENSOR_MODE::ANALOG) voltages[ch] = voltages_local[ch];
    }
    spin_unlock(readings_lock, save);
}

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
            B_measured = voltages_local[ch];
            break;
        case SENSOR_MODE::HARMONIC: // TODO: 3rd harmonic driver W.I.P. (via I2C)
            break;
        default:
            break;
    }
    double result_tesla = (B_measured - ch_data.offset) * ch_data.slope;

    double result_nT = result_tesla * 1e9f; // Result in nT
    readings_local[ch] = result_nT;
}

// Get given channel's current reading in nT
float get_nT(uint8_t ch){
    // Sync from Core1
    uint32_t save = spin_lock_blocking(readings_lock);
    float result_nT = readings[ch];
    spin_unlock(readings_lock, save);
    // Return value
    return result_nT;
}

float get_volts(uint8_t ch){
    if(SENSOR_MODES[ch] == SENSOR_MODE::ANALOG || SENSOR_MODES[ch] == SENSOR_MODE::ANALOG_ADS1115){
        uint32_t save = spin_lock_blocking(readings_lock);
        float result_V = voltages[ch];
        spin_unlock(readings_lock, save);
        return result_V;
    }else return 0.0f;
}

double get_hz(uint8_t ch){
    if(SENSOR_MODES[ch] == SENSOR_MODE::FREQ){
        uint32_t save = spin_lock_blocking(readings_lock);
        float result_Hz = frequencies[ch];
        spin_unlock(readings_lock, save);
        return result_Hz;
    }else return 0.0f;
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
void apply_sample_count(uint8_t ch){
    SET_SAMPLE_COUNT[ch] = SAMPLE_COUNT[ch];
}
// Saves all the current sample counts to the SET_SAMPLE_COUNT buffer.
void apply_sample_counts(){
    for(size_t i = 0; i < SENSOR_CH_COUNT; i++){
        apply_sample_count(i);
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
void apply_median_offset(uint8_t ch){
    SET_MEDIAN_SAMPLE_OFFSET[ch] = MEDIAN_SAMPLE_OFFSET[ch];
}
// Saves all current median sample offsets to the SET_MEDIAN_SAMPLE_OFFSET buffer.
void apply_median_offsets(){
    for(size_t i = 0; i < SENSOR_CH_COUNT; i++){
        apply_median_offset(i);
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
    absolute_time_t timer_timestamp;
    absolute_time_t timer_duration;
    bool timer_led_state;
    bool timer_set = false;

    void init_led(){
        gpio_set_function(STATUS_LED, GPIO_FUNC_SIO);
        gpio_set_dir(STATUS_LED, GPIO_OUT);
    }
    void set_led(bool state){
        gpio_put(STATUS_LED, state);
    }
    void set_led(bool state, absolute_time_t duration){
        set_led(state);
        timer_led_state = !state;
        timer_set = true;
        timer_timestamp = get_absolute_time();
        timer_duration = duration;
    }
    void loop(){
        if(timer_set && get_absolute_time() - timer_timestamp >= timer_duration){
            set_led(timer_led_state);
            timer_set = false;
        }
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
        ds3231_init(I2C_PORT, I2C_SDA, I2C_SCL, &ds3231);
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


namespace ads1115{
    bool ads1115_initialized = false;
    struct ads1115_adc adc;

    void configure_ads1115(ads1115_pga_t pga, ads1115_rate_t rate){
        if(!ads1115_initialized) return;
        ads1115_set_pga(pga, &adc);
        ads1115_set_data_rate(rate, &adc);
        ads1115_write_config(&adc);
    }

    void init_ads1115(){
        if(ads1115_initialized) return; // Don't reinitialize
        ads1115_init(I2C_PORT, 0x48, &adc);
        ads1115_initialized = true;
        configure_ads1115(ADS1115_PGA_4_096, ADS1115_RATE_64_SPS); // default config
    }

    uint16_t read_raw(ads1115_mux_t mux){
        if(!ads1115_initialized) return 0;
        ads1115_set_input_mux(mux, &adc);
        ads1115_write_config(&adc);        

        uint16_t adc_val;
        ads1115_read_adc(&adc_val, &adc);
        return adc_val;
    }

    uint16_t read_raw_single(uint8_t ch){
        if(!ads1115_initialized || ch >= 4) return 0; // Only channels 0-3 are valid

        ads1115_mux_t mux;
        switch (ch) {
            case 0: mux = ADS1115_MUX_SINGLE_0; break;
            case 1: mux = ADS1115_MUX_SINGLE_1; break;
            case 2: mux = ADS1115_MUX_SINGLE_2; break;
            case 3: mux = ADS1115_MUX_SINGLE_3; break;
        }
        return read_raw(mux);
    }

    float read_volts(uint8_t ch){
        uint16_t raw = read_raw_single(ch);
        return ads1115_raw_to_volts(raw, &adc);
    }

    float read_volts(ads1115_mux_t mux){
        uint16_t raw = read_raw(mux);
        return ads1115_raw_to_volts(raw, &adc);
    }
}



namespace nvm { // Non-volatile-memory (flash) storage for settings / configurations / calibrations

static uint32_t compute_checksum(const Block& b) {
    uint32_t sum = 0;
    auto acc = [&](const void* data, size_t len) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
        for (size_t i = 0; i < len; ++i) sum += p[i];
    };

    acc(&b.magic,           sizeof(b.magic));
    acc(&b.version,         sizeof(b.version));
    acc(&b.channel_count,   sizeof(b.channel_count));
    acc(&b.data_format,     sizeof(b.data_format));
    acc(&b.log_interval_ms, sizeof(b.log_interval_ms));
    acc(&b.logging_status,  sizeof(b.logging_status));

    for (uint8_t i = 0; i < MAX_CH; ++i) {
        acc(&b.calibrations[i].offset, sizeof(double));
        acc(&b.calibrations[i].slope,  sizeof(double));
        acc(&b.calibrations[i].min,    sizeof(float));
        acc(&b.calibrations[i].max,    sizeof(float));
        acc(&b.sensor_modes[i],        sizeof(int8_t));
        acc(&b.sample_counts[i],       sizeof(uint32_t));
        acc(&b.median_sample_offsets[i], sizeof(uint32_t));
    }

    return sum;
}

static const Block* flash_block() {
    return reinterpret_cast<const Block*>(XIP_BASE + FLASH_OFFSET);
}

static Block pack() {
    Block b;
    memset(&b, 0, sizeof(Block));  // guarantees all padding bytes are zero

    b.magic           = MAGIC;
    b.version         = VERSION;
    b.channel_count   = fgm::SENSOR_CH_COUNT;
    b.data_format     = static_cast<uint8_t>(logger::DATA_FORMAT);
    b.log_interval_ms = logger::log_interval_ms;
    b.logging_status  = static_cast<uint8_t>(logger::logging_status);

    for (uint8_t i = 0; i < fgm::SENSOR_CH_COUNT; ++i) {
        b.calibrations[i] = { fgm::SENSOR_CALIBRATIONS[i].offset,
                            fgm::SENSOR_CALIBRATIONS[i].slope,
                            fgm::SENSOR_CALIBRATIONS[i].MIN,
                            fgm::SENSOR_CALIBRATIONS[i].MAX };
        b.sensor_modes[i]          = static_cast<int8_t>(fgm::SENSOR_MODES[i]);
        b.sample_counts[i]         = fgm::SAMPLE_COUNT[i];
        b.median_sample_offsets[i] = fgm::MEDIAN_SAMPLE_OFFSET[i];
    }

    b.checksum = compute_checksum(b);
    return b;
}

static void unpack(const Block& b) {
    const uint8_t ch = (b.channel_count < fgm::SENSOR_CH_COUNT)
                        ? b.channel_count : fgm::SENSOR_CH_COUNT;

    for (uint8_t i = 0; i < ch; ++i) {
        fgm::SENSOR_CALIBRATIONS[i].offset = b.calibrations[i].offset;
        fgm::SENSOR_CALIBRATIONS[i].slope  = b.calibrations[i].slope;
        fgm::SENSOR_CALIBRATIONS[i].MIN    = b.calibrations[i].min;
        fgm::SENSOR_CALIBRATIONS[i].MAX    = b.calibrations[i].max;
        fgm::SENSOR_MODES[i]               = static_cast<fgm::SENSOR_MODE>(b.sensor_modes[i]);
        fgm::SAMPLE_COUNT[i]               = b.sample_counts[i];
        fgm::MEDIAN_SAMPLE_OFFSET[i]       = b.median_sample_offsets[i];
    }

    logger::DATA_FORMAT     = static_cast<logger::FORMATS>(b.data_format);
    logger::log_interval_ms = b.log_interval_ms;
    logger::logging_status  = static_cast<logger::LOG_STATUS>(b.logging_status);
    fgm::apply_sample_counts();
    fgm::apply_median_offsets();
}

bool check() {
    const Block* b = flash_block();
    if (b->magic    != MAGIC)             return false;
    if (b->version  != VERSION)           return false;
    if (b->checksum != compute_checksum(*b)) return false;
    return true;
}

bool load() {
    const Block* b = flash_block();
    if (!check()) return false;
    unpack(*b);
    status::set_led(true, 500*250); // Indicate successful load with LED
    return true;
}

void save() {
    status::set_led(true, 1000*500); // Indicate saving with LED
    Block b = pack();

    uint8_t page[FLASH_PAGE_SIZE] = {};
    memcpy(page, &b, sizeof(Block));

    multicore_lockout_start_blocking();

    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_OFFSET, page, FLASH_PAGE_SIZE);
    restore_interrupts(irq);
    
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(XIP_BASE + FLASH_OFFSET);

    multicore_lockout_end_blocking();
}

void load_defaults(){
    fgm::set_sensor_modes((fgm::SENSOR_MODE[fgm::SENSOR_CH_COUNT]){fgm::SENSOR_MODE::FREQ, fgm::SENSOR_MODE::FREQ, fgm::SENSOR_MODE::DISABLED, fgm::SENSOR_MODE::DISABLED});
    //fgm::set_sensor_modes((fgm::SENSOR_MODE[fgm::SENSOR_CH_COUNT]){fgm::SENSOR_MODE::FREQ, fgm::SENSOR_MODE::FREQ, fgm::SENSOR_MODE::FREQ, fgm::SENSOR_MODE::FREQ});
    fgm::set_sample_counts((uint[fgm::SENSOR_CH_COUNT]){128, 128, 128, 128});
    //fgm::set_sample_counts((uint[fgm::SENSOR_CH_COUNT]){1, 1, 1, 1});
    fgm::set_median_offsets((uint[fgm::SENSOR_CH_COUNT]){32, 32, 32, 32});
    fgm::apply_sample_counts();
    fgm::apply_median_offsets();
    logger::set_log_interval(6000);
}

}

// Override FatFs timestamp function with DS3231 RTC data
extern "C" DWORD get_fattime(void) {
    ds3231_datetime_t dt;
    rtc::get_datetime(&dt);

    // Pack into FatFs DWORD format:
    // bits 31:25 — year since 1980 (0..127)
    // bits 24:21 — month (1..12)
    // bits 20:16 — day (1..31)
    // bits 15:11 — hour (0..23)
    // bits 10:5  — minute (0..59)
    // bits 4:0   — second / 2 (0..29)
    return ((DWORD)(dt.year - 1980) << 25)
         | ((DWORD)(dt.month)       << 21)
         | ((DWORD)(dt.day)         << 16)
         | ((DWORD)(dt.hour)        << 11)
         | ((DWORD)(dt.minutes)     <<  5)
         | ((DWORD)(dt.seconds / 2));
}