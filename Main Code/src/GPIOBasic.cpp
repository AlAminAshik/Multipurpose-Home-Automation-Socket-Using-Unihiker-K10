#include "unihiker_k10.h"

UNIHIKER_K10 k10;
uint8_t screen_dir=2;

#define LeftTopButton eP5_KeyA //keyA
#define LeftBottomButton eP4
#define RightTopButton eP6
#define RightBottomButton eP8

void setup() {
    k10.begin();
    Serial.begin(9600);

    pinMode(LeftTopButton, INPUT_PULLDOWN);
    pinMode(LeftBottomButton, INPUT_PULLDOWN);
    pinMode(RightTopButton, INPUT_PULLDOWN);
    pinMode(RightBottomButton, INPUT_PULLDOWN);
}

void loop() {
    Serial.print(digital_read(LeftTopButton));  ///watch carefully this digital read is different!!
    Serial.print(digital_read(LeftBottomButton));
    Serial.print(digital_read(RightTopButton));
    Serial.println(digital_read(RightBottomButton));
    delay(1);

    k10.initScreen(screen_dir);
    k10.creatCanvas();                      //Create a canvas for drawing
    if(digital_read(LeftTopButton) == 1){
        k10.setScreenBackground(0xFFFFFF);      //Set background color to white
        k10.canvas->canvasText("Left Top Pressed", 1, 0x000000);    //Display text in black color
    } else if(digital_read(LeftBottomButton) == 1){
        k10.setScreenBackground(0xADD8E6);      //Set background color to light blue
        k10.canvas->canvasText("Left Bottom Pressed", 1, 0x000000);    //Display text in black color
    } else if(digital_read(RightTopButton) == 1){
        k10.setScreenBackground(0x800000);      //Set background color to Maroon
        k10.canvas->canvasText("Right Top Pressed", 1, 0x000000);    //Display text in black color
    } else if(digital_read(RightBottomButton) == 1){
        k10.setScreenBackground(0x90EE90);      //Set background color to light green
        k10.canvas->canvasText("Right Bottom Pressed", 1, 0x000000);    //Display text in black color
    }
    k10.canvas->updateCanvas();             //Update the canvas to reflect changes
}
