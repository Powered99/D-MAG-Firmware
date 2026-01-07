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

// RTC
#define RTC_I2C_PORT i2c1
#define RTC_I2C_SDA 14
#define RTC_I2C_SCL 15

// Buttons
const uint8_t BTN_PINS[3] = {22, 26, 27};

// Status LED
#define STATUS_LED PICO_DEFAULT_LED_PIN

// Sensor pins (must be equal to or more than the SENSOR_CH_COUNT)
const uint8_t SENSOR_PINS_FREQ[4] = {9, 7, 5, 3};
const int8_t SENSOR_PINS_ANALOG[4] = {26, 27, 28, -1};