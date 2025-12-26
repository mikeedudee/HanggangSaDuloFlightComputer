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

// Initialize Sensor with Safety Loop
void InitSensors() {
    ms5611.enableMedianFilter();
    
    // Critical Hardware Check
    if (!ms5611.begin()) {
        Serial.println("CRITICAL FAILURE: MS5611 not found.");
        while (true) {
            // Blink RED fast to indicate error (Deadlock Trap)
            digitalWrite(PIN_RED_LED, !digitalRead(PIN_RED_LED));
            delay(100);
            esp_task_wdt_reset(); 
        }
    }
    
    // Warm-up delay for sensor stability
    delay(1000); 

    // Calibration: Set Ground Reference
    ref_pressure_pa     = ms5611.readPressure();
    float raw_alt_m     = ms5611.getAltitude(ref_pressure_pa, ref_pressure_pa);
    
    // Seed the previous values
    prev_alt_filtered_m = ms5611.medianFilter(raw_alt_m);
    prev_pressure_pa    = ref_pressure_pa;
}

// Read and Calculate Sensor Data
// Returns a DataPoint struct populated with latest physics
DataPoint ReadAltimeter(uint32_t now_ms, uint32_t dt_ms) {
    // 1. Raw Readings
    float temp_c        = ms5611.readTemperature();
    int32_t pres_pa     = ms5611.readPressure();
    float alt_m         = ms5611.getAltitude(pres_pa, ref_pressure_pa);
    float alt_filt_m    = ms5611.medianFilter(alt_m);

    // 2. Derived Calculations
    float v_speed_mps   = ((alt_filt_m - prev_alt_filtered_m) * 1000.0F) / (float)dt_ms;
    float p_rate_paps   = ((pres_pa - prev_pressure_pa) * 1000.0F) / (float)dt_ms;
    float temp_k        = temp_c + 273.15F;
    float density_kgm3  = pres_pa / (GAS_CONSTANT_R * temp_k);
    float dyn_press_pa  = 0.5F * density_kgm3 * v_speed_mps * v_speed_mps;
    float sound_spd_mps = sqrtf(1.4F * GAS_CONSTANT_R * temp_k);
    float mach          = v_speed_mps / sound_spd_mps;

    // 3. Update History
    prev_alt_filtered_m = alt_filt_m;
    prev_pressure_pa    = pres_pa;

    // 4. Glitch Detection (Safety Check)
    if (!contingency_mode && (fabs(alt_filt_m - prev_alt_filtered_m) >= MAX_ALT_JUMP_M)) {
        contingency_mode          = true;
        contingency_start_ms      = now_ms;
        contingency_save_start_ms = now_ms;
        red_blinking              = true;
        Serial.println("!!! CONTINGENCY ACTIVATED: SENSOR SPIKE !!!");
        
        if (current_state == SystemState::FLIGHT) {
            current_state = SystemState::CONTINGENCY_COUNTDOWN;
        }
    }

    // 5. Pack Data
    DataPoint pt = {
        temp_c, pres_pa, alt_filt_m, v_speed_mps, p_rate_paps, 
        density_kgm3, dyn_press_pa, mach,
        static_cast<uint8_t>(current_state), contingency_mode, now_ms
    };
    
    return pt;
}