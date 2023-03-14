# STM32_JBD_BMS_Monitor
This project provides Platform.io code for building an STM32F103C8T6 "blue pill" based monitor for a JBD BMS.

- Sends read commands to the JBD BMS and receives all key statistics, including battery voltage, current, remaining capacity, number of cycles, BMS protection status, state of charge, NTC temperatures, individual cell voltages and cell delta voltage.
- Prints these statistics to the Serial1 (PA9 and PA10) output and also displays key metrics on a TFT screen.
- Allows simultaneous operation with the stock Bluetooth dongle over the single UART interface on the BMS.
- Turns an optional external active cell balancer (e.g. Hankzor) on and off when certain cell voltage thresholds are met (by default it will turn on when any cell is above 3.4v, and off when the highest cell drops below 3.35v).


### Notes
- A generic 3.3v 320x240 ILI9341 TFT screen can be connected through the STM32's SPI interface to display realtime statistics.
- To avoid ground loops, it is essential to use an ADUM1201 serial isolator between the BMS UART interface and the STM32.
- The active-high input for a relay connected across the balancer's "run" pads should be connected to PB1.
- The variables maxCurrent, minVoltage and maxVoltage should be adjusted in the code to suit your battery to ensure the ring meters render with a useful scale.
- The STM32 can operate without the TFT display attached. Remove the TFT_ENABLE define to disable all TFT functions.
- The TFT_eSPI library must be configured to suit your display. Instructions for the generic SPI ILI9341 are included inline with the code.
- If you need more advanced functinality including the ability to send statistics to InfluxDB over WiFi, have a look at the sister project for the ESP8266 here: https://github.com/octal-ip/ESP8266_JBD_BMS_Monitor


### Bill of materials
- STM32F103 "blue pill" board (or clone)
- ST-Link v2 programmer (only if you don't have one already)
- ADUM1201 dual channel digital isolator
- 2 x linear voltage regulator modules, one 3.3v and one 5v.
- 5v relay module
- ILI9341 SPI TFT screen (optional, but very useful - must be the 3.3v version)


### The Bluetooth module, Xiaoxiang app and STM32 TFT operating simultaneously.
![BT and STM32 TFT](https://raw.githubusercontent.com/octal-ip/STM32_JBD_BMS_Monitor/main/pics/STM32_JBD.jpg "BT and STM32 TFT")


### Wiring
#### TFT Display Connections
| STM32 Pin  | ILI9341 TFT Pin  |
| :------------ | :------------ |
| PB13  | CS  |
| PB14  | DC  |
| PB15  | RST  |
| PA7  | MOSI  |
| PA6  | MISO  |
| PA5  | SCK  |
| +3.3v  | LED  |

#### UART Connections
| ADUM1201 Pin  | STM32 Pin  | JBD BMS Pin |
| :------------ | :------------ | :------------ |
| VIA  | X | TX (next to positive) |
| VIB  | TX3 (PB10) | X |
| VOA  | RX3 (PB11) | X |
| VOB  | X | RX (next to negative) |
| VDD1  | 3.3v | X |
| GND1  | GND | X |
| VDD2  | X | Positive via 3.3v regulator (red) |
| GND2  | X | Negative (black) |

#### Bluetooth Connections
|  Bluetooth Pin  | STM32 Pin  |
| :------------ | :------------ | 
| TXD | TX2 (PA2) | 
| RXD  | RX2 (PA3) | 
| GND  | GND |
| VCC | Battery cell 3 +ve (~10v) |

![Wiring layout](https://raw.githubusercontent.com/octal-ip/STM32_JBD_BMS_Monitor/main/pics/STM32_JBD_Diagram.png "Wiring layout")

### Credits:
[Bodmer for TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
