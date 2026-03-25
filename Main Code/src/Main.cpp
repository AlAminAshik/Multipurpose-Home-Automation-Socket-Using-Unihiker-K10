#include "unihiker_k10.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "asr.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <time.h>
#define sensor_t adafruit_sensor_t
#include <Adafruit_AHTX0.h>
#undef sensor_t
#include <alarmLogo.h>

Adafruit_AHTX0 aht;
WiFiManager wm;

#define WIFI_SSID       "Amin Residence GP"
#define WIFI_PASS       "alamiralif86"
#define WIFI_SSID_DEBUG "Smart Socket by Alamin"
#define WIFI_PASS_DEBUG "87654321"

UNIHIKER_K10 k10;
uint8_t screen_dir = 2;
uint16_t lightIntensity;

ASR   asr;
Music music;

const uint16_t kIrLed = 1;
IRsend irsend(kIrLed);

uint16_t AC_ONrawData[197]  = {6084,7334,614,1652,618,1648,620,1648,622,1650,616,1650,620,1650,620,1648,620,1648,618,534,618,528,620,530,622,528,620,530,624,526,620,528,622,528,620,1648,622,1648,622,1648,620,1650,618,1648,620,1652,616,1652,618,1650,618,530,622,530,620,530,616,532,620,530,620,530,618,532,618,528,620,1652,616,1648,620,1650,620,1652,614,1650,620,1650,618,1650,616,1648,620,532,616,530,620,534,618,528,620,530,616,534,618,530,618,530,618,1650,618,530,620,532,618,1650,620,528,620,534,618,530,618,1648,620,530,622,1646,620,1650,616,534,618,1646,620,1648,618,1652,618,534,614,530,620,1648,620,1648,620,530,620,1652,616,532,616,1652,616,1652,590,1680,616,530,618,530,618,1648,594,558,592,1678,614,532,618,534,618,532,616,1654,614,534,616,1654,614,534,618,1652,616,532,616,1654,616,1648,620,530,622,1646,618,536,618,1648,618,530,622,1652,614,530,622,7348,622};
uint16_t AC_OFFrawData[197] = {6076,7334,614,1654,614,1656,614,1654,616,1652,612,1656,614,1658,610,1656,612,1656,612,538,614,536,612,538,614,534,614,536,614,538,614,536,614,536,614,1654,614,1654,612,1654,612,1656,616,1652,614,1654,614,1656,614,1652,612,538,612,538,614,534,612,536,614,536,614,538,614,536,614,536,614,1656,612,1656,614,1654,614,1654,612,1656,612,1656,614,1654,612,1656,610,538,614,536,614,536,612,536,612,538,616,534,610,542,610,538,612,1656,612,1656,614,536,614,1654,614,536,614,536,612,538,612,1654,614,534,614,536,616,1652,614,536,614,1652,614,1654,612,1658,612,536,614,536,614,1652,612,1654,614,540,612,1652,614,542,584,1682,614,1654,612,1652,616,538,610,538,612,1656,612,538,588,1678,612,540,610,540,586,564,584,1682,612,540,610,1656,610,542,592,1674,588,564,584,1682,584,1686,586,564,584,1684,584,568,584,1684,584,568,582,1686,582,566,582,7384,584};

// ── Button & output pins ──────────────────────────────────────────────────────
#define LeftTopButton     eP5_KeyA
#define LeftBottomButton  eP4
#define RightTopButton    eP6
#define RightBottomButton eP8
#define TriacLight        eP9
#define TriacFan          eP10

// ── Page enum ─────────────────────────────────────────────────────────────────
enum Page { PAGE_MAIN, PAGE_ALARM };
static volatile Page currentPage = PAGE_MAIN;

// ── Alarm state ───────────────────────────────────────────────────────────────
static volatile int  alarmHour    = 6;
static volatile int  alarmMinute  = 0;
static volatile bool alarmEnabled = false;
static volatile bool alarmFired   = false;
static volatile bool alarmRunning = false;

// ── Shared appliance state ────────────────────────────────────────────────────
static volatile bool TriacLightState = false;
static volatile bool TriacFanState   = false;
static volatile bool ACState         = false;

