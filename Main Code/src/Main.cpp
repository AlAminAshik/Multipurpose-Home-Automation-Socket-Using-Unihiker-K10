#include "unihiker_k10.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "asr.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>

//wifi connect
const char* ssid = "Amin Residence GP";
const char* password = "alamiralif86";
WebServer server(80);
unsigned long ota_progress_millis = 0;

UNIHIKER_K10 k10;
uint8_t screen_dir=2;
//#define LCD_BLK eLCD_BLK

ASR asr;
Music music;

const uint16_t kIrLed = 1;  // P0 on the unihiker k10
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

uint16_t AC_ONrawData[197] = {6084, 7334,  614, 1652,  618, 1648,  620, 1648,  622, 1650,  616, 1650,  620, 1650,  620, 1648,  620, 1648,  618, 534,  618, 528,  620, 530,  622, 528,  620, 530,  624, 526,  620, 528,  622, 528,  620, 1648,  622, 1648,  622, 1648,  620, 1650,  618, 1648,  620, 1652,  616, 1652,  618, 1650,  618, 530,  622, 530,  620, 530,  616, 532,  620, 530,  620, 530,  618, 532,  618, 528,  620, 1652,  616, 1648,  620, 1650,  620, 1652,  614, 1650,  620, 1650,  618, 1650,  616, 1648,  620, 532,  616, 530,  620, 534,  618, 528,  620, 530,  616, 534,  618, 530,  618, 530,  618, 1650,  618, 530,  620, 532,  618, 1650,  620, 528,  620, 534,  618, 530,  618, 1648,  620, 530,  622, 1646,  620, 1650,  616, 534,  618, 1646,  620, 1648,  618, 1652,  618, 534,  614, 530,  620, 1648,  620, 1648,  620, 530,  620, 1652,  616, 532,  616, 1652,  616, 1652,  590, 1680,  616, 530,  618, 530,  618, 1648,  594, 558,  592, 1678,  614, 532,  618, 534,  618, 532,  616, 1654,  614, 534,  616, 1654,  614, 534,  618, 1652,  616, 532,  616, 1654,  616, 1648,  620, 530,  622, 1646,  618, 536,  618, 1648,  618, 530,  622, 1652,  614, 530,  622, 7348,  622};
uint16_t AC_OFFrawData[197] = {6076, 7334,  614, 1654,  614, 1656,  614, 1654,  616, 1652,  612, 1656,  614, 1658,  610, 1656,  612, 1656,  612, 538,  614, 536,  612, 538,  614, 534,  614, 536,  614, 538,  614, 536,  614, 536,  614, 1654,  614, 1654,  612, 1654,  612, 1656,  616, 1652,  614, 1654,  614, 1656,  614, 1652,  612, 538,  612, 538,  614, 534,  612, 536,  614, 536,  614, 538,  614, 536,  614, 536,  614, 1656,  612, 1656,  614, 1654,  614, 1654,  612, 1656,  612, 1656,  614, 1654,  612, 1656,  610, 538,  614, 536,  614, 536,  612, 536,  612, 538,  616, 534,  610, 542,  610, 538,  612, 1656,  612, 1656,  614, 536,  614, 1654,  614, 536,  614, 536,  612, 538,  612, 1654,  614, 534,  614, 536,  616, 1652,  614, 536,  614, 1652,  614, 1654,  612, 1658,  612, 536,  614, 536,  614, 1652,  612, 1654,  614, 540,  612, 1652,  614, 542,  584, 1682,  614, 1654,  612, 1652,  616, 538,  610, 538,  612, 1656,  612, 538,  588, 1678,  612, 540,  610, 540,  586, 564,  584, 1682,  612, 540,  610, 1656,  610, 542,  592, 1674,  588, 564,  584, 1682,  584, 1686,  586, 564,  584, 1684,  584, 568,  584, 1684,  584, 568,  582, 1686,  582, 566,  582, 7384,  584};

#define LeftTopButton eP5_KeyA
volatile bool LeftTopButtonState = false;
#define LeftBottomButton eP4
volatile bool LeftBottomButtonState = false;
#define RightTopButton eP6
volatile bool RightTopButtonState = false;
#define RightBottomButton eP8
volatile bool RightBottomButtonState = false;

#define TriacLight eP9
volatile bool TriacLightState = true; // true because light is set to LOW on at startup, so the next toggle should turn it on.
#define TriacFan eP10
volatile bool TriacFanState = true; // true because fan is set to LOW on at startup, so the next toggle should turn it on.


