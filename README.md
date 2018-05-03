# ESP12 Flasher for M5Stack-ESP12-Shield

<img src=https://raw.githubusercontent.com/tobozo/M5Stack-ESP12-Shield/master/esp12.jpg>


PIN Mapping:
============

         M5Stack | ESP12
     ------------+------------
        16 (Rx2) | D1 (TX0)
        17 (Tx2) | D3 (RX0)
              12 | D0 (FLASH)
              13 | RESET
            3.3v | VCC
            3.3v | D2
            3.3v | CH_PD (EN)
             GND | GND
             GND | D15

Usage:
======

- Upload this sketch on the M5Stack
- Change boards to ESP8266
- Open your ESP8266 sketch
- Hit the Upload button


Also acts as a serial bridge (115200) after flash timeout.
