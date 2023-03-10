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


### TFT Statistics
![BMS_TFT](https://raw.githubusercontent.com/octal-ip/ESP8266_JBD_BMS_Monitor/main/pics/BMS_TFT.jpg "BMS TFT")

### The Bluetooth module, Xiaoxiang app and STM32 operating simultaneously.
![BT STM32 and ESP8266](https://raw.githubusercontent.com/octal-ip/ESP8266_JBD_BMS_Monitor/main/pics/STM32_BT_ESP8266.jpg "BT STM32 and ESP8266")


### Credits:
[Bodmer for TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
