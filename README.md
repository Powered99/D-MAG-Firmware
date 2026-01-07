# D-MAG-Firmware
A vastly configurable Flux-Gate-Magnetometer firmware for the Raspberry Pi Pico used to read from up to 4 FGM3+ fluxgate magnetometers (via PWM or ADC).

The D-MAG-Firmware is still **in early development** and has **only been published to show its current progress!**
Therefor, it is **NOT MEANT FOR PUBLIC USE** yet!

## Features:
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


# Credits:
## D-MAG-Firmware
D-MAG-Firmware by Dominik Kultys

## Libraries:
ads1x15: https://github.com/gavinlyonsrepo/ADS1x15_PICO

ds3231: https://github.com/antgon/pico-ds3231

displaylib_16: https://github.com/gavinlyonsrepo/displaylib_16bit_PICO

fatfs library: https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico