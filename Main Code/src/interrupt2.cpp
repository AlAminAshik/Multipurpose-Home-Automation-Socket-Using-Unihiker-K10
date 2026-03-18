// #include "unihiker_k10.h"

// UNIHIKER_K10 k10;
// volatile bool ep4Pressed = false;
// bool lastState = HIGH;

// void ep4Task(void *pvParameters) {
//   while (true) {
//     bool state = digital_read(eP4);
//     if (state == LOW && lastState == HIGH) {
//       ep4Pressed = true;
//     }
//     lastState = state;
//     vTaskDelay(pdMS_TO_TICKS(5));  // 5ms polling = 200Hz
//   }
// }

// void setup() {
//   Serial.begin(115200);
//   k10.begin();
//   xTaskCreatePinnedToCore(ep4Task, "ep4Task", 2048, NULL, 5, NULL, 0);
// }

// void loop() {
//   if (ep4Pressed) {
//     ep4Pressed = false;
//     Serial.println("eP4 pressed!");
//   }
// }