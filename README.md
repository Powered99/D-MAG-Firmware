# THIS IS THE DEV BRANCH OF THE PROJECT! IT IS UNSTABLE AND WILL PROBABLY NOT WORK. PLEASE CHECK THE STABLE BRANCH

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

Also please keep in mind that this is just some software I made for private use.
You can still use it if you like though! - Just please make sure to credit me if you're planning on publishing the project.
As it's one of the many projects im working on, some of these features could take a longer time to complete or never see the light of day at all.
