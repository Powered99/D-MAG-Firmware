# D-MAG-Firmware
A vastly configurable Flux-Gate-Magnetometer firmware for the Raspberry Pi Pico used to read from up to 4 FGM3+ fluxgate magnetometers (via PWM or ADC).

The D-MAG-Firmware is still **in early development** and has **only been published to show its current progress!**
Therefor, it is **NOT MEANT FOR PUBLIC USE** yet!


# Features:
- Read up to 4 PWM FGM3+ sensors, which output a frequency range
- Read up to 4 Analog FGM3+ sensors, which output a voltage range
- (Auto-detect sensors on non-disabled channels)
- Take up to 2048 sensor samples and filter them using an averaged median array (up to 128 filtered samples)
- Log data with accurate date/time on an SD card
- Display sensor readings / configurations, etc. in the UI
- Configure multiple settings / factors (like sample count (median sample count setting is yet to be implemented!)) at runtime.


## Upcoming features I'm working on:

- Running measurements on CORE #1 (to avoid overhead from other system components, thus improving precision and speed)
- Saving settings / sensor calibrations in NVM (Non-volatile-memory, probably flash or SD)
- Some missing or unfinished settings in the settings page (like runtime RTC configuration or median sample count)
- Compatibility with EAS-based DIY sensors over the ADC fast mode
- I2C ADS1115 ADC compatibility

Please keep in mind that this is currently being developed for private use, which could mean,
that some of these upcoming features or bugs could take a long time to or never become finished or fixed.

You can still use it though! - If you do, please credit me in your project if you're publishing it / it's documentation.



# Usage:
## Connect components:

ST7735 Display
- SCK  -> 18
- TX   -> 19
- RX   -> 16
- CS   -> 17
- DC   -> 20
- RST  -> 21

SD Card
- SCK  -> 10
- TX   -> 11
- RX   -> 12
- CS   -> 13

RTC DS3231
- SDA  -> 14
- SCL  -> 15

Buttons
- Button Left -> 22
- Button Select -> 26
- Button Right -> 27

### Sensors:
*(For 5V pwm sensors, make sure to use a level shifter!)*

Frequency / PWM sensors
- Sensor 0 -> 9
- Sensor 1 -> 7
- Sensor 2 -> 5
- Sensor 3 -> 3

Analog [FAST MODE] sensors [WIP] *Button pins have to be rewired, for now: testing only!*
- Sensor 0 -> 26
- Sensor 1 -> 27
- Sensor 2 -> 28
- Sensor 3 -> (unsupported on the official Pico)


## Compile & Upload code
Use VSCode with the Raspberry Pi Pico extension to compile and flash the code.
Make sure your Pico is in BOOTSEL mode for flashing!




# Credits:
## D-MAG-Firmware
D-MAG-Firmware by Dominik Kultys

## Third party libraries:
ads1x15: https://github.com/gavinlyonsrepo/ADS1x15_PICO

ds3231: https://github.com/antgon/pico-ds3231

displaylib_16: https://github.com/gavinlyonsrepo/displaylib_16bit_PICO

fatfs library: https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico