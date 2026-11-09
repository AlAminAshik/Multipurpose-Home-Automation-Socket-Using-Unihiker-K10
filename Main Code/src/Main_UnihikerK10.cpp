#include "unihiker_k10.h"
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
#include "RedOffSwitchIcon.h"
#include "GreenOnSwitchIcon.h"
#include "alarmLogo.h"
#include "noWifiIcon.h"
#include "wifiIcon.h"
#include "lightIcon.h"
#include "fanIcon.h"
#include "ACIcon.h"
#include <soc/gpio_struct.h> // Native ESP32-S3 GPIO register access for fan ISR

Adafruit_AHTX0 aht;
WiFiManager wm;

#define WIFI_SSID       "Amin Residence"
#define WIFI_PASS       "alamiralif86"
#define WIFI_SSID_DEBUG "Smart Socket by Alamin"
#define WIFI_PASS_DEBUG "87654321"

// Backlight and display ─────────────────────────────────────────────────────────────────────────────
UNIHIKER_K10 k10;
uint8_t screen_dir = 2;
uint16_t lightIntensity;
#define BackLED P1  //this pin was soldered with the backled transistor of the display since eLCD_BLK pin interferes with the voice
int minLightIntensity = 2; // less than 2 lux the screen with turn off
int maxLightIntensity = 15; // more than 15 lux the screen will be at full brightness

ASR   asr;
Music music;
Music alarmMusic; // Dedicated player so voice feedback cannot overwrite the alarm melody

// IR Data ─────────────────────────────────────────────────────────────────────────────
const uint16_t kIrLed = P0; // this pin was soldered with the IR LED on the back of the board
IRsend irsend(kIrLed);
uint16_t AC_ONrawData[197]  = {6058, 7340,  608, 1684,  586, 1684,  584, 1682,  586, 1682,  586, 1682,  584, 1684,  586, 1684,  586, 1682,  586, 564,  586, 566,  582, 568,  584, 566,  584, 566,  584, 566,  584, 564,  586, 566,  582, 1684,  584, 1686,  582, 1684,  584, 1684,  582, 1688,  582, 1684,  584, 1686,  582, 1684,  584, 566,  586, 564,  582, 566,  584, 574,  576, 566,  584, 574,  576, 566,  584, 566,  586, 1684,  582, 1692,  578, 1682,  586, 1682,  584, 1684,  584, 1682,  584, 1684,  584, 1684,  586, 564,  584, 566,  586, 564,  586, 566,  584, 568,  584, 564,  584, 566,  586, 564,  582, 1688,  584, 564,  586, 564,  586, 1684,  584, 564,  586, 564,  586, 562,  586, 1682,  586, 564,  586, 1684,  584, 1684,  582, 566,  582, 1686,  588, 1680,  558, 1710,  586, 564,  584, 1686,  582, 1684,  584, 576,  576, 568,  580, 1686,  586, 564,  584, 1684,  582, 1684,  584, 568,  586, 564,  584, 1684,  584, 1684,  586, 562,  586, 1684,  582, 566,  586, 564,  586, 564,  586, 1684,  584, 564,  584, 1692,  574, 568,  586, 1682,  584, 568,  584, 1682,  586, 1684,  584, 566,  586, 1684,  584, 566,  582, 1686,  582, 566,  584, 1684,  584, 564,  586, 7386,  586};
uint16_t AC_OFFrawData[197] = {6094, 7330,  618, 1654,  620, 1650,  614, 1650,  620, 1654,  616, 1652,  616, 1650,  618, 1650,  618, 1650,  616, 536,  614, 536,  616, 536,  614, 538,  612, 536,  614, 534,  616, 536,  614, 536,  616, 1654,  614, 1650,  616, 1654,  616, 1654,  614, 1652,  616, 1654,  616, 1652,  612, 1658,  612, 534,  616, 534,  616, 538,  586, 562,  612, 538,  612, 536,  612, 540,  610, 536,  614, 1652,  616, 1652,  614, 1656,  612, 1654,  614, 1654,  616, 1652,  616, 1654,  614, 1650,  616, 536,  614, 536,  616, 534,  616, 534,  614, 534,  616, 536,  614, 534,  616, 536,  614, 1656,  614, 1654,  612, 536,  616, 1654,  612, 538,  614, 534,  616, 536,  614, 1654,  616, 536,  612, 536,  616, 1652,  612, 536,  588, 1682,  612, 1656,  612, 1654,  588, 566,  610, 1654,  612, 1658,  612, 540,  612, 538,  612, 1656,  612, 536,  586, 1684,  612, 1652,  612, 538,  616, 538,  610, 1654,  614, 1656,  612, 538,  614, 1654,  614, 536,  614, 536,  612, 536,  614, 1656,  612, 538,  612, 1654,  616, 536,  610, 1660,  612, 538,  612, 1652,  618, 1652,  614, 538,  612, 1656,  612, 536,  616, 1652,  614, 536,  614, 1652,  616, 536,  614, 7354,  618};

// ── Button & output pins ──────────────────────────────────────────────────────
#define LeftTopButton     eP5_KeyA
#define LeftBottomButton  eP4
#define RightTopButton    eP6
#define RightBottomButton eP8
#define TriacLight        eP9
#define TriacFan          18   // Native ESP32-S3 GPIO18; camera removed (CAM_D6)
//#define BaclLED           eLCD_BLK    //using this pin blocks voice, idk why
uint8_t fanSpeed = 4; // fan speed changes from 0 to 4