// ── Semaphores / mutexes ──────────────────────────────────────────────────────
static SemaphoreHandle_t stateMutex = NULL;  // protects appliance states + page
static SemaphoreHandle_t irMutex    = NULL;  // protects irsend
static SemaphoreHandle_t AlarmMutex = NULL;  // protects alarm state

// ── Page-change tracking (UI task only, no mutex needed) ─────────────────────
static Page lastDrawnPage = PAGE_MAIN;
static bool backgroundNeedsRedraw = true;   // force draw on first frame

// ─────────────────────────────────────────────────────────────────────────────
void toggleLight() {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    TriacLightState = !TriacLightState;
    digital_write(TriacLight, TriacLightState ? HIGH : LOW);
    xSemaphoreGive(stateMutex);
}

void toggleFan() {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    TriacFanState = !TriacFanState;
    digital_write(TriacFan, TriacFanState ? HIGH : LOW);
    xSemaphoreGive(stateMutex);
}

void toggleAC() {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    bool newState = !ACState;
    xSemaphoreGive(stateMutex);

    xSemaphoreTake(irMutex, portMAX_DELAY);
    if (newState) {
        irsend.sendRaw(AC_ONrawData,  197, 38);
        Serial.println("IR → AC ON");
    } else {
        irsend.sendRaw(AC_OFFrawData, 197, 38);
        Serial.println("IR → AC OFF");
    }
    xSemaphoreGive(irMutex);

    xSemaphoreTake(stateMutex, portMAX_DELAY);
    ACState = newState;
    xSemaphoreGive(stateMutex);
}

// ─────────────────────────────────────────────────────────────────────────────
void Configure_OTA_WIFI() {
    wm.resetSettings();
    wm.setConfigPortalTimeout(300);
    wm.setShowInfoErase(false);
    wm.setClass("invert");
    bool res = wm.autoConnect(WIFI_SSID_DEBUG, WIFI_PASS_DEBUG);
    if (!res) Serial.println("WiFi portal: failed to connect");
    else      Serial.println("New WiFi saved successfully");
    wm.stopConfigPortal();
}

