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

// Forward Declaration
void FireChannels(const int* pins, size_t num_pins, bool state);
void FlushBuffer();

// Core Flight Decision Engine
void RunFlightLogic(const DataPoint& pt) {
    uint32_t now_ms = pt.timestamp_ms;
    float alt_m = pt.alt_m;
    float v_speed = pt.vert_speed_mps;

    switch (current_state) {
        
        case SystemState::BUFFERING:
            if (alt_m >= ALT_ARM_THRESHOLD_M) {
                Serial.println(">> ARMED");
                digitalWrite(PIN_GREEN_LED, HIGH);
                FlushBuffer();
                current_state = SystemState::ARMED;
            }
            break;

        case SystemState::ARMED:
            digitalWrite(PIN_RED_LED, HIGH);
            current_state = SystemState::FLIGHT;
            break;

        case SystemState::FLIGHT:
            if (!contingency_mode && (alt_m >= ALT_DEPLOY_THRESHOLD_M)) {
                green_blinking = true;
                red_blinking   = true;

                // Track Apogee
                if (alt_m > apogee_alt_m) {
                    apogee_alt_m = alt_m;
                }

                // --- DROGUE LOGIC ---
                // Detect descent (-1.0 m/s) and drop from apogee
                if (!drogue_deployed && !drogue_firing && 
                    (v_speed < -1.0F) && (alt_m < apogee_alt_m - 2.0F)) {
                     
                     Serial.println(">> FIRING DROGUE");
                     FireChannels(DEPLOY_PINS_DROGUE, NUM_DROGUE_PINS, HIGH);
                     drogue_firing        = true;
                     drogue_fire_start_ms = now_ms;
                }

                if (drogue_firing && ((now_ms - drogue_fire_start_ms) >= FIRE_DURATION_MS)) {
                    FireChannels(DEPLOY_PINS_DROGUE, NUM_DROGUE_PINS, LOW);
                    drogue_firing   = false;
                    drogue_deployed = true;
                }

                // --- MAIN LOGIC ---
                // Deploy at Fixed Altitude AGL (300m)
                if (drogue_deployed && !main_deployed && !main_firing && 
                    (alt_m <= MAIN_DEPLOY_ALT_AGL_M)) {
                    
                    Serial.println(">> FIRING MAIN");
                    FireChannels(DEPLOY_PINS_MAIN, NUM_MAIN_PINS, HIGH);
                    main_firing        = true;
                    main_fire_start_ms = now_ms;
                }

                if (main_firing && ((now_ms - main_fire_start_ms) >= FIRE_DURATION_MS)) {
                    FireChannels(DEPLOY_PINS_MAIN, NUM_MAIN_PINS, LOW);
                    main_firing   = false;
                    main_deployed = true;
                    current_state = SystemState::DEPLOYED_NORMAL;
                }
            }
            break;

        case SystemState::CONTINGENCY_COUNTDOWN:
             // 1. Timer Based Drogue
             if (!cont_drogue_deployed && !cont_drogue_firing && 
                ((now_ms - contingency_start_ms) >= TIME_BASED_EJECTION_MS)) {
                 
                 Serial.println(">> CONTINGENCY: FIRING DROGUE");
                 FireChannels(DEPLOY_PINS_DROGUE, NUM_DROGUE_PINS, HIGH);
                 cont_drogue_firing   = true;
                 cont_drogue_start_ms = now_ms;
             }
             
             if (cont_drogue_firing && ((now_ms - cont_drogue_start_ms) >= FIRE_DURATION_MS)) {
                 FireChannels(DEPLOY_PINS_DROGUE, NUM_DROGUE_PINS, LOW);
                 cont_drogue_firing   = false;
                 cont_drogue_deployed = true;
             }

             // 2. Timer Based Main
             if (cont_drogue_deployed && !cont_main_deployed && !cont_main_firing && 
                ((now_ms - (cont_drogue_start_ms + FIRE_DURATION_MS)) >= CONTINGENCY_WAIT_MS)) {
                 
                 Serial.println(">> CONTINGENCY: FIRING MAIN");
                 FireChannels(DEPLOY_PINS_MAIN, NUM_MAIN_PINS, HIGH);
                 cont_main_firing   = true;
                 cont_main_start_ms = now_ms;
             }
             
             if (cont_main_firing && ((now_ms - cont_main_start_ms) >= FIRE_DURATION_MS)) {
                 FireChannels(DEPLOY_PINS_MAIN, NUM_MAIN_PINS, LOW);
                 cont_main_firing   = false;
                 cont_main_deployed = true;
                 current_state      = SystemState::CONTINGENCY_SAVING;
             }
             break;

        case SystemState::CONTINGENCY_SAVING:
             if ((now_ms - contingency_save_start_ms) >= CONTINGENCY_SAVE_PERIOD_MS) {
                 FlushBuffer();
                 current_state = SystemState::STOPPED;
             }
             break;

        case SystemState::DEPLOYED_NORMAL:
             if (alt_m <= ALT_RESET_THRESHOLD_M) {
                 reset_countdown_start_ms = now_ms;
                 current_state = SystemState::RESET_COUNTDOWN;
             }
             break;

        case SystemState::RESET_COUNTDOWN:
             if ((now_ms - reset_countdown_start_ms) >= RESET_SAVE_PERIOD_MS) {
                 FlushBuffer();
                 current_state = SystemState::STOPPED;
             }
             break;

        case SystemState::STOPPED:
             green_blinking = false;
             red_blinking   = false;
             digitalWrite(PIN_GREEN_LED, LOW);
             digitalWrite(PIN_RED_LED, HIGH);
             break;
    }
}