// ── Page enum ─────────────────────────────────────────────────────────────────
enum Page { PAGE_MAIN, PAGE_ALARM};
static volatile Page currentPage = PAGE_MAIN;
int pageSwitchPressTime = 1000; // 1 second press on the mode button changes to next page

// Alarm state ───────────────────────────────────────────────────────────────
static volatile int  alarmHour    = 5;  // default alarm time 5:30 AM, can be changed via buttons on the alarm page
static volatile int  alarmMinute  = 30;
static volatile bool alarmEnabled = true;
static volatile bool alarmFired   = false;
static volatile bool alarmRunning = false;
// Alarm timeout; the built-in ringtone loops until dismissed or one hour elapses.
int alarmDurationMins = 30; // 30 minutes

// Shared appliance state ────────────────────────────────────────────────────
static volatile bool TriacLightState = false;
static volatile bool TriacFanState   = false;
static volatile bool ACState         = false;

// Semaphores / mutexes ──────────────────────────────────────────────────────
static SemaphoreHandle_t stateMutex = NULL;  // protects appliance states + page
static SemaphoreHandle_t irMutex    = NULL;  // protects irsend
static SemaphoreHandle_t AlarmMutex = NULL;  // protects alarm state
static SemaphoreHandle_t DisplayMutex = NULL; // protects display brightness
uint8_t duty; // current backlight duty cycle (0-255)

//Page-change tracking (UI task only, no mutex needed) ─────────────────────
static volatile bool backgroundNeedsRedraw = true;   // force draw on first frame
uint32_t backGroundColor = 0x2C4C9C;


// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
void toggleLight() {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    TriacLightState = !TriacLightState;
    digital_write(TriacLight, TriacLightState ? HIGH : LOW);
    xSemaphoreGive(stateMutex);
}

// ─────────────────────────────────────────────────────────────────────────────
void toggleFan() {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    TriacFanState = !TriacFanState; //only triggers the RunFanSpeed active/inactive
    xSemaphoreGive(stateMutex);
}

// ── FAN SPEED CONTROL: GPIO8 ZCD + GPIO18 hardware-timed TRIAC firing ──────
// GPIO8 and GPIO18 are NATIVE ESP32-S3 pins (not K10 eP8/eP10).
// The K10 camera must remain disconnected: GPIO8=CAM_D0, GPIO18=CAM_D6.
// Arduino-ESP32 2.x timer API is used (matching ledcSetup elsewhere in this file).
// The MOC3021 LED must be driven by a suitable external transistor/driver.
#define FAN_ZCD_GPIO 8

// Bangladesh: 50 Hz mains -> 10,000 microseconds per HALF-cycle.
static const uint32_t FAN_HALF_CYCLE_US   = 10000UL;
static const uint32_t FAN_ZCD_MIN_GAP_US  = 7000UL;  // reject spurious ZCD edges
static const uint32_t FAN_ZCD_TIMEOUT_US  = 30000UL; // missing ZCD => no gate drive
static const uint32_t FAN_GATE_PULSE_US   = 1200UL;  // gate drive for 1.2 ms
static const uint32_t FAN_TICK_US         = 100UL;   // 100-us hardware timer tick
static const uint32_t FAN_MAX_LATE_US     = 500UL;   // do not fire a badly late pulse
static const uint32_t FAN_GATE_GPIO_MASK = (1UL << TriacFan); // GPIO18 < 32

// Set firing angles for speeds 1..3. Later = lower effective motor voltage.
// TUNE THESE THREE VALUES for your particular fan; they are not RPM settings.
static const uint32_t FAN_DELAY_LOW_US    = FAN_HALF_CYCLE_US * 60UL / 100UL; // 6000us, 108 deg
static const uint32_t FAN_DELAY_MED_US    = FAN_HALF_CYCLE_US * 40UL / 100UL; // 4000us,  72 deg
static const uint32_t FAN_DELAY_HIGH_US   = FAN_HALF_CYCLE_US * 20UL / 100UL; // 2000us,  36 deg

// 32-bit variables are atomically read/written on the ESP32-S3.
// Use the same spinlock to read the crossing timestamp and count consistently.
static portMUX_TYPE fanZcdMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t fanLastCrossUs = 0;
static volatile uint32_t fanCrossCount  = 0;
static volatile uint8_t  fanRequestedSpeed = 0; // safe OFF until ButtonTasks runs
static hw_timer_t *fanPhaseTimer = nullptr;

// Called on each zero-crossing rising edge (once per AC HALF-cycle).
// Never perform I2C, Serial printing, delays or long computations here.
void IRAM_ATTR onFanZeroCross() {
    const uint32_t now = micros();
    portENTER_CRITICAL_ISR(&fanZcdMux);
    if ((uint32_t)(now - fanLastCrossUs) >= FAN_ZCD_MIN_GAP_US) {
        fanLastCrossUs = now;
        ++fanCrossCount;
    }
    portEXIT_CRITICAL_ISR(&fanZcdMux);
}

