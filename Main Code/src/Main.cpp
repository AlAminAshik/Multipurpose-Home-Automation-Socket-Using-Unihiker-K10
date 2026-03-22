#include "unihiker_k10.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>

const uint16_t kIrLed = 1;  // P0 on the unihiker k10
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

uint16_t AC_ONrawData[197] = {6084, 7334,  614, 1652,  618, 1648,  620, 1648,  622, 1650,  616, 1650,  620, 1650,  620, 1648,  620, 1648,  618, 534,  618, 528,  620, 530,  622, 528,  620, 530,  624, 526,  620, 528,  622, 528,  620, 1648,  622, 1648,  622, 1648,  620, 1650,  618, 1648,  620, 1652,  616, 1652,  618, 1650,  618, 530,  622, 530,  620, 530,  616, 532,  620, 530,  620, 530,  618, 532,  618, 528,  620, 1652,  616, 1648,  620, 1650,  620, 1652,  614, 1650,  620, 1650,  618, 1650,  616, 1648,  620, 532,  616, 530,  620, 534,  618, 528,  620, 530,  616, 534,  618, 530,  618, 530,  618, 1650,  618, 530,  620, 532,  618, 1650,  620, 528,  620, 534,  618, 530,  618, 1648,  620, 530,  622, 1646,  620, 1650,  616, 534,  618, 1646,  620, 1648,  618, 1652,  618, 534,  614, 530,  620, 1648,  620, 1648,  620, 530,  620, 1652,  616, 532,  616, 1652,  616, 1652,  590, 1680,  616, 530,  618, 530,  618, 1648,  594, 558,  592, 1678,  614, 532,  618, 534,  618, 532,  616, 1654,  614, 534,  616, 1654,  614, 534,  618, 1652,  616, 532,  616, 1654,  616, 1648,  620, 530,  622, 1646,  618, 536,  618, 1648,  618, 530,  622, 1652,  614, 530,  622, 7348,  622};
uint16_t AC_OFFrawData[197] = {6076, 7334,  614, 1654,  614, 1656,  614, 1654,  616, 1652,  612, 1656,  614, 1658,  610, 1656,  612, 1656,  612, 538,  614, 536,  612, 538,  614, 534,  614, 536,  614, 538,  614, 536,  614, 536,  614, 1654,  614, 1654,  612, 1654,  612, 1656,  616, 1652,  614, 1654,  614, 1656,  614, 1652,  612, 538,  612, 538,  614, 534,  612, 536,  614, 536,  614, 538,  614, 536,  614, 536,  614, 1656,  612, 1656,  614, 1654,  614, 1654,  612, 1656,  612, 1656,  614, 1654,  612, 1656,  610, 538,  614, 536,  614, 536,  612, 536,  612, 538,  616, 534,  610, 542,  610, 538,  612, 1656,  612, 1656,  614, 536,  614, 1654,  614, 536,  614, 536,  612, 538,  612, 1654,  614, 534,  614, 536,  616, 1652,  614, 536,  614, 1652,  614, 1654,  612, 1658,  612, 536,  614, 536,  614, 1652,  612, 1654,  614, 540,  612, 1652,  614, 542,  584, 1682,  614, 1654,  612, 1652,  616, 538,  610, 538,  612, 1656,  612, 538,  588, 1678,  612, 540,  610, 540,  586, 564,  584, 1682,  612, 540,  610, 1656,  610, 542,  592, 1674,  588, 564,  584, 1682,  584, 1686,  586, 564,  584, 1684,  584, 568,  584, 1684,  584, 568,  582, 1686,  582, 566,  582, 7384,  584};

UNIHIKER_K10 k10;
uint8_t screen_dir=2;

#define LeftTopButton eP5_KeyA
volatile bool LeftTopButtonState = false;
#define LeftBottomButton eP4
volatile bool LeftBottomButtonState = false;
#define RightTopButton eP6
volatile bool RightTopButtonState = false;
#define RightBottomButton eP8
volatile bool RightBottomButtonState = false;

void ButtonTasks(void *pvParameters) {
  while (true) {
    LeftTopButtonState = digital_read(LeftTopButton);
    LeftBottomButtonState = digital_read(LeftBottomButton);
    RightTopButtonState = digital_read(RightTopButton);
    RightBottomButtonState = digital_read(RightBottomButton);
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms polling = 200Hz
  }
}


void setup() {
    k10.begin();
    Serial.begin(115200);
    irsend.begin();

    pinMode(LeftTopButton, INPUT_PULLDOWN);
    pinMode(LeftBottomButton, INPUT_PULLDOWN);
    pinMode(RightTopButton, INPUT_PULLDOWN);
    pinMode(RightBottomButton, INPUT_PULLDOWN);

    k10.initScreen(screen_dir);
    k10.creatCanvas();                      //Create a canvas for drawing
    xTaskCreatePinnedToCore(ButtonTasks, "ButtonTasks", 2048, NULL, 5, NULL, 0);
}

void loop() {
    Serial.print(digital_read(LeftTopButton));  ///watch carefully this digital read is different!!
    Serial.print(digital_read(LeftBottomButton));
    Serial.print(digital_read(RightTopButton));
    Serial.println(digital_read(RightBottomButton));
    delay(1);

    if(LeftTopButtonState){
        LeftTopButtonState = false;  //Reset the state to avoid repeated triggers
        k10.setScreenBackground(0xFFFFFF);      //Set background color to white
        k10.canvas->canvasText("Left Top Pressed", 1, 0x000000);    //Display text in black color
        k10.canvas->updateCanvas();             //Update the canvas to reflect changes
        irsend.sendRaw(AC_ONrawData, 197, 38);  // Send a raw data capture at 38kHz.
        Serial.println("AC ON");  
      }
    else if(LeftBottomButtonState){
        LeftBottomButtonState = false;  //Reset the state to avoid repeated triggers
        k10.setScreenBackground(0xADD8E6);      //Set background color to light blue
        k10.canvas->canvasText("Left Bottom Pressed", 1, 0x000000);    //Display text in black color
        k10.canvas->updateCanvas();             //Update the canvas to reflect changes
        irsend.sendRaw(AC_OFFrawData, 197, 38);  // Send a raw data capture at 38kHz.
        Serial.println("AC OFF");
      } 
    else if(RightTopButtonState){
        RightTopButtonState = false;  //Reset the state to avoid repeated triggers
        k10.setScreenBackground(0x800000);      //Set background color to Maroon
        k10.canvas->canvasText("Right Top Pressed", 1, 0x000000);    //Display text in black color
        k10.canvas->updateCanvas();             //Update the canvas to reflect changes
      } 
    else if(RightBottomButtonState){
        RightBottomButtonState = false;  //Reset the state to avoid repeated triggers
        k10.setScreenBackground(0x90EE90);      //Set background color to light green
        k10.canvas->canvasText("Right Bottom Pressed", 1, 0x000000);    //Display text in black color
        k10.canvas->updateCanvas();             //Update the canvas to reflect changes
    }
}
