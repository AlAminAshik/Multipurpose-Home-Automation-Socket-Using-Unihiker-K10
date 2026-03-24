// #include "unihiker_k10.h"

// #include <Arduino.h>
// #include <IRremoteESP8266.h>
// #include <IRsend.h>

// UNIHIKER_K10 k10;

// const uint16_t kIrLed = 37;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
// IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// #define RightTopButton eP6
// volatile bool RightTopButtonState = false;
// #define RightBottomButton eP8
// volatile bool RightBottomButtonState = false;

// void setup() {
//     k10.begin();
//     Serial.begin(115200);
//     irsend.begin();
//     pinMode(RightTopButton,    INPUT_PULLDOWN);
//     pinMode(RightBottomButton, INPUT_PULLDOWN);
// }

// void loop() {
//     RightBottomButtonState = digital_read(RightBottomButton);
//     RightTopButtonState    = digital_read(RightTopButton);

//     if (RightBottomButtonState) {
//         irsend.sendNEC(0x6DEFF85F, 64);
//         Serial.println("AC ON");
//         delay(1000);
//     }
//     else if (RightTopButtonState) {
//         irsend.sendNEC(0x22AE7A29, 64);
//         Serial.println("AC OFF");
//         delay(1000);
//     }
// }