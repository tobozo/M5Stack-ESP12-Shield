# ESP12 Flasher for M5Stack-ESP12-Shield

<img src=https://raw.githubusercontent.com/tobozo/M5Stack-ESP12-Shield/master/doc/ESP12-Shield.jpg>

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
             
<img src=https://raw.githubusercontent.com/tobozo/M5Stack-ESP12-Shield/master/doc/esp12.jpg>

Using the Arduino IDE:
======================

   - Compile & Upload this sketch on the M5Stack
   - Change board to ESP8266
   - Set Baudrate to "115200"
   - Set Flash Mode to "DOUT"
   - Set Reset Method to "no dtr (aka ck)"
   - Open your ESP8266 sketch
   - Hit the Button B on the M5 so the ESP8266 waits for flashing
   - Hit the Upload button


Also acts as a serial bridge (115200) after flash timeout.

ESP8266 Flash settings
======================

![image](https://user-images.githubusercontent.com/1893754/100287790-9efd1f80-2f75-11eb-856f-7b2350530cc3.png)
