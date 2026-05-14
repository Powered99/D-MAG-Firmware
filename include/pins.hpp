/*
 * D-MAG-Firmware
 * Copyright (c) 2026 Dominik Kultys
 *
 * Licensed under the Apache v2.0 License.
 * See LICENSE file in the project root for full license information.
 */

#pragma once

// Hardware configuration. Available options: HW_V1, HW_V2
#define HW_V1

// Hardware ports / pinouts for the MAG-Station V1
#ifdef HW_V1
// TFT Display
#define TFT_SPI_PORT spi0

#define TFT_SPI_SCK 18
#define TFT_SPI_TX 19
#define TFT_SPI_RX 16
#define TFT_SPI_CS 17

#define TFT_SPI_DC 20
#define TFT_SPI_RST 21

// SD Card
#define SD_SPI_PORT spi1

#define SD_SPI_CS 13
#define SD_SPI_RX 12
#define SD_SPI_TX 11
#define SD_SPI_SCK 10

// I2C Port (RTC & ADS1115)
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

// Buttons
const uint8_t BTN_PINS[3] = {22, 26, 27};

// Status LED
#define STATUS_LED PICO_DEFAULT_LED_PIN

// Sensor pins (must be equal to or more than the SENSOR_CH_COUNT)
const uint8_t SENSOR_PINS_FREQ[4] = {9, 7, 5, 3};
const int8_t SENSOR_PINS_ANALOG[4] = {26, 27, 28, -1};

#endif



// Hardware ports / pinouts for the MAG-Station V2
#ifdef HW_V2
// TFT Display
#define TFT_SPI_PORT spi1

#define TFT_SPI_SCK 10
#define TFT_SPI_TX 11
#define TFT_SPI_RX 12
#define TFT_SPI_CS 13

#define TFT_SPI_DC 15
#define TFT_SPI_RST 14

// TFT Backlight
#define TFT_BL 8

// SD Card
#define SD_SPI_PORT spi0

#define SD_SPI_CS 17
#define SD_SPI_RX 16
#define SD_SPI_TX 19
#define SD_SPI_SCK 18

// I2C Port (RTC & ADS1115)
#define I2C_PORT i2c0
#define I2C_SDA 20
#define I2C_SCL 21

// Buttons
const uint8_t BTN_PINS[3] = {2, 4, 6};

// Status LED
#define STATUS_LED PICO_DEFAULT_LED_PIN

// Sensor pins (must be equal to or more than the SENSOR_CH_COUNT)
const uint8_t SENSOR_PINS_FREQ[4] = {9, 7, 5, 3};
const int8_t SENSOR_PINS_ANALOG[4] = {28, 27, 26, -1};

#endif