// Runs once per 100 us from an ESP32-S3 hardware timer, independent of
// display refresh, buttons, Wi-Fi, and FreeRTOS task scheduling.
// The GPIO18 output is written DIRECTLY (no I2C calls in an ISR).
void IRAM_ATTR onFanPhaseTimer() {
    static uint32_t handledCross = 0;
    static uint32_t pulseStartUs = 0;
    static uint8_t  lastSpeed = 0;
    static bool     firedThisHalfCycle = true;
    static bool     gateHigh = false;

    uint32_t crossingUs, crossCount;
    portENTER_CRITICAL_ISR(&fanZcdMux);
    crossingUs = fanLastCrossUs;
    crossCount = fanCrossCount;
    portEXIT_CRITICAL_ISR(&fanZcdMux);

    const uint32_t now = micros();
    const uint32_t elapsed = (uint32_t)(now - crossingUs);
    const uint8_t speed = fanRequestedSpeed;

    // Fail OFF if the fan is disabled or the ZCD disappears.
    if (speed == 0 || speed > 4 || crossCount == 0 ||
        elapsed > FAN_ZCD_TIMEOUT_US) {
        if (gateHigh) GPIO.out_w1tc = FAN_GATE_GPIO_MASK;
        gateHigh = false;
        firedThisHalfCycle = true;
        handledCross = crossCount;
        lastSpeed = speed;
        return;
    }

    // Do not accidentally issue a partial-cycle pulse when speeds change.
    if (speed != lastSpeed) {
        if (gateHigh) GPIO.out_w1tc = FAN_GATE_GPIO_MASK;
        gateHigh = false;
        firedThisHalfCycle = true;
        handledCross = crossCount;
        lastSpeed = speed;
        if (speed != 4) return; // phase-controlled modes begin next half-cycle
    }

    // Speed 4: continuous optotriac LED drive, as in the original code.
    // ZCD timeout above still shuts the LED off when synchronization is lost.
    if (speed == 4) {
        if (!gateHigh) GPIO.out_w1ts = FAN_GATE_GPIO_MASK;
        gateHigh = true;
        handledCross = crossCount;
        firedThisHalfCycle = true;
        return;
    }

    // Reset pulse state on a new half-cycle.
    if (crossCount != handledCross) {
        if (gateHigh) GPIO.out_w1tc = FAN_GATE_GPIO_MASK;
        gateHigh = false;
        handledCross = crossCount;
        firedThisHalfCycle = false;
    }

    // Gate pulse is short; main TRIAC conducts until its current reaches zero.
    if (gateHigh && (uint32_t)(now - pulseStartUs) >= FAN_GATE_PULSE_US) {
        GPIO.out_w1tc = FAN_GATE_GPIO_MASK;
        gateHigh = false;
    }

    uint32_t delayUs = FAN_DELAY_HIGH_US;
    if (speed == 1)      delayUs = FAN_DELAY_LOW_US;
    else if (speed == 2) delayUs = FAN_DELAY_MED_US;

    // One gate pulse per half-cycle, at the chosen phase angle.
    if (!firedThisHalfCycle && elapsed >= delayUs) {
        firedThisHalfCycle = true;
        if (elapsed <= delayUs + FAN_MAX_LATE_US) {
            GPIO.out_w1ts = FAN_GATE_GPIO_MASK;
            gateHigh = true;
            pulseStartUs = now;
        }
    }
}

// Called by the EXISTING ButtonTasks fan-control section.
// No busy waiting: ISR-based timing keeps working while other tasks run.
// speed: 0=OFF, 1=LOW, 2=MEDIUM, 3=HIGH, 4=FULL.
void RunFanSpeed(uint8_t speed) {
    fanRequestedSpeed = (fanPhaseTimer != nullptr && speed <= 4) ? speed : 0;
}

// ─────────────────────────────────────────────────────────────────────────────
void toggleAC() {
    xSemaphoreTake(stateMutex, portMAX_DELAY);
    bool newState = !ACState;
    xSemaphoreGive(stateMutex);

    xSemaphoreTake(irMutex, portMAX_DELAY);
    //suspend all other tasks on this code during transmission (very important)
    vTaskSuspendAll();
    if (newState) {
        irsend.sendRaw(AC_ONrawData,  197, 38);
        Serial.println("IR → AC ON");
    } else {
        irsend.sendRaw(AC_OFFrawData, 197, 38);
        Serial.println("IR → AC OFF");
    }
    xTaskResumeAll();
    xSemaphoreGive(irMutex);

    xSemaphoreTake(stateMutex, portMAX_DELAY);
    ACState = newState;
    xSemaphoreGive(stateMutex);
}

