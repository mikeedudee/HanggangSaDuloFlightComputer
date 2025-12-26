/*
    OPEN ACCESS LICENSE

    Project: HANGGANG SA DULO Flight Computer v1.1.0
    Copyright (c) 2025 HANGGANG SA DULO Project Team
    Lead Developer: Francis Mike John Camogao (GitHub: https://github.com/mikeedudee)

    Permission is hereby granted, free of charge, to any person, educational institution, or organization to obtain a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, and distribute copies of the Software, subject to the following conditions:

    1. ATTRIBUTION: The above copyright notice, the name of the Lead Developer, and this permission notice shall be included in all copies or substantial portions of the Software.

    2. COMPLIANCE ACKNOWLEDGMENT: This software was developed in strict adherence to the safety-critical coding guidelines established by the Lead Developer. Any derivative works utilizing the coding structure of this project must cite the origin as follows:
    Camogao, F. M. J. & HANGGANG SA DULO Project Team. (2025). HSD FC v1.1.0. Aerospace Engineering Department, Indiana Aerospace University. (Available at: https://github.com/mikeedudee)

    DISCLAIMER OF WARRANTY:
    THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS, THE LEAD DEVELOPER, OR THE HANGGANG SA DULO PROJECT TEAM BE LIABLE FOR ANY CLAIM, DAMAGES, MISSION FAILURE, OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THIS SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "GlobalVariables.h"
#include <esp_task_wdt.h>

// =================================================================================
// GLOBAL VARIABLE DEFINITIONS (Allocating Memory Here)
// =================================================================================
MS5611          ms5611;
WiFiUDP         udp;

float           ref_pressure_pa             = 0.0F;
float           prev_alt_filtered_m         = 0.0F;
int32_t         prev_pressure_pa            = 0;

SystemState     current_state               = SystemState::BUFFERING;
bool            contingency_mode            = false;

uint32_t        reset_countdown_start_ms    = 0UL;
uint32_t        contingency_start_ms        = 0UL;
uint32_t        contingency_save_start_ms   = 0UL;

bool            drogue_deployed             = false;
bool            drogue_firing               = false;
uint32_t        drogue_fire_start_ms        = 0UL;
float           apogee_alt_m                = 0.0F;

bool            main_deployed               = false;
bool            main_firing                 = false;
uint32_t        main_fire_start_ms          = 0UL;

bool            cont_drogue_deployed        = false;
bool            cont_drogue_firing          = false;
uint32_t        cont_drogue_start_ms        = 0UL;

bool            cont_main_deployed          = false;
bool            cont_main_firing            = false;
uint32_t        cont_main_start_ms          = 0UL;

bool            red_led_state               = false;
bool            green_led_state             = false;
bool            red_blinking                = false;
bool            green_blinking              = false;
uint32_t        last_blink_time_ms          = 0UL;

DataPoint       telemetry_buffer[10];
int             buffer_index                = 0;
bool            buffer_is_full              = false;

// =================================================================================
// PROTOTYPES (From other files)
// =================================================================================
void InitSensors();
DataPoint ReadAltimeter(uint32_t now_ms, uint32_t dt_ms);
void RunFlightLogic(const DataPoint& pt);

// Local Helpers
void InitSpiffs();
void AddToBuffer(const DataPoint &pt);
void FlushBuffer();
void FireChannels(const int* pins, size_t num_pins, bool state);
void HandleBlinking(uint32_t now_ms);

// =================================================================================
// SETUP
// =================================================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    // Watchdog
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    InitSpiffs();
    
    // Pin Init
    pinMode(PIN_GREEN_LED, OUTPUT); digitalWrite(PIN_GREEN_LED, LOW);
    pinMode(PIN_RED_LED,   OUTPUT); digitalWrite(PIN_RED_LED,   LOW);
    pinMode(PIN_BUZZER,    OUTPUT); digitalWrite(PIN_BUZZER,    LOW);

    // Pyro Safety
    for (size_t i = 0; i < NUM_DROGUE_PINS; ++i) {
        pinMode(DEPLOY_PINS_DROGUE[i], OUTPUT);
        digitalWrite(DEPLOY_PINS_DROGUE[i], LOW);
    }

    for (size_t i = 0; i < NUM_MAIN_PINS; ++i) {
        pinMode(DEPLOY_PINS_MAIN[i], OUTPUT);
        digitalWrite(DEPLOY_PINS_MAIN[i], LOW);
    }

    // Initialize Sensors (moved to Sensors.ino)
    InitSensors();

    // Wi-Fi Setup
    WiFi.persistent(false);
    WiFi.softAPdisconnect(true);
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, 1, 0, 4);
    udp.begin(UDP_PORT);

    Serial.println("SYSTEM READY.");
}

// =================================================================================
// MAIN LOOP
// =================================================================================
void loop() {
    esp_task_wdt_reset();
    
    static uint32_t last_time_ms = 0UL;
    uint32_t        now_ms       = millis();

    // 1. Rate Limiting
    if ((now_ms - last_time_ms) < LOOP_INTERVAL_MS) {
        HandleBlinking(now_ms);
        return;
    }
    uint32_t dt_ms = now_ms - last_time_ms;
    last_time_ms   = now_ms;

    // 2. Sensor Read & Physics (moved to Sensors.ino)
    DataPoint pt = ReadAltimeter(now_ms, dt_ms);

    // 3. Buffer Data
    AddToBuffer(pt);
    if (buffer_is_full) {
        FlushBuffer();
    }

    // 4. Telemetry Broadcast
    char payload[256];
    int len = snprintf(payload, sizeof(payload),
        "T:%.2f P:%d A:%.2f V:%.2f M:%.3f S:%u C:%u T:%lu",
        pt.temp_c, pt.pressure_pa, pt.alt_m, pt.vert_speed_mps,
        pt.mach, pt.state_id, pt.is_contingency, pt.timestamp_ms
    );
    udp.beginPacket(BROADCAST_IP, UDP_PORT);
    udp.write((uint8_t*)payload, len);
    udp.endPacket();

    // 5. Flight Logic (moved to FlightLogic.ino)
    RunFlightLogic(pt);
}

// =================================================================================
// LOCAL HELPERS
// =================================================================================
void FireChannels(const int* pins, size_t num_pins, bool state) {
    for (size_t i = 0; i < num_pins; ++i) digitalWrite(pins[i], state ? HIGH : LOW);
}

void InitSpiffs() {
    if (!SPIFFS.begin(true)) Serial.println("SPIFFS MOUNT FAILED");
}

void AddToBuffer(const DataPoint &pt) {
    if (buffer_index < 10) {
        telemetry_buffer[buffer_index++] = pt;
        if (buffer_index >= 10) buffer_is_full = true;
    } else {
        memmove(telemetry_buffer, telemetry_buffer + 1, sizeof(DataPoint) * 9);
        telemetry_buffer[9] = pt;
    }
}

void FlushBuffer() {
    if (buffer_index == 0) return;
    File f = SPIFFS.open(SPIFFS_FILENAME, FILE_APPEND);
    if (!f) return;
    for (int i = 0; i < buffer_index; ++i) {
        f.printf("%.2f,%d,%.2f,%.2f,%.2f,%.4f,%.2f,%.3f,%u,%d,%lu\n",
            telemetry_buffer[i].temp_c, telemetry_buffer[i].pressure_pa,
            telemetry_buffer[i].alt_m, telemetry_buffer[i].vert_speed_mps,
            telemetry_buffer[i].pressure_rate_paps, telemetry_buffer[i].density_kgm3,
            telemetry_buffer[i].dyn_pressure_pa, telemetry_buffer[i].mach,
            telemetry_buffer[i].state_id, telemetry_buffer[i].is_contingency ? 1 : 0,
            telemetry_buffer[i].timestamp_ms
        );
    }
    f.close();
    buffer_index = 0;
    buffer_is_full = false;
}

void HandleBlinking(uint32_t now_ms) {
    if ((now_ms - last_blink_time_ms) >= BLINK_INTERVAL_MS) {
        last_blink_time_ms = now_ms;
        if (red_blinking) {
            red_led_state = !red_led_state;
            digitalWrite(PIN_RED_LED, red_led_state);
        }
        if (green_blinking) {
            green_led_state = !green_led_state;
            digitalWrite(PIN_GREEN_LED, green_led_state);
        }
    }
}