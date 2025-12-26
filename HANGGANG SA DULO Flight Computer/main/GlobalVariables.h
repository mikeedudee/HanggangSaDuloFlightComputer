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

#ifndef GLOBALVARIABLES_H
#define GLOBALVARIABLES_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <MS5611.h>
#include "FS.h"
#include "SPIFFS.h"

// =================================================================================
// 1. CONSTANTS (Read-Only) -
// =================================================================================
static const char * const       WIFI_SSID                   = "HANGGANG SA DULO Telemetry";
static const char * const       WIFI_PASSWORD               = "HSDGRP09";
static constexpr uint16_t       UDP_PORT                    = 4210;
static const IPAddress          BROADCAST_IP                (192, 168, 4, 255);
static const char * const       SPIFFS_FILENAME             = "/data.txt";

static constexpr int            WDT_TIMEOUT_S               = 5;
static constexpr uint32_t       LOOP_INTERVAL_MS            = 300UL;
static constexpr uint32_t       CONTINGENCY_SAVE_PERIOD_MS  = 120000UL;
static constexpr uint32_t       RESET_SAVE_PERIOD_MS        = 3000UL;
static constexpr uint32_t       TIME_BASED_EJECTION_MS      = 12000UL;
static constexpr uint32_t       FIRE_DURATION_MS            = 1000UL;
static constexpr uint32_t       CONTINGENCY_WAIT_MS         = 5000UL;
static constexpr uint32_t       BLINK_INTERVAL_MS           = 500UL;

static constexpr float          ALT_ARM_THRESHOLD_M         = 10.0F;
static constexpr float          ALT_DEPLOY_THRESHOLD_M      = 5.0F;
static constexpr float          ALT_RESET_THRESHOLD_M       = 50.0F;
static constexpr float          MAX_ALT_JUMP_M              = 30.0F;
static constexpr float          MAIN_DEPLOY_ALT_AGL_M       = 300.0F;
static constexpr float          GAS_CONSTANT_R              = 287.05F;

static constexpr int            PIN_GREEN_LED               = 19;
static constexpr int            PIN_RED_LED                 = 18;
static constexpr int            PIN_BUZZER                  = 4;

static constexpr int            DEPLOY_PINS_DROGUE[]        = { 32, 33 };
static constexpr int            DEPLOY_PINS_MAIN[]          = { 27, 13 };

static constexpr size_t         NUM_DROGUE_PINS             = sizeof(DEPLOY_PINS_DROGUE) / sizeof(DEPLOY_PINS_DROGUE[0]);
static constexpr size_t         NUM_MAIN_PINS               = sizeof(DEPLOY_PINS_MAIN)   / sizeof(DEPLOY_PINS_MAIN[0]);

// =================================================================================
// 2. DATA STRUCTURES & ENUMS
// =================================================================================
enum class SystemState : uint8_t {
    BUFFERING = 0, ARMED, FLIGHT, CONTINGENCY_COUNTDOWN,
    CONTINGENCY_SAVING, DEPLOYED_NORMAL, RESET_COUNTDOWN, STOPPED
};

typedef struct {
    float temp_c; int32_t pressure_pa; float alt_m; float vert_speed_mps;
    float pressure_rate_paps; float density_kgm3; float dyn_pressure_pa; float mach;
    uint8_t state_id; bool is_contingency; uint32_t timestamp_ms;
} DataPoint;

// =================================================================================
// 3. EXTERNAL DECLARATIONS (Mutable)
// =================================================================================
// Defined in main.ino to prevent "Multiple Definition" errors

extern MS5611           ms5611;
extern WiFiUDP          udp;

extern float            ref_pressure_pa;
extern float            prev_alt_filtered_m;
extern int32_t          prev_pressure_pa;

extern SystemState      current_state;
extern bool             contingency_mode;

extern uint32_t         reset_countdown_start_ms;
extern uint32_t         contingency_start_ms;
extern uint32_t         contingency_save_start_ms;

extern bool             drogue_deployed;
extern bool             drogue_firing;
extern uint32_t         drogue_fire_start_ms;
extern float            apogee_alt_m;

extern bool             main_deployed;
extern bool             main_firing;
extern uint32_t         main_fire_start_ms;

extern bool             cont_drogue_deployed;
extern bool             cont_drogue_firing;
extern uint32_t         cont_drogue_start_ms;

extern bool             cont_main_deployed;
extern bool             cont_main_firing;
extern uint32_t         cont_main_start_ms;

extern bool             red_led_state;
extern bool             green_led_state;
extern bool             red_blinking;
extern bool             green_blinking;
extern uint32_t         last_blink_time_ms;

extern DataPoint        telemetry_buffer[10];
extern int              buffer_index;
extern bool             buffer_is_full;

#endif // GLOBALVARIABLES_H