// ─────────────────────────────────────────────────────────────────────────────
void setBacklight(uint16_t lux) {
    xSemaphoreTake(DisplayMutex, portMAX_DELAY);
    if (lux < minLightIntensity) {
        duty = 0;           // display off
    } else if (lux >= maxLightIntensity) {
        duty = 255;         // full brightness
    } else {
        // linear ramp
        // minimum 50 (not 0) so the screen is always barely visible at lux=minLightIntensity
        uint32_t range = maxLightIntensity - minLightIntensity;
        uint32_t delta = lux - minLightIntensity;
        duty = (uint16_t)(50 + (delta * (255 - 50)) / range);
    }
    ledcWrite(BackLED, duty);
    xSemaphoreGive(DisplayMutex);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper – draws a vertical bar gauge using pipe characters
// minVal, maxVal = scale range
// label    = "Lux", "Hum", "Tmp"
// unit     = "%", "C", ""
// barHeight = total number of '|' characters at full scale
void drawVerticalBar(int x, int y, float value, float minVal, float maxVal, const char* label, const char* unit, int barHeight = 10) {
    // Clamp & compute how many filled bars
    float clamped = constrain(value, minVal, maxVal);
    int filled    = (int)roundf((clamped - minVal) / (maxVal - minVal) * barHeight);

    // Draw label + numeric value above the bar
    char header[20];
    snprintf(header, sizeof(header), "%s:%.1f%s", label, value, unit);
    k10.canvas->canvasText(header, x, y, 0xFFFFFF, k10.canvas->eCNAndENFont16, 10, true);

    // Build the bar string:  filled = '|',  empty = '.'
    char bar[barHeight + 1];
    for (int i = 0; i < barHeight; i++)
        bar[i] = (i < filled) ? '|' : '.';
    bar[barHeight] = '\0';
    k10.canvas->canvasText(bar, x, y +18 , 0x00FF88, k10.canvas->eCNAndENFont24, barHeight, true);
}

// ─────────────────────────────────────────────────────────────────────────────
void Configure_WIFI() {
    wm.resetSettings();
    wm.setConfigPortalTimeout(100);
    wm.setShowInfoErase(false);
    wm.setClass("invert");
    bool res = wm.autoConnect(WIFI_SSID_DEBUG, WIFI_PASS_DEBUG);
    if (!res) Serial.println("WiFi portal: failed to connect");
    else      Serial.println("New WiFi saved successfully");
    wm.stopConfigPortal();
}
// ─────────────────────────────────────────────────────────────────────────────

// TASKS ─────────────────────────────────────────────────────────────────────────────
// TASK: VoiceTasks
// ─────────────────────────────────────────────────────────────────────────────
void VoiceTasks(void *pvParameters) {
    while (true) {
        if (asr.isWakeUp()) {
            if (!alarmRunning) music.playMusic(Melodies::BA_DING, OnceInBackground);
            asr._wakeUp = false;
        }
        if (asr.isDetectCmdID(0+1)) {
            asr._isDetectCmdID = 0;
            if (!alarmRunning) music.playMusic(Melodies::BA_DING, OnceInBackground);
            toggleLight();
        } else if (asr.isDetectCmdID(1+1)) {
            asr._isDetectCmdID = 0;
            if (!alarmRunning) music.playMusic(Melodies::BA_DING, OnceInBackground);
            toggleFan();
        } else if (asr.isDetectCmdID(2+1)) {
            asr._isDetectCmdID = 0;
            if (!alarmRunning) music.playMusic(Melodies::BA_DING, OnceInBackground);
            toggleAC();
        }
        else if (asr.isDetectCmdID(3+1)) {
            asr._isDetectCmdID = 0;
            if (!alarmRunning) music.playMusic(Melodies::BA_DING, OnceInBackground);
            xSemaphoreTake(stateMutex, portMAX_DELAY);
            currentPage = PAGE_ALARM;
            xSemaphoreGive(stateMutex);
            backgroundNeedsRedraw = true;
        }
        else if (asr.isDetectCmdID(4+1)) {
            asr._isDetectCmdID = 0;
            if (!alarmRunning) music.playMusic(Melodies::BA_DING, OnceInBackground);
            xSemaphoreTake(stateMutex, portMAX_DELAY);
            currentPage = PAGE_MAIN;
            xSemaphoreGive(stateMutex);
            backgroundNeedsRedraw = true;
        }
        else if (asr.isDetectCmdID(5+1)) {
            asr._isDetectCmdID = 0;
            if (!alarmRunning) music.playMusic(Melodies::BA_DING, OnceInBackground);
            xSemaphoreTake(stateMutex, portMAX_DELAY);
            fanSpeed = 4; // full speed
            xSemaphoreGive(stateMutex);
            Serial.println("Fan set to full speed via voice command");
        }
        else if (asr.isDetectCmdID(6+1)) {
            asr._isDetectCmdID = 0;
            if (!alarmRunning) music.playMusic(Melodies::BA_DING, OnceInBackground);
            xSemaphoreTake(stateMutex, portMAX_DELAY);
            fanSpeed = 1; // low speed
            xSemaphoreGive(stateMutex);
            Serial.println("Fan set to low speed via voice command");
        }
        else if (asr.isDetectCmdID(7+1)) {
            asr._isDetectCmdID = 0;
            if (!alarmRunning) music.playMusic(Melodies::BA_DING, OnceInBackground);
            xSemaphoreTake(stateMutex, portMAX_DELAY);
            fanSpeed = 2; // medium speed
            xSemaphoreGive(stateMutex);
            Serial.println("Fan set to medium speed via voice command");
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// TASK: ButtonTasks
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
        bool releasedLeftTop = !curLeftTop &&  prevLeftTop;

        // ALARM: Any button PRESS dismisses the ringtone immediately.
        // Consume that press, so dismissing an alarm cannot also toggle an appliance,
        // edit the alarm setting, or switch pages. Works even with screen off.
        bool dismissedAlarm = false;
        if (pressedLeftTop || pressedLeftBottom || pressedRightTop || pressedRightBottom) {
            xSemaphoreTake(AlarmMutex, portMAX_DELAY);
            if (alarmRunning) {
                alarmRunning = false;
                alarmFired = true; // prevent retrigger during the current alarm minute
                dismissedAlarm = true;
            }
            xSemaphoreGive(AlarmMutex);
            if (dismissedAlarm) {
                lbHeld = false; // suppress the later LeftTop release/long-press action
                Serial.println("Alarm dismissed by button");
            }
        }

        Page page;
        xSemaphoreTake(stateMutex, portMAX_DELAY);
            page = currentPage;
        xSemaphoreGive(stateMutex);
        xSemaphoreTake(DisplayMutex, portMAX_DELAY);
            uint8_t dutyCopy = duty; // make a local copy of duty for decision making to avoid holding the mutex during long operations
        xSemaphoreGive(DisplayMutex);


        //When buttons pressed
        //turn display on if it is off
        if (!dismissedAlarm && (pressedLeftBottom || pressedRightBottom || pressedLeftTop || pressedRightTop) && dutyCopy == 0) {
            setBacklight(maxLightIntensity); // turn on to max brightness on button press if currently off for better visibility of UI during interaction, will auto-dim after 2 seconds based on ambient light
        }
        else if (!dismissedAlarm){
            // ── LeftBottomButton ─────────────────────────────────────────────────
            // AC functionality in main page and alarm toggle in alarm page
            if (pressedLeftBottom) {
                if (page == PAGE_MAIN) {
                    toggleAC();
                } else {
                    xSemaphoreTake(AlarmMutex, portMAX_DELAY);
                    alarmEnabled = !alarmEnabled;
                    if (alarmEnabled) alarmFired = false;  // re-arm when enabling
                    xSemaphoreGive(AlarmMutex);
                }
            }

            // ── RightBottomButton ─────────────────────────────────────────────
            // toggle fan in home page and increase minute in alarm page
            if (pressedRightBottom) {
                if (page == PAGE_MAIN) {
                    toggleFan();
                } else {
                    xSemaphoreTake(AlarmMutex, portMAX_DELAY);
                    alarmMinute = (alarmMinute + 2) % 60;
                    alarmFired  = false;   // time changed, re-arm
                    xSemaphoreGive(AlarmMutex);
                }
            }

            // ── RightTopButton ────────────────────────────────────────────────
            // toggle light in home page and increase hour in alarm page
            if (pressedRightTop) {
                if (page == PAGE_MAIN) {
                    toggleLight();
                } else {
                    xSemaphoreTake(AlarmMutex, portMAX_DELAY);
                    alarmHour = (alarmHour + 1) % 24;
                    alarmFired = false;   // time changed, re-arm
                    xSemaphoreGive(AlarmMutex);
                }
            }

            // ── LeftTopButton ──────────────────────────────────────────────
            // switch pages and alarm off in both pages with long-press and short-press respectively and speed control
            if (pressedLeftTop) {
                lbHeld      = true;
                lbPressTime = millis();
                lbLongFired = false;
            }
            //if pressed for long time, toggle page. if released before long-press threshold, toggle alarm (if on alarm page) or change fan speed (if on main page)
            if (lbHeld && curLeftTop) {
                if (!lbLongFired && (millis() - lbPressTime >= pageSwitchPressTime)) {
                    // long press, alarm was not running switch page
                    lbLongFired = true;
                    xSemaphoreTake(stateMutex, portMAX_DELAY);
                    currentPage = (currentPage == PAGE_MAIN) ? PAGE_ALARM : PAGE_MAIN;
                    xSemaphoreGive(stateMutex);
                    backgroundNeedsRedraw = true;
                    Serial.println(currentPage == PAGE_MAIN ? "Page: MAIN" : "Page: ALARM");
                }
            }
            // short press action (only if not long-pressed): toggle alarm if on alarm page, change fan speed if on main page
            if (releasedLeftTop && lbHeld) {
                //change speed of fan at short presses when fan is on.
                if (page == PAGE_MAIN && TriacFanState == true && !lbLongFired) {
                    xSemaphoreTake(stateMutex, portMAX_DELAY);
                    fanSpeed = (fanSpeed % 4) + 1;  // cycle through 1,2,3,4 (0 is off, 4 is full on)
                    xSemaphoreGive(stateMutex);
                }

                lbHeld = false;
            }


            // ── enter config mode ─────────────────────────────────────────────
            if (pressedLeftBottom && pressedRightBottom) {
                music.playMusic(Melodies::BA_DING, OnceInBackground);
                Serial.println("Entering OTA Mode");
                Configure_WIFI();       //enable WiFi and block here until new credentials are entered
            }
        }

        prevLeftTop     = curLeftTop;
        prevLeftBottom  = curLeftBottom;
        prevRightTop    = curRightTop;
        prevRightBottom = curRightBottom;
        
        //Fan control
        //vTaskDelay(pdMS_TO_TICKS(20));
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        bool triacfanstatee = TriacFanState;
        uint8_t fanspeede = fanSpeed;
        xSemaphoreGive(stateMutex);
        // Run fan speed control in button task for more responsive control since it uses timing-based control
        if(triacfanstatee){
            RunFanSpeed(fanspeede);  // from speed 1 to 4
        }
        else{
            RunFanSpeed(0); // fully off
        }
    }
}

//Task: AlarmTasks
// ─────────────────────────────────────────────────────────────────────────────
void AlarmTasks(void *pvParameters) {
    struct tm timeinfo = {}; // initialized, so debug output is safe before NTP sync
    bool alarmSoundActive = false; // owned by this task
    uint32_t alarmStartMs = 0;

    while (true) {
        // The ESP32 keeps its synchronized clock running even after WiFi disconnects.
        // This also returns false if the clock has not synchronized since boot.
        bool timeOkk = getLocalTime(&timeinfo, 0);

        xSemaphoreTake(AlarmMutex, portMAX_DELAY);
        int  aHour    = alarmHour;
        int  aMin     = alarmMinute;
        bool aEnabled = alarmEnabled;
        bool aFired   = alarmFired;
        bool aRunning = alarmRunning;
        xSemaphoreGive(AlarmMutex);

        Serial.printf("Alarm check: timeOk=%d enabled=%d fired=%d running=%d "
                      "now=%02d:%02d alarm=%02d:%02d sound=%d\n",
                      timeOkk, aEnabled, aFired, aRunning,
                      timeOkk ? timeinfo.tm_hour : -1,
                      timeOkk ? timeinfo.tm_min : -1,
                      aHour, aMin, alarmSoundActive);

        // Start once when the valid local clock reaches the selected minute.
        if (aEnabled && timeOkk && !aFired && !aRunning &&
            timeinfo.tm_hour == aHour && timeinfo.tm_min == aMin) {
            xSemaphoreTake(AlarmMutex, portMAX_DELAY);
            if (alarmEnabled && !alarmFired && !alarmRunning) {
                alarmRunning = true;
                aRunning = true;
                alarmStartMs = millis();
                Serial.println("Alarm triggered");
            }
            xSemaphoreGive(AlarmMutex);
        }

        // Keep ringing for 60 minutes unless dismissed by a button.
        if (aRunning && (uint32_t)(millis() - alarmStartMs) >=
                            (uint32_t)alarmDurationMins * 60000UL) {
            xSemaphoreTake(AlarmMutex, portMAX_DELAY);
            alarmRunning = false;
            alarmFired   = true;
            xSemaphoreGive(AlarmMutex);
            Serial.println("Alarm stopped: one-hour timeout");
        }

        // Re-arm for the next day after the scheduled clock minute has passed.
        if (aFired && timeOkk &&
            (timeinfo.tm_hour != aHour || timeinfo.tm_min != aMin)) {
            xSemaphoreTake(AlarmMutex, portMAX_DELAY);
            alarmFired = false;
            xSemaphoreGive(AlarmMutex);
        }

        // Start / stop the ACTUAL audio (previous code only changed alarmRunning).
        xSemaphoreTake(AlarmMutex, portMAX_DELAY);
        bool shouldRing = alarmRunning;
        xSemaphoreGive(AlarmMutex);

        if (shouldRing && !alarmSoundActive) {
            alarmMusic.playMusic(Melodies::JUMP_UP, ForeverInBackground);
            alarmSoundActive = true;
            Serial.println("Alarm ringtone started");
        } else if (!shouldRing && alarmSoundActive) {
            alarmMusic.stopPlayTone();
            alarmSoundActive = false;
            Serial.println("Alarm ringtone stopped");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

//UI TASKS
// ─────────────────────────────────────────────────────────────────────────────
void UITasks(void *pvParameters) {
    char timeStringBuff[20];
    struct tm timeinfo;
    Page lastPage = PAGE_MAIN;

    while (true) {
        // ── Snapshot appliance + page state ──────────────────────────────
        xSemaphoreTake(stateMutex, portMAX_DELAY);
        bool lightOn = TriacLightState;
        bool fanOn   = TriacFanState;
        bool acOn    = ACState;
        Page page    = currentPage;
        xSemaphoreGive(stateMutex);

        
        // ── Redraw background only on page change ─────────────────────────
        if (backgroundNeedsRedraw) {
            //k10.canvas->canvasClear(13); doesnt work
            k10.canvas->clearLocalCanvas(0, 0, 240, 320);   // full clear before redraw
            k10.setScreenBackground(backGroundColor);
            backgroundNeedsRedraw = false;
        }

        // ── Get time ONCE, outside WiFi check, with a hard timeout ────────
        // getLocalTime()'s second argument is the timeout in ms.
        // Default is 5000ms — set it to 0 to return immediately from cache.
        // This prevents blocking UITasks when WiFi is down.
        bool timeOk = getLocalTime(&timeinfo, 0);  // 0ms timeout, never blocks

        // ── Top bar ───────────────────────────────────────────────────────
        if (WiFi.status() == WL_CONNECTED) {
            if (timeOk) {
                strftime(timeStringBuff, sizeof(timeStringBuff), "%I:%M %p", &timeinfo);
                k10.canvas->canvasText(timeStringBuff, 10, 5, 0xFFFFFF,
                                       k10.canvas->eCNAndENFont24, 8, true);
            } else {
                k10.canvas->canvasText("!Time!", 10, 5, 0xFF0000,
                                       k10.canvas->eCNAndENFont24, 6, true);
            }
            k10.canvas->canvasDrawBitmap(190, 7, WIFIICON_WIDTH, WIFIICON_HEIGHT, WifiIcon);
        } else {
            // Still show cached time if available even without WiFi
            if (timeOk) {
                strftime(timeStringBuff, sizeof(timeStringBuff), "%I:%M %p", &timeinfo);
                k10.canvas->canvasText(timeStringBuff, 10, 5, 0xAAAAAA,  // greyed out = stale
                                       k10.canvas->eCNAndENFont24, 8, true);
            } else {
                k10.canvas->canvasText("!Time!", 10, 5, 0xFF0000,
                                       k10.canvas->eCNAndENFont24, 6, true);
            }
            k10.canvas->canvasDrawBitmap(190, 7, NOWIFIICON_WIDTH, NOWIFIICON_HEIGHT, NoWifiIcon);
        }
        k10.canvas->canvasLine(10, 35, 230, 35, 0xFFFFFF);

        // DRAW: MAIN PAGE  — runs regardless of WiFi status
        // ─────────────────────────────────────────────────────────────────────────────
        if (page == PAGE_MAIN) {
            // k10.canvas->canvasText(" [LIGHT]",   3, 0xFFFFFF);
            // k10.canvas->canvasText("  [FAN] (" + String(fanSpeed) + ")",  5, 0xFFFFFF);
            // k10.canvas->canvasText("      [AC]", 7, 0xFFFFFF);
            k10.canvas->canvasText(" " + String(fanSpeed), 210, 101, 0xFFFFFF, k10.canvas->eCNAndENFont24, 11, true);
            k10.canvas->canvasDrawBitmap(16, 43, LIGHT_ICON_WIDTH, LIGHT_ICON_HEIGHT, light_icon);
            k10.canvas->canvasDrawBitmap(16, 93, FAN_ICON_WIDTH, FAN_ICON_HEIGHT, FAN_icon);
            k10.canvas->canvasDrawBitmap(16, 143, AC_ICON_WIDTH, AC_ICON_HEIGHT, AC_icon);


            k10.canvas->canvasDrawBitmap(127,  53, RED_OFF_SWITCH_WIDTH, RED_OFF_SWITCH_HEIGHT,
                lightOn ? greenbutton : Red_off_switch);
            k10.canvas->canvasDrawBitmap(127, 102, RED_OFF_SWITCH_WIDTH, RED_OFF_SWITCH_HEIGHT,
                fanOn   ? greenbutton : Red_off_switch);
            k10.canvas->canvasDrawBitmap(127, 147, RED_OFF_SWITCH_WIDTH, RED_OFF_SWITCH_HEIGHT,
                acOn    ? greenbutton : Red_off_switch);

            k10.canvas->canvasLine(10, 191, 230, 191, 0xFFFFFF);

            //refresh backlight every 2 seconds
            static unsigned long lastBacklightUpdate = 0;
            if (millis() - lastBacklightUpdate >= 4000) {
                lastBacklightUpdate = millis();
                uint16_t newlux = k10.readALS(); // measure ambient light for backlight control and display on UI
                if (newlux > 0) lightIntensity = newlux; // update global light intensity if reading is valid (non-zero)
                setBacklight(lightIntensity);   // also update the backlight duty cycle based on the current ambient light level
            }

            sensors_event_t humidity_event, temp_event; // local variables to hold sensor readings
            aht.getEvent(&humidity_event, &temp_event); // store current humidity and temperature readings
            
            //draw intensity lines on main screen
            const int BAR_Y = 200;
            drawVerticalBar( 10, BAR_Y, (float)lightIntensity,              0, 400, "Lux", "",  10);
            drawVerticalBar( 90, BAR_Y, humidity_event.relative_humidity,   0, 100, "Hum", "%", 10);
            drawVerticalBar(170, BAR_Y, temp_event.temperature - 8,             0,  70, "Tmp", "C", 10); // counter the offset in temperature readings, probably due to heat from the device itself.

            // draw a line and the alarm status at the bottom of the main page (even when WiFi is down, using cached alarm state)
            k10.canvas->canvasLine(10, 257, 230, 257, 0xFFFFFF);

            // Display alarm time and status at the bottom of main screen
            char alarmHint[24];
            xSemaphoreTake(AlarmMutex, portMAX_DELAY);
            snprintf(alarmHint, sizeof(alarmHint), " Alarm %02d:%02d %s",
                     alarmHour, alarmMinute, alarmEnabled ? "[ON]" : "[OFF]");
            xSemaphoreGive(AlarmMutex);
            k10.canvas->canvasText(alarmHint, 12, alarmEnabled ? 0x00FF88 : 0x888888);
            k10.canvas->canvasText("alaminn.com", 10, 305, 0xFFFFFF,
                                   k10.canvas->eCNAndENFont16, 11, true);
        }

        // DRAW: ALARM PAGE  — runs regardless of WiFi status
        // ─────────────────────────────────────────────────────────────────────────────
        else {
            k10.canvas->canvasText("-- ALARM --", 50, 40, 0xFFFFFF,
                                   k10.canvas->eCNAndENFont24, 11, true);
            k10.canvas->canvasDrawBitmap(65, 68, ALARMCLOCKk_WIDTH, ALARMCLOCKk_HEIGHT, alarmclockk);

            char alarmTimeBuff[10];
            xSemaphoreTake(AlarmMutex, portMAX_DELAY);
            snprintf(alarmTimeBuff, sizeof(alarmTimeBuff), "%02d:%02d", alarmHour, alarmMinute);
            bool aEnabled = alarmEnabled;
            xSemaphoreGive(AlarmMutex);

            k10.canvas->canvasText(alarmTimeBuff, 88, 240, aEnabled ? 0x00FF88 : 0xFF4444,
                                   k10.canvas->eCNAndENFont24, 8, true);
            k10.canvas->canvasText(aEnabled ? "ON" : "OFF", 88, 275,
                                   aEnabled ? 0x00FF88 : 0xFF4444,
                                   k10.canvas->eCNAndENFont24, 8, true);
            k10.canvas->canvasLine(10, 227, 230, 227, 0xFFFFFF);
            k10.canvas->canvasText("[A] Toggle",    10, 295, 0xFFFFFF, k10.canvas->eCNAndENFont16,  10, false);
            k10.canvas->canvasText("Hr [^]",     185, 235, 0xFFFFFF, k10.canvas->eCNAndENFont16,  6, false);
            k10.canvas->canvasText("Min [^]",    185, 295, 0xFFFFFF, k10.canvas->eCNAndENFont16,  7, false);
            k10.canvas->canvasText("[<] Back",  10, 235, 0xFFFFFF, k10.canvas->eCNAndENFont16, 8, false);
        }

        k10.canvas->updateCanvas();
        vTaskDelay(pdMS_TO_TICKS(100));

        //see the task stack
        UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        Serial.printf("UITasks free stack: %u words\n", stackLeft);
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    k10.begin();
    Serial.begin(115200);
    irsend.begin();

    // ── Create ALL mutexes before any task is spawned ─────────────────────
    stateMutex = xSemaphoreCreateMutex();
    irMutex    = xSemaphoreCreateMutex();
    AlarmMutex = xSemaphoreCreateMutex();
    DisplayMutex = xSemaphoreCreateMutex();

    // ── WiFi ──────────────────────────────────────────────────────────────
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    for (int n = 20; n > 0 && WiFi.status() != WL_CONNECTED; n--) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print('.');
    }
    configTime(6 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    // ── ASR ───────────────────────────────────────────────────────────────
    asr.asrInit(1, EN_MODE, 3000);
    while (asr._asrState == 0) {
        Serial.print('.');
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    asr.addASRCommand(0+1, const_cast<char*>("Lights"));
    asr.addASRCommand(1+1, const_cast<char*>("Fan"));
    asr.addASRCommand(2+1, const_cast<char*>("AC"));
    asr.addASRCommand(3+1, const_cast<char*>("Show Alarm"));
    asr.addASRCommand(4+1, const_cast<char*>("Hide Alarm"));
    asr.addASRCommand(5+1, const_cast<char*>("Full Speed"));
    asr.addASRCommand(6+1, const_cast<char*>("Zero Speed"));
    asr.addASRCommand(7+1, const_cast<char*>("Medium Speed"));


    // ── GPIO ──────────────────────────────────────────────────────────────
    pinMode(LeftTopButton,    INPUT_PULLDOWN);
    pinMode(LeftBottomButton, INPUT_PULLDOWN);
    pinMode(RightTopButton,   INPUT_PULLDOWN);
    pinMode(RightBottomButton,INPUT_PULLDOWN);
    pinMode(TriacLight, OUTPUT);
    pinMode(TriacFan,   OUTPUT); // native ESP32-S3 GPIO18
    digital_write(TriacLight, LOW);
    digitalWrite(TriacFan, LOW); // native GPIO; do NOT use K10 digital_write()

    // Fan: periodic 100-us hardware timer for deterministic phase firing.
    // ESP32-S3 has four timers; timer index 3 is reserved here for the fan.
    // This is Arduino-ESP32 2.x syntax (same core as the existing ledcSetup).
    fanPhaseTimer = timerBegin(3, 80, true); // 80-MHz APB / 80 = 1 MHz
    if (fanPhaseTimer != nullptr) {
        timerAttachInterrupt(fanPhaseTimer, &onFanPhaseTimer, true);
        timerAlarmWrite(fanPhaseTimer, FAN_TICK_US, true);
        timerAlarmEnable(fanPhaseTimer);
        pinMode(FAN_ZCD_GPIO, INPUT); // ZCD already provides logic-level edges
        attachInterrupt(digitalPinToInterrupt(FAN_ZCD_GPIO),
                        onFanZeroCross, RISING);
    } else {
        // Safety: fan remains OFF if timer allocation fails.
        Serial.println("ERROR: Fan phase timer failed; fan output disabled");
    }

    // ── Display ───────────────────────────────────────────────────────────
    k10.initScreen(screen_dir);
    k10.creatCanvas();
    k10.canvas->canvasSetLineWidth(1);
    k10.setScreenBackground(backGroundColor);   // draw background once at startup
    
    //setup backlight PWM (display backlight is wired to P1 via a transistor, so we can use PWM to control brightness if desired. For now we just turn it fully on)
    ledcSetup(2, 5000, 8);  // channel 0, 5 KHz, 8-bit resolution
    ledcAttachPin(BackLED, 2);  // attach pin to channel 0

    // ── AHT sensor ──────────────────────────────────────x─────────────────
    if (!aht.begin()) Serial.println("AHT10/AHT20 not found");

    // ── Tasks ─────────────────────────────────────────────────────────────
    Serial.print("Free heap before ButtonTasks: "); Serial.println(ESP.getFreeHeap());
    xTaskCreatePinnedToCore(ButtonTasks, "ButtonTasks", 1024*8, NULL, 10, NULL, 0);
    Serial.print("Free heap before VoiceTasks:  "); Serial.println(ESP.getFreeHeap());
    xTaskCreatePinnedToCore(VoiceTasks,  "VoiceTasks",  1024*3, NULL, 5, NULL, 1);
    Serial.print("Free heap before UITasks:     "); Serial.println(ESP.getFreeHeap());
    xTaskCreatePinnedToCore(UITasks,     "UITasks",     1024*8, NULL, 5, NULL, 1);
    Serial.print("Free heap before alarm tasks: "); Serial.println(ESP.getFreeHeap());
    xTaskCreatePinnedToCore(AlarmTasks, "AlarmTasks", 1024*5, NULL, 5, NULL, 1);
    Serial.print("Free heap after alarm tasks: "); Serial.println(ESP.getFreeHeap());
}

// LOOP ( 8kb in size) core 1
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
 vTaskDelay(pdMS_TO_TICKS(10));
}
