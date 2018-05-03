/*
   ESP12 Flasher for M5Stack-ESP12-Shield
   Project Page: https://github.com/tobozo/M5Stack-ESP12-Shield

   Copyright 2018 tobozo http://github.com/tobozo

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files ("M5Stack ESP12 Shield"), to deal in the Software without
   restriction, including without limitation the rights to use,
   copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following
   conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.



   PIN Mapping:

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

   - Upload this sketch on the M5Stack
   - Change boards to ESP8266
   - Open your ESP8266 sketch
   - Hit the Upload button

   Also acts as a serial bridge (115200) after flash timeout.

*/

#include <M5Stack.h>
#include <M5StackUpdater.h>
#include "esp12.h"

// For some reason GPIO12 and GPIO13 pins aren't exposed by the battery shield and the
// other available pins wouldn't work as expected (boot HIGH, no pullup or input only).
// I had to get a M5 Proto Board to be able to access those pins properly, but they're
// available directly on the socket http://forum.m5stack.com/topic/188/bottom-io-expander-pinout
#define ESP8266_RESET_PIN 13
#define ESP8266_FLASH_PIN 12
#define FLASH_TIMEOUT 5000 // timeout delay (ms) before acting as a serial bridge
// comment/uncomment this to toggle debug
//#define DEBUG

HardwareSerial ESP12Serial(2); // ESP32 UART2 GPIO-16 ( RXD2 )

unsigned long start_time = 0;
bool wait_done = false;
bool bg_rendered = false;


void displayMsg(String msg = "") {
  M5.Lcd.fillRect(0, 185, 319, 239, TFT_BLACK);
  M5.Lcd.drawCentreString(F("M5Stack ESP12 Flasher"), M5.Lcd.width() / 2, 188, 4);
  M5.Lcd.drawCentreString(msg, M5.Lcd.width() / 2, 214, 4);
}

void displayIntro() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.fillRect(0, 0, 319, 184, TFT_WHITE);
  M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Lcd.drawCentreString(F("BRIDGE/FLASH MODE"), M5.Lcd.width() / 2, 85, 4);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
}

void resetOn() {
  digitalWrite(ESP8266_RESET_PIN, LOW);
}

void resetOff() {
  digitalWrite(ESP8266_RESET_PIN, HIGH);
}

void flashOn() {
  #ifdef DEBUG
    Serial.println("\nWill turn flash off\n");
  #endif
  digitalWrite(ESP8266_FLASH_PIN, LOW);
  displayMsg("Flash Mode: ON");
}

void flashOff() {
  #ifdef DEBUG
    Serial.println("\nWill turn flash off\n");
  #endif
  digitalWrite(ESP8266_FLASH_PIN, HIGH);
  displayMsg("Flash Mode: OFF");
}

void doResetESP() {
  #ifdef DEBUG
    Serial.println("\nWill reset\n");
  #endif
  resetOn();
  delay(300);
  resetOff();
}

void enableFlashMode() {
  flashOn();
  resetOn();
  delay(100);
  resetOff();
  delay(100);
  flashOff();
}



void setup() {
  Serial.begin(115200);
  ESP12Serial.begin(115200);

  M5.begin();
  displayIntro();

  pinMode(BUTTON_A_PIN, INPUT_PULLUP); // Set A button
  pinMode(BUTTON_B_PIN, INPUT_PULLUP); // Set B button
  pinMode(BUTTON_C_PIN, INPUT_PULLUP); // Set C button
  pinMode(ESP8266_RESET_PIN, OUTPUT);  // Set Reset Pin on the ESP12
  pinMode(ESP8266_FLASH_PIN, OUTPUT);  // Set Flash Pin on the ESP12

  if (digitalRead(BUTTON_A_PIN) == 0) {
    updateFromFS(SD);
    ESP.restart();
  }

  start_time = millis();
  enableFlashMode();
  displayMsg("Flash Ready!");
}


void loop() {
  // ESP12 talking to the M5Stack
  while (ESP12Serial.available()) {
    char c = ESP12Serial.read();
    Serial.print(c);
  }
  // M5Stack talking to the ESP12
  while (Serial.available()) {
    char c = Serial.read();
    ESP12Serial.print(c);
    start_time = millis();
  }

  M5.update();

  if ( M5.BtnA.wasPressed()) {
    wait_done = false;
    start_time = millis();
    doResetESP();
    displayMsg("Flash Ready!");
  } else if ( M5.BtnB.wasPressed()) {
    flashOn();
  } else if ( M5.BtnC.wasPressed()) {
    flashOff();
  }

  if (!wait_done) {
    if (start_time + FLASH_TIMEOUT < millis()) {
      #ifdef DEBUG
        Serial.println("\nFlash timeout\n");
      #endif
      wait_done = true;
      flashOff();
      doResetESP();
      displayMsg("Idle");
      if(!bg_rendered) {
        // drawing the JPG earlier seems to break flashing process
        // M5 bug or heap problem ?
        M5.Lcd.drawJpg(esp12_jpg, 13721);
        bg_rendered = true;
      }
    }
  }
}