// TASK: ButtonTasks  (core 0, priority 5)
// ─────────────────────────────────────────────────────────────────────────────
void ButtonTasks(void *pvParameters) {
    bool prevLeftTop     = false;
    bool prevLeftBottom  = false;
    bool prevRightTop    = false;
    bool prevRightBottom = false;

    bool     lbHeld      = false;
    uint32_t lbPressTime = 0;
    bool     lbLongFired = false;

    while (true) {
        bool curLeftTop     = digital_read(LeftTopButton);
        bool curLeftBottom  = digital_read(LeftBottomButton);
        bool curRightTop    = digital_read(RightTopButton);
        bool curRightBottom = digital_read(RightBottomButton);

        bool pressedLeftTop     = curLeftTop     && !prevLeftTop;
        bool pressedLeftBottom  = curLeftBottom  && !prevLeftBottom;
        bool pressedRightTop    = curRightTop    && !prevRightTop;
        bool pressedRightBottom = curRightBottom && !prevRightBottom;
        bool releasedLeftBottom = !curLeftBottom &&  prevLeftBottom;

        Page page;
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        page = currentPage;
        xSemaphoreGive(stateMutex);

        // ── LeftTopButton ─────────────────────────────────────────────────
        if (pressedLeftTop) {
            if (page == PAGE_MAIN) {
                toggleAC();
            } else {
                xSemaphoreTake(AlarmMutex, portMAX_DELAY);
                alarmEnabled = !alarmEnabled;
                if (alarmEnabled) alarmFired = false;  // re-arm when enabling
                xSemaphoreGive(AlarmMutex);
                backgroundNeedsRedraw = true;
            }
        }

        // ── LeftBottomButton ──────────────────────────────────────────────
        if (pressedLeftBottom) {
            lbHeld      = true;
            lbPressTime = millis();
            lbLongFired = false;
        }
        if (lbHeld && curLeftBottom) {
            if (!lbLongFired && (millis() - lbPressTime >= 5000)) {
                lbLongFired = true;
                Serial.println("Entering OTA Mode");
                // Configure_OTA_WIFI();
            }
        }
        if (releasedLeftBottom) {
            // Dismiss alarm if running (check before page-toggle)
            xSemaphoreTake(AlarmMutex, portMAX_DELAY);
            bool running = alarmRunning;
            if (running) {
                alarmRunning = false;
                alarmFired   = true;   // prevent re-fire this minute
            }
            xSemaphoreGive(AlarmMutex);

            if (!lbLongFired && !running) {
                // Short press, alarm was not running → switch page
                xSemaphoreTake(stateMutex, portMAX_DELAY);
                currentPage = (currentPage == PAGE_MAIN) ? PAGE_ALARM : PAGE_MAIN;
                xSemaphoreGive(stateMutex);
                backgroundNeedsRedraw = true;
                Serial.println(currentPage == PAGE_MAIN ? "Page: MAIN" : "Page: ALARM");
            }
            lbHeld = false;
        }

        // ── RightTopButton ────────────────────────────────────────────────
        if (pressedRightTop) {
            if (page == PAGE_MAIN) {
                toggleLight();
            } else {
                xSemaphoreTake(AlarmMutex, portMAX_DELAY);
                alarmHour = (alarmHour + 1) % 24;
                alarmFired = false;   // time changed, re-arm
                xSemaphoreGive(AlarmMutex);
                backgroundNeedsRedraw = true;
            }
        }

        // ── RightBottomButton ─────────────────────────────────────────────
        if (pressedRightBottom) {
            if (page == PAGE_MAIN) {
                toggleFan();
            } else {
                xSemaphoreTake(AlarmMutex, portMAX_DELAY);
                alarmMinute = (alarmMinute + 2) % 60;
                alarmFired  = false;   // time changed, re-arm
                xSemaphoreGive(AlarmMutex);
                backgroundNeedsRedraw = true;
            }
        }

        prevLeftTop     = curLeftTop;
        prevLeftBottom  = curLeftBottom;
        prevRightTop    = curRightTop;
        prevRightBottom = curRightBottom;

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// TASK: VoiceTasks  (core 1, priority 5)
// ─────────────────────────────────────────────────────────────────────────────
void VoiceTasks(void *pvParameters) {
    while (true) {
        if (asr.isWakeUp()) {
            music.playMusic(Melodies::BA_DING, OnceInBackground);
            asr._wakeUp = false;
        }
        if (asr.isDetectCmdID(0+1)) {
            asr._isDetectCmdID = 0;
            toggleLight();
        } else if (asr.isDetectCmdID(1+1)) {
            asr._isDetectCmdID = 0;
            toggleFan();
        } else if (asr.isDetectCmdID(2+1)) {
            asr._isDetectCmdID = 0;
            toggleAC();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// TASK: UITasks  (core 0, priority 3)
// ─────────────────────────────────────────────────────────────────────────────
void UITasks(void *pvParameters) {
    char timeStringBuff[20];
    struct tm timeinfo;

    while (true) {
        // ── Snapshot appliance + page state ──────────────────────────────
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        bool lightOn = TriacLightState;
        bool fanOn   = TriacFanState;
        bool acOn    = ACState;
        Page page    = currentPage;
        xSemaphoreGive(stateMutex);

        // ── Snapshot alarm state ──────────────────────────────────────────
        xSemaphoreTake(AlarmMutex, portMAX_DELAY);
        int  aHour    = alarmHour;
        int  aMin     = alarmMinute;
        bool aEnabled = alarmEnabled;
        bool aFired   = alarmFired;
        bool aRunning = alarmRunning;
        xSemaphoreGive(AlarmMutex);

        // ── Get current time ──────────────────────────────────────────────
        bool timeOk = (WiFi.status() == WL_CONNECTED) && getLocalTime(&timeinfo);

        // ── Check alarm ───────────────────────────────────────────────────
        if (aEnabled && timeOk && !aFired && !aRunning) {
            if (timeinfo.tm_hour == aHour && timeinfo.tm_min == aMin) {
                xSemaphoreTake(AlarmMutex, portMAX_DELAY);
                alarmRunning = true;
                xSemaphoreGive(AlarmMutex);
                aRunning = true;
                music.playMusic(Melodies::BA_DING, OnceInBackground);
            }
        }
        // Reset alarmFired when minute advances past the alarm minute
        if (aFired && timeOk) {
            if (timeinfo.tm_hour != aHour || timeinfo.tm_min != aMin) {
                xSemaphoreTake(AlarmMutex, portMAX_DELAY);
                alarmFired = false;
                xSemaphoreGive(AlarmMutex);
            }
        }

        // ── Redraw background only on page change ─────────────────────────
        if (backgroundNeedsRedraw || page != lastDrawnPage) {
            if (page == PAGE_MAIN)  k10.setScreenBackground(0x2C4C9C);
            else                    k10.setScreenBackground(0x576490);
            lastDrawnPage        = page;
            backgroundNeedsRedraw = false;
        }

        // ── Top bar ───────────────────────────────────────────────────────
        if (WiFi.status() == WL_CONNECTED) {
            if (timeOk) {
                strftime(timeStringBuff, sizeof(timeStringBuff), "%I:%M %p", &timeinfo);
                k10.canvas->canvasText(timeStringBuff, 10, 5, 0xFFFFFF, k10.canvas->eCNAndENFont24, 8, true);
            } else {
                k10.canvas->canvasText("!Time!", 1, 0xFF0000);
            }
            k10.canvas->canvasCircle(212, 15, 10, 0x00FF00, 0x00FF00, true);
        } else {
            k10.canvas->canvasCircle(212, 15, 10, 0xFF0000, 0xFF0000, true);
            k10.canvas->canvasText("!WiFi!", 1, 0xFF0000);
        }
        k10.canvas->canvasLine(10, 35, 230, 35, 0xFFFFFF);

        // ═════════════════════════════════════════════════════════════════
        // DRAW: MAIN PAGE
        // ═════════════════════════════════════════════════════════════════
        if (page == PAGE_MAIN) {
            k10.canvas->canvasText(lightOn ? "Light (ON)"  : "Light (OFF)", 3, 0xFFFFFF);
            k10.canvas->canvasText(fanOn   ? "Fan   (ON)"  : "Fan   (OFF)", 4, 0xFFFFFF);
            k10.canvas->canvasText(acOn    ? "AC    (ON)"  : "AC    (OFF)", 5, 0xFFFFFF);

            lightIntensity = k10.readALS();
            k10.canvas->canvasText(("LightLux: " + String(lightIntensity)).c_str(), 6, 0xFFFFFF);

            sensors_event_t humidity_event, temp_event;
            aht.getEvent(&humidity_event, &temp_event);
            k10.canvas->canvasText(("Hum: "  + String(humidity_event.relative_humidity, 1) + " %").c_str(), 7, 0xFFFFFF);
            k10.canvas->canvasText(("Temp: " + String(temp_event.temperature, 1)           + " C").c_str(), 8, 0xFFFFFF);

            char alarmHint[24];
            snprintf(alarmHint, sizeof(alarmHint), "Alarm %02d:%02d %s",
                     aHour, aMin, aEnabled ? "[ON]" : "[OFF]");
            k10.canvas->canvasText(alarmHint, 9, aEnabled ? 0x00FF88 : 0x888888);
        }

        // ═════════════════════════════════════════════════════════════════
        // DRAW: ALARM PAGE
        // ═════════════════════════════════════════════════════════════════
        else {
            k10.canvas->canvasText("-- ALARM --", 50, 40, 0xFFFFFF, k10.canvas->eCNAndENFont24, 11, true);
            const uint8_t* alarmBitmap = reinterpret_cast<const uint8_t*>(epd_bitmap_alarm_logo);
            k10.canvas->canvasDrawBitmap(65, 70, 140, 140, alarmBitmap);

            char alarmTimeBuff[10];
            snprintf(alarmTimeBuff, sizeof(alarmTimeBuff), "%02d:%02d", aHour, aMin);
            k10.canvas->canvasText(alarmTimeBuff, 88, 240, aEnabled ? 0x00FF88 : 0xFF4444,
                                   k10.canvas->eCNAndENFont24, 8, true);
            k10.canvas->canvasText(aEnabled ? "ON" : "OFF",
                                   88, 275, aEnabled ? 0x00FF88 : 0xFF4444,
                                   k10.canvas->eCNAndENFont24, 8, true);
            k10.canvas->canvasLine(10, 227, 230, 227, 0x444466);
            k10.canvas->canvasText("[<] Back",   10,  295, 0xFFFFFF, k10.canvas->eCNAndENFont16, 8,  false);
            k10.canvas->canvasText("Hr [^]",    185,  235, 0xFFFFFF, k10.canvas->eCNAndENFont16, 6,  false);
            k10.canvas->canvasText("Min [^]",   185,  295, 0xFFFFFF, k10.canvas->eCNAndENFont16, 7,  false);
            k10.canvas->canvasText("[A] Toggle",  10, 235, 0xFFFFFF, k10.canvas->eCNAndENFont16, 10, false);
        }

        k10.canvas->updateCanvas();
        vTaskDelay(pdMS_TO_TICKS(100));   // 10 fps is plenty for a status display
    }
}

// SETUP
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
    k10.begin();
    Serial.begin(115200);
    irsend.begin();

    // ── Create ALL mutexes before any task is spawned ─────────────────────
    stateMutex = xSemaphoreCreateMutex();
    irMutex    = xSemaphoreCreateMutex();
    AlarmMutex = xSemaphoreCreateMutex();   // ← was missing, caused the crash

    // ── WiFi ──────────────────────────────────────────────────────────────
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    for (int n = 20; n > 0 && WiFi.status() != WL_CONNECTED; n--) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print('.');
    }
    configTime(6 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    // ── ASR ───────────────────────────────────────────────────────────────
    asr.asrInit(0, EN_MODE, 5000);
    while (asr._asrState == 0) {
        Serial.print('.');
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    asr.addASRCommand(0+1, const_cast<char*>("lights"));
    asr.addASRCommand(1+1, const_cast<char*>("fan"));
    asr.addASRCommand(2+1, const_cast<char*>("ac"));

    // ── GPIO ──────────────────────────────────────────────────────────────
    pinMode(LeftTopButton,    INPUT_PULLDOWN);
    pinMode(LeftBottomButton, INPUT_PULLDOWN);
    pinMode(RightTopButton,   INPUT_PULLDOWN);
    pinMode(RightBottomButton,INPUT_PULLDOWN);
    pinMode(TriacLight, OUTPUT);
    pinMode(TriacFan,   OUTPUT);
    digital_write(TriacLight, LOW);
    digital_write(TriacFan,   LOW);

    // ── Display ───────────────────────────────────────────────────────────
    k10.initScreen(screen_dir);
    k10.creatCanvas();
    k10.canvas->canvasSetLineWidth(1);
    k10.setScreenBackground(0x2C4C9C);   // draw background once at startup

    // ── AHT sensor ───────────────────────────────────────────────────────
    if (!aht.begin()) Serial.println("AHT10/AHT20 not found");

    // ── Tasks ─────────────────────────────────────────────────────────────
    Serial.print("Free heap before ButtonTasks: "); Serial.println(ESP.getFreeHeap());
    xTaskCreatePinnedToCore(ButtonTasks, "ButtonTasks", 1024*6, NULL, 5, NULL, 0);
    Serial.print("Free heap before VoiceTasks:  "); Serial.println(ESP.getFreeHeap());
    xTaskCreatePinnedToCore(VoiceTasks,  "VoiceTasks",  1024*3, NULL, 5, NULL, 1);
    Serial.print("Free heap before UITasks:     "); Serial.println(ESP.getFreeHeap());
    xTaskCreatePinnedToCore(UITasks,     "UITasks",     1024*8, NULL, 3, NULL, 0);
}

// LOOP
// ═════════════════════════════════════════════════════════════════════════════
void loop() {
    vTaskDelay(pdMS_TO_TICKS(200));
}