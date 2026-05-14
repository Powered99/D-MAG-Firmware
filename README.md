## THIS IS THE DEV BRANCH! The project can be buggy, crash or even not compile! Please check the stable branch for usage!

# D-MAG-Firmware
**Version: v1.9.4 DEV**  
A vastly configurable and very flexible geomagnetic observatory firmware for the Raspberry Pi Pico (RP2040) microcontroller.  
Currently supports a variety of sensors that output an analog signal or square wave (eg. FGM3-Plus/Pro).  
It reads, processes, displays and logs data, lets you configure settings / calibrations at runtime using an intuitive and powerful UI and more.  

The D-MAG-Firmware is still **in early development**. Therefore, using it can still be buggy and incomplete.

Its currently being developed mainly for private use, which could mean,
that some of these upcoming features or bugs could take a long time to or never become finished or fixed.

**Feel free to use and modify it however!** - If you do, please **credit me** in your project if you're publishing it / it's documentation.

# Features:
- Reads from 4 or more sensors (eg. FGM3-Pro/Plus, Hall-Effect, etc.), via PWM or ADCs (Auto-detects sensors on non-disabled channels)
- Utilizes both CPU-Cores to drastically improve performance
- Takes up to 2048 sensor samples and filters them using an averaged median array (up to 1024 filtered samples)
- Keeps track of time and reads temperature using RTC
- Logs data with accurate date/time on an SD card with a flexible logging system in various formats (temporarily only IAGA-2002)
- Backward compatibility with older hardware revisions
- Full control thanks to the intuitive and powerful UI:
  - Displays sensor readings / settings, etc. in the UI
  - Sensor calibrations, editable settings and configurations (eg. RTC setup) at runtime
- Saving and loading settings and calibrations to / from NVM (non-volatile-memory)
- Autonomous logger recovery from crashes / power outages using NVM flag

# Upcoming features I'm working on / planning:
- Support for ADS1115 ADCs and EAS-based DIY sensors over I2C
- Configuration for logging raw voltage / frequency values
- A new, flexible logging format - "DMAG-2026"
- Some extra runtime settings in the settings page (like log elements and formatting configuration)
- SD-card hot-plugging
- UI improvements, further bugfixes and optimizations, etc.

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

### SD Card (SPI0)
| Signal | GPIO |
|--------|------|
| SCK    | 18   |
| TX     | 19   |
| RX     | 16   |
| CS     | 17   |

### I2C (DS3231 RTC, ADS1115) (I2C0)
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

*NOTE: The buttons should pull the GPIO pin down when pressed.*

### Frequency Sensors
| Sensor | GPIO |
|--------|------|
| CH0    | 9    |
| CH1    | 7    |
| CH2    | 5    |
| CH3    | 3    |

***WARNING:*** *Make sure the RP2040 doesn't receive more than 3.3V at it's input! If your sensors output more, eg. 5V, use a level shifter to prevent damage.*

### Analog Sensors
| Sensor | GPIO |
|--------|------|
| CH0    | 28   |
| CH1    | 27   |
| CH2    | 26   |
| CH3    |  *Unavailable on official PICO*   |

***WARNING:*** *Make sure the RP2040 ADCs don't receive more than 3.3V at their inputs! If your sensors output more, use a voltage divider to prevent damage.*

If you need precise analog sensor readings, connect an ADS1115 to the I2C pins listed above.
Make sure to enable the ADS1115 driver in config.hpp by setting "ENABLE_ADS1115" under the "ads1115" namespace to true.

## Compile & Upload code
Clone this repository and use VSCode with the Raspberry Pi Pico extension to import the project.  
Compile and flash the code or copy the compiled build/D-MAG-Firmware.uf2 file onto the Pico in BOOTSEL mode.

# Credits:
## D-MAG-Firmware by Dominik Kultys

## Third party libraries:
pico-ads1115: https://github.com/antgon/pico-ads1115/blob/main/lib/ads1115.c

pico-ds3231: https://github.com/antgon/pico-ds3231

displaylib_16: https://github.com/gavinlyonsrepo/displaylib_16bit_PICO

no-OS-FatFS-SDI-RPi-Pico: https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico

# License:
D-MAG-Firmware
Copyright (c) 2026 Dominik Kultys
Licensed under the Apache v2.0 License.
See LICENSE file in the project root for full license information.