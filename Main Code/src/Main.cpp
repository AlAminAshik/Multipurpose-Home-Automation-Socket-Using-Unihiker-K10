#include "unihiker_k10.h"

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

void LeftTopButtonTask(void *pvParameters) {
  while (true) {
    LeftTopButtonState = digital_read(LeftTopButton);
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms polling = 200Hz
  }
}
void LeftBottomButtonTask(void *pvParameters) {
  while (true) {
    LeftBottomButtonState = digital_read(LeftBottomButton);
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms polling = 200Hz
  }
}
void RightTopButtonTask(void *pvParameters) {
  while (true) {
    RightTopButtonState = digital_read(RightTopButton);
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms polling = 200Hz
  }
}
void RightBottomButtonTask(void *pvParameters) {
  while (true) {
    RightBottomButtonState = digital_read(RightBottomButton);
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms polling = 200Hz
  }
}


void setup() {
    k10.begin();
    Serial.begin(115200);

    pinMode(LeftTopButton, INPUT_PULLDOWN);
    pinMode(LeftBottomButton, INPUT_PULLDOWN);
    pinMode(RightTopButton, INPUT_PULLDOWN);
    pinMode(RightBottomButton, INPUT_PULLDOWN);

    k10.initScreen(screen_dir);
    k10.creatCanvas();                      //Create a canvas for drawing
    xTaskCreatePinnedToCore(LeftTopButtonTask, "LeftTopButtonTask", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(LeftBottomButtonTask, "LeftBottomButtonTask", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(RightTopButtonTask, "RightTopButtonTask", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(RightBottomButtonTask, "RightBottomButtonTask", 2048, NULL, 5, NULL, 0);

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
    }
    else if(LeftBottomButtonState){
        LeftBottomButtonState = false;  //Reset the state to avoid repeated triggers
        k10.setScreenBackground(0xADD8E6);      //Set background color to light blue
        k10.canvas->canvasText("Left Bottom Pressed", 1, 0x000000);    //Display text in black color
    } else if(RightTopButtonState){
        RightTopButtonState = false;  //Reset the state to avoid repeated triggers
        k10.setScreenBackground(0x800000);      //Set background color to Maroon
        k10.canvas->canvasText("Right Top Pressed", 1, 0x000000);    //Display text in black color
    } else if(RightBottomButtonState){
        RightBottomButtonState = false;  //Reset the state to avoid repeated triggers
        k10.setScreenBackground(0x90EE90);      //Set background color to light green
        k10.canvas->canvasText("Right Bottom Pressed", 1, 0x000000);    //Display text in black color
    }
    k10.canvas->updateCanvas();             //Update the canvas to reflect changes
}
