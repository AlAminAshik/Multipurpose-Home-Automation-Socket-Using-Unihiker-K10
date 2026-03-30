//this is the code to send the IR signal to the AC unit, this code is based on the IRsend example provided by the IRremoteESP8266 library, but it has been modified to work with the ESP32-C3 and to use a larger capture buffer size and a longer timeout value to capture more complex messages. The code also includes some tunable parameters that can be adjusted to improve the accuracy of the decoding process.
//this is not the part of the main code.
//ignore this code

// #include "unihiker_k10.h"

// #include <Arduino.h>
// #include <IRremoteESP8266.h>
// #include <IRsend.h>

// UNIHIKER_K10 k10;

// const uint16_t kIrLed = P0;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
// IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// #define RightTopButton eP6
// volatile bool RightTopButtonState = false;
// #define RightBottomButton eP8
// volatile bool RightBottomButtonState = false;

// uint16_t AC_ONrawData[197]  = {6058, 7340,  608, 1684,  586, 1684,  584, 1682,  586, 1682,  586, 1682,  584, 1684,  586, 1684,  586, 1682,  586, 564,  586, 566,  582, 568,  584, 566,  584, 566,  584, 566,  584, 564,  586, 566,  582, 1684,  584, 1686,  582, 1684,  584, 1684,  582, 1688,  582, 1684,  584, 1686,  582, 1684,  584, 566,  586, 564,  582, 566,  584, 574,  576, 566,  584, 574,  576, 566,  584, 566,  586, 1684,  582, 1692,  578, 1682,  586, 1682,  584, 1684,  584, 1682,  584, 1684,  584, 1684,  586, 564,  584, 566,  586, 564,  586, 566,  584, 568,  584, 564,  584, 566,  586, 564,  582, 1688,  584, 564,  586, 564,  586, 1684,  584, 564,  586, 564,  586, 562,  586, 1682,  586, 564,  586, 1684,  584, 1684,  582, 566,  582, 1686,  588, 1680,  558, 1710,  586, 564,  584, 1686,  582, 1684,  584, 576,  576, 568,  580, 1686,  586, 564,  584, 1684,  582, 1684,  584, 568,  586, 564,  584, 1684,  584, 1684,  586, 562,  586, 1684,  582, 566,  586, 564,  586, 564,  586, 1684,  584, 564,  584, 1692,  574, 568,  586, 1682,  584, 568,  584, 1682,  586, 1684,  584, 566,  586, 1684,  584, 566,  582, 1686,  582, 566,  584, 1684,  584, 564,  586, 7386,  586};
// uint16_t AC_OFFrawData[197] = {6094, 7330,  618, 1654,  620, 1650,  614, 1650,  620, 1654,  616, 1652,  616, 1650,  618, 1650,  618, 1650,  616, 536,  614, 536,  616, 536,  614, 538,  612, 536,  614, 534,  616, 536,  614, 536,  616, 1654,  614, 1650,  616, 1654,  616, 1654,  614, 1652,  616, 1654,  616, 1652,  612, 1658,  612, 534,  616, 534,  616, 538,  586, 562,  612, 538,  612, 536,  612, 540,  610, 536,  614, 1652,  616, 1652,  614, 1656,  612, 1654,  614, 1654,  616, 1652,  616, 1654,  614, 1650,  616, 536,  614, 536,  616, 534,  616, 534,  614, 534,  616, 536,  614, 534,  616, 536,  614, 1656,  614, 1654,  612, 536,  616, 1654,  612, 538,  614, 534,  616, 536,  614, 1654,  616, 536,  612, 536,  616, 1652,  612, 536,  588, 1682,  612, 1656,  612, 1654,  588, 566,  610, 1654,  612, 1658,  612, 540,  612, 538,  612, 1656,  612, 536,  586, 1684,  612, 1652,  612, 538,  616, 538,  610, 1654,  614, 1656,  612, 538,  614, 1654,  614, 536,  614, 536,  612, 536,  614, 1656,  612, 538,  612, 1654,  616, 536,  610, 1660,  612, 538,  612, 1652,  618, 1652,  614, 538,  612, 1656,  612, 536,  616, 1652,  614, 536,  614, 1652,  616, 536,  614, 7354,  618};


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
//         irsend.sendRaw(AC_ONrawData, 197, 38);
//         delay(100);
//         irsend.sendNEC(0x6DEFF85F, 64);
//         Serial.println("AC ON");
//         delay(1000);
//     }
//     else if (RightTopButtonState) {
//         irsend.sendRaw(AC_OFFrawData, 197, 38);
//         delay(100);
//         irsend.sendNEC(0x22AE7A29, 64);
//         Serial.println("AC OFF");
//         delay(1000);
//     }
// }