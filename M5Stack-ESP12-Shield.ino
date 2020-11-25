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

   Using the Arduino IDE:

   - Compile & Upload this sketch on the M5Stack
   - Change board to ESP8266
   - Set Baudrate to "115200"
   - Set Flash Mode to "DOUT"
   - Set Reset Method to "no dtr (aka ck)"
   - Open your ESP8266 sketch
   - Hit the Button B on the M5 so the ESP8266 waits for flashing
   - Hit the Upload button

   Also acts as a serial bridge (115200) after flash timeout.

*/

#include <ESP32-Chimera-Core.h> // available from the Arduino Library Manager or https://github.com/tobozo/ESP32-Chimera-Core
#define tft M5.Lcd
#include <M5StackUpdater.h>     // available from the Arduino Library Manager or https://github.com/tobozo/M5Stack-SD-Updater
#include "esp12.h"
#include "scrollpanel.h"

// For some reason GPIO12 and GPIO13 pins aren't exposed by the battery shield and the
// other available pins wouldn't work as expected (boot HIGH, no pullup or input only).
// I had to get a M5 Proto Board to be able to access those pins properly, but they're
// available directly on the socket http://forum.m5stack.com/topic/188/bottom-io-expander-pinout
#define ESP8266_RESET_PIN 13
#define ESP8266_FLASH_PIN 12
#define FLASH_TIMEOUT 10000 // timeout delay (ms) before acting as a serial bridge

HardwareSerial ESP12Serial(2); // ESP32 UART2 GPIO-16 ( RXD2 )

unsigned long start_time = 0;
bool ignore_timeout = false; // UI logic: wait for timeout before showing UI menu
bool bg_rendered = false;
bool scrollPanelIsActive = false;
bool isflashon = false;


void displayMsg(const char* title= "", const char* msg = "")
{
  tft.fillRect(0, 190, 319, 239, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum( TC_DATUM );
  tft.setFont( &Font4 );
  tft.setTextSize( 1 );
  tft.drawString(title, tft.width() / 2, 194);
  tft.setTextSize( 0.75 );
  tft.drawString(msg, tft.width() / 2, 220);
}

void displayIntro()
{
  tft.fillScreen(BLACK);
  tft.fillRect(0, 0, 319, 184, TFT_WHITE);
  tft.drawJpg( doc_intrologo_jpg, doc_intrologo_jpg_len );
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextDatum( TC_DATUM );
  tft.setFont( &Font4 );
  tft.setTextSize( 1 );
  tft.drawString("M5Stack-ESP12 Flasher", tft.width() / 2, 105);
  tft.setFont( &Font2 );
  tft.drawString(F("Copyright 2018-2020 tobozo"), tft.width() / 2, 145);
}

static uint16_t laststatus = 0;
void displayComStatus( uint16_t color )
{
  if( laststatus == color ) return;
  laststatus = color;
  tft.fillCircle( 10, tft.height()-14, 5, color );
}


void resetOn()
{
  digitalWrite(ESP8266_RESET_PIN, LOW);
  displayMsg("PIN STATE CHANGE", "Reset Pin: ON");
}

void resetOff()
{
  digitalWrite(ESP8266_RESET_PIN, HIGH);
  displayMsg("PIN STATE CHANGE", "Reset Pin: OFF");
}

void flashOn()
{
  digitalWrite(ESP8266_FLASH_PIN, LOW);
  displayMsg("PIN STATE CHANGE", "Flash Pin: ON");
}

void flashOff()
{
  digitalWrite(ESP8266_FLASH_PIN, HIGH);
  displayMsg("PIN STATE CHANGE", "Flash Pin: OFF");
}

void doResetESP()
{
  resetOn();
  delay(300);
  resetOff();
  displayMsg("BRIDGE MODE", "Reset done!");
}

void enableFlashMode()
{
  flashOn();
  resetOn();
  delay(100);
  resetOff();
  delay(100);
  flashOff();
  displayMsg("FLASH MODE", "ESP8266 awaits flashing..");
}



void setup()
{
  Serial.begin(115200);
  ESP12Serial.begin(115200);

  M5.begin();

  scrollSetup();
  scrollPanelIsActive = true;

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
}


void loop()
{
  // ESP12 talking to the M5Stack (or forwarding to Arduino IDE)
  while (ESP12Serial.available()) {
    char c = ESP12Serial.read();
    Serial.print(c);

    if(! scrollPanelIsActive ) {
      //ignore_timeout = true;
      scrollPanelIsActive = true;
      scrollReset();
    }
    if( scrollPanelIsActive ) {
      scrollLinePushChar( c );
    }
    displayComStatus( TFT_GREEN );
    start_time = millis();
  }

  // M5Stack (or Arduino IDE forwarded) talking to the ESP12
  while (Serial.available()) {
    char c = Serial.read();
    ESP12Serial.print(c);
    start_time = millis();
    displayComStatus( TFT_RED );
  }

  displayComStatus( TFT_DARKGREY );

  M5.update();

  if ( M5.BtnA.wasPressed()) {
    ignore_timeout = false;
    start_time = millis();
    doResetESP();
    //scrollSetup();
    scrollPanelIsActive = true;
    scrollReset();
    bg_rendered = false;
  } else if ( M5.BtnB.wasPressed()) {
    enableFlashMode();
    ignore_timeout = true;
  } else if ( M5.BtnC.wasPressed()) {
    if( isflashon ) {
      flashOff();
    } else {
      flashOn();
    }
    isflashon = !isflashon;
    ignore_timeout = true;
  }

  if (!ignore_timeout) {
    if (start_time + FLASH_TIMEOUT < millis()) {
      ignore_timeout = true;
      //setupScrollArea( 0, 0, 0 ); // reset scroll area
      scrollReset();
      scrollPanelIsActive = false;
      flashOff();
      //doResetESP();
      displayMsg("IDLE MODE", "Reset                Flash               Toggle");
      if(!bg_rendered) {
        // drawing the JPG earlier seems to break flashing process
        // M5 bug or heap problem ?
        tft.drawJpg(esp12_jpg, esp12_jpg_len);
        bg_rendered = true;
      }
    }
  }
}