void ButtonTasks(void *pvParameters) {
  while (true) {
    LeftTopButtonState = digital_read(LeftTopButton);
    LeftBottomButtonState = digital_read(LeftBottomButton);
    RightTopButtonState = digital_read(RightTopButton);
    RightBottomButtonState = digital_read(RightBottomButton);
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms polling = 200Hz
  }
}
void VoiceTasks(void *pvParameters) {
  while (true) {
    if (asr.isWakeUp()) {
        music.playMusic(Melodies::BA_DING, OnceInBackground); //sound when wake up
        asr._wakeUp = false; // reset wake up state
    }
    if(asr.isDetectCmdID(0+1)){
        asr._isDetectCmdID = 0; // reset command ID state
        k10.setScreenBackground(0xFFFF00);      //Set background color to yellow
        k10.canvas->canvasText("Voice: Lights toggled", 1, 0x000000);    //Display text in black color
        k10.canvas->updateCanvas();             //Update the canvas to reflect changes
        digital_write(TriacLight, TriacLightState?HIGH:LOW);  // Turn on the light
        delay(10);
        TriacLightState = !TriacLightState;  // Toggle the state for next command
    }
    else if(asr.isDetectCmdID(1+1)){
        asr._isDetectCmdID = 0; // reset command ID state
        k10.setScreenBackground(0xFFFF00);      //Set background color to yellow
        k10.canvas->canvasText("Voice: Fan toggled", 1, 0x000000);    //Display text in black color
        k10.canvas->updateCanvas();             //Update the canvas to reflect changes
        digital_write(TriacFan, TriacFanState?HIGH:LOW);  // Turn on the fan
        delay(10);
        TriacFanState = !TriacFanState;  // Toggle the state for next command
    }
    vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms
  }
}
void OTATasks(void *pvParameters) {
  while (true) {
    server.handleClient();
    ElegantOTA.loop();  // Handle OTA updates
    vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms
  }
}

void setup() {
    k10.begin();
    Serial.begin(115200);
    irsend.begin();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //start OTA server
    server.on("/", []() {
      server.send(200, "text/plain", "Hi! type this to update: http://192.168.0.173/update");
    });
    ElegantOTA.begin(&server);    // Start ElegantOTA
    server.begin();
    Serial.println("HTTP server started");
    
    //setup voice
    asr.asrInit(0, EN_MODE, 5000);  //0: Once, 1: continuous
    while(asr._asrState == 0){
      Serial.println("Waiting for ASR to initialize...");
      delay(500);
    }
    asr.addASRCommand(0+1, "Lights");
    asr.addASRCommand(1+1, "Fan");

    pinMode(LeftTopButton, INPUT_PULLDOWN);
    pinMode(LeftBottomButton, INPUT_PULLDOWN);
    pinMode(RightTopButton, INPUT_PULLDOWN);
    pinMode(RightBottomButton, INPUT_PULLDOWN);
    pinMode(TriacLight, OUTPUT);
    pinMode(TriacFan, OUTPUT);

    digital_write(TriacLight, LOW);  // Ensure the light is off at startup
    digital_write(TriacFan, LOW);    // Ensure the fan is off at startup

    k10.initScreen(screen_dir);
    k10.creatCanvas();                      //Create a canvas for drawing

    xTaskCreatePinnedToCore(ButtonTasks, "ButtonTasks", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(VoiceTasks, "VoiceTasks", 1024 * 6, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(OTATasks, "OTATasks", 1024 * 4, NULL, 10, NULL, 1);
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
        k10.canvas->canvasText("AC Turned ON", 1, 0x000000);    //Display text in black color
        k10.canvas->updateCanvas();             //Update the canvas to reflect changes
        irsend.sendRaw(AC_ONrawData, 197, 38);  // Send a raw data capture at 38kHz.
        Serial.println("AC ON");  
      }
    else if(LeftBottomButtonState){
        LeftBottomButtonState = false;  //Reset the state to avoid repeated triggers
        k10.setScreenBackground(0xADD8E6);      //Set background color to light blue
        k10.canvas->canvasText("AC Turned OFF", 1, 0x000000);    //Display text in black color
        k10.canvas->updateCanvas();             //Update the canvas to reflect changes
        irsend.sendRaw(AC_OFFrawData, 197, 38);  // Send a raw data capture at 38kHz.
        Serial.println("AC OFF");
      } 
    else if(RightTopButtonState){
        RightTopButtonState = false;  //Reset the state to avoid repeated triggers
        k10.setScreenBackground(0x800000);      //Set background color to Maroon
        k10.canvas->canvasText("Light Toggled", 1, 0x000000);    //Display text in black color
        k10.canvas->updateCanvas();             //Update the canvas to reflect changes
        digital_write(TriacLight, TriacLightState?HIGH:LOW);  // Turn on the light
        delay(10);
        TriacLightState = !TriacLightState;  // Toggle the state for next press
      } 
    else if(RightBottomButtonState){
        RightBottomButtonState = false;  //Reset the state to avoid repeated triggers
        k10.setScreenBackground(0x90EE90);      //Set background color to light green
        k10.canvas->canvasText("Fan Toggled", 1, 0x000000);    //Display text in black color
        k10.canvas->updateCanvas();             //Update the canvas to reflect changes
        digital_write(TriacFan, TriacFanState?HIGH:LOW);  // Turn on the fan
        delay(10);
        TriacFanState = !TriacFanState;  // Toggle the state for next press
      }
}
