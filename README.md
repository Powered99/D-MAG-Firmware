## THIS IS THE DEV BRANCH! The project will likely not compile and can be buggy or even crash! Please check the stable branch for usage!

# D-MAG-Firmware
A vastly configurable Flux-Gate-Magnetometer firmware for the Raspberry Pi Pico (RP2040) used to read from up to 4 FGM3+ fluxgate magnetometers (via PWM or ADC).

The D-MAG-Firmware is still **in early development** and has **been published mostly just to show its current progress!**
Therefore, using it can still be buggy and incomplete.

Its currently being developed for private use, which could mean,
that some of these upcoming features or bugs could take a long time to or never become finished or fixed.

Feel free to use and modify it however! - If you do, please credit me in your project if you're publishing it / it's documentation.

# Features:
- Read up to 4 PWM FGM3+ sensors, which output a frequency range
- Read up to 4 Analog FGM3+ sensors, which output a voltage range
- (Auto-detect sensors on non-disabled channels)
- Take up to 2048 sensor samples and filter them using an averaged median array (up to 128 filtered samples)
- Log data with accurate date/time on an SD card
- Display sensor readings / configurations, etc. in the UI
- Configure multiple settings / factors (like sample count (median sample count setting is yet to be implemented!)) at runtime.


# Upcoming features I'm working on:

- Running measurements on CORE #1 (to avoid overhead from other system components, thus improving precision and speed)
- Saving settings / sensor calibrations in NVM (Non-volatile-memory, probably flash or SD)
- Some missing or unfinished settings in the settings page (like runtime RTC configuration or median sample count)
- Compatibility with EAS-based DIY sensors over I2C
- Other features, like display power-saving, further optimizations, etc.

# Usage:
## Connect components:
The hardware for the D-MAG-Firmware is in constant development as well. This is the pinout for the 2nd revision.
You can still use previous revisions of the device but you'll have to change the HW_V{version here} definition in the include/pins.h file to apply the corresponding pinout. (Example: For v1; #define HW_V2 -> #define HW_V1).

### ST7735 Display (SPI1)
| Signal | GPIO |
|--------|------|
| SCK    | 10   |
| TX     | 11   |
| RX     | 12   |
| CS     | 13   |
| RST    | 14   |
| DC     | 15   |
| BL*    | 8    | 

*(BL: Backlight pin, might not be present on some TFT modules. Display powersaving W.I.P.)*

### SD Card (SPI0)
| Signal | GPIO |
|--------|------|
| SCK    | 18   |
| TX     | 19   |
| RX     | 16   |
| CS     | 17   |

### RTC (DS3231, I2C0)
| Signal | GPIO |
|--------|------|
| SDA    | 20   |
| SCL    | 21   |

### Buttons
| Button | GPIO |
|--------|------|
| LEFT   | 2   |
| SELECT | 4   |
| RIGHT  | 6   |

### Frequency Sensors
| Sensor | GPIO |
|--------|------|
| CH0    | 9    |
| CH1    | 7    |
| CH2    | 5    |
| CH3    | 3    |

### Analog Sensors
| Sensor | GPIO |
|--------|------|
| CH0    | 28   |
| CH1    | 27   |
| CH2    | 26   |
| CH3    |  *Unavailable on official PICO*   |


## Compile & Upload code
Use VSCode with the Raspberry Pi Pico extension to compile and flash the code or copy the build/D-MAG-Firmware.uf2 file onto the Pico in BOOTSEL mode.


# Credits:
## D-MAG-Firmware by Dominik Kultys

## Third party libraries:
ads1x15: https://github.com/gavinlyonsrepo/ADS1x15_PICO

ds3231: https://github.com/antgon/pico-ds3231

displaylib_16: https://github.com/gavinlyonsrepo/displaylib_16bit_PICO

fatfs library: https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico