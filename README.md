# STM32_JBD_BMS_Monitor
This project provides Platform.io code for building an STM32F103C8T6 "blue pill" based monitor for a JBD BMS.

- Sends read commands to the JBD BMS and receives all key statistics, including battery voltage, current, remaining capacity, number of cycles, BMS protection status, state of charge, NTC temperatures, individual cell voltages and cell delta voltage.
- Prints these statistics to the Serial1 (PA9 and PA10) output and also displays key metrics on a TFT screen.
- Allows simultaneous operation with the stock Bluetooth dongle over a single UART interface.
- Turns an optional external active cell balancer (e.g. Hankzor) on and off when certain cell voltage thresholds are met (by default it will turn on when any cell is above 3.4v, and off when the highest cell drops below 3.35v).


### Notes
- A generic 3.3v 320x240 ILI9341 TFT screen can be connected through the STM32's SPI interface to display realtime statistics.
- To avoid ground loops, it is essential to use an ADUM1201 serial isolator between the BMS UART interface and the STM32.
- The active-high input for a relay connected across the balancer's "run" pads should be connected to PB1.


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
| VOA  | X | RX (next to negative) |
| VOB  | RX3 (PB11) | X |
| VDD1  | 3.3v | X |
| GND1  | GND | X |
| VDD2  | X | Positive (red) |
| GND2  | X | Negative (black) |

#### Bluetooth Connections
|  Bluetooth Pin  | STM32 Pin  |
| :------------ | :------------ | 
| TXD | TX2 (PA2) | 
| RXD  | RX2 (PA3) | 
| GND  | GND |
| VCC | Battery cell 3 +ve (~10v) |

### Credits:
[Bodmer for TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
