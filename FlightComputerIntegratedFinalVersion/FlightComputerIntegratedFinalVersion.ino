/*
 * Flight Computer Firmware
 * File: FlightComputerIntegratedFinalVersion.ino
 *
 * Copyright (c) 2025 Hanggang Sa Dulo
 * SPDX-License-Identifier: MIT
 *
 * Summary:
 *   Safety-critical firmware for a rocket/CanSat flight computer providing
 *   state management, ascent/apogee detection, and dual-channel ejection.
 *
 * SAFETY NOTICE:
 *   This software may control energetic devices (e.g., pyrotechnic charges).
 *   Bench-test with inert loads only. Remove all energetic materials during
 *   development. Follow local laws and range safety codes. You are responsible
 *   for integration, testing, and operation in your vehicle.
 *
 * Licensing:
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * SPDX-License-Identifier: MIT
 *
 * Date: 2025-07-20
 * Repository: https://github.com/mikeedudee/HanggangSaDuloFlightComputer.git
 * Contact: francismikejohn.camogao@gmail.com
 */

#include "FS.h"                 // File system
#include "SPIFFS.h"             // SPI Flash file system
#include <Wire.h>               // I2C communication
#include <MS5611.h>             // Barometric sensor library
#include <cstring>              // For memmove shifting values in the buffer
#include <WiFi.h>               // For BroadCasting own Wi-Fi signal
#include <WiFiUdp.h>            // Setup Wi-Fi parameters
#include <math.h>               // For mathetmical operations

WiFiUDP udp;
static constexpr uint16_t   UDP_PORT = 4210;          // Port for UDP communication
static const                IPAddress broadcastIP(192,168,4,255);  // IP for broadcast communication

// “State” of each LED output when blinking
bool redLedState    = false;
bool greenLedState  = false;
bool redBlinking    = false;
bool greenBlinking  = false;

// FILE SYSTEM
static const char * const SPIFFS_FILENAME = "/data.txt";  // File path for logged telemetry

// DATA STRUCTURE 
// Telemetry record: each sample contains raw and derived data
// This structyre holds one timestamped “snapshot” from the MS5611.
typedef struct {
    float          temperature;         // Degrees Celsius
    long           pressure;            // Pascals
    float          altitude;            // Meters (computed from pressure)
    float          verticalSpeed;       // m/s, derived from altitude change
    float          pressureRate;        // Pa/s, derived from pressure change
    float          density;             // kg/m^3, computed via ideal gas law
    float          dynamicPressure;     // Pa, 0.5 * rho * v^2
    float          mach;                // Ratio of v to speed of sound
    uint8_t        stateID;             // Encoded current state
    bool           contingencyFlag;     // True if in contingency mode
    unsigned long  timestamp;           // ms since startup
} DataPoint;

// This enum defines the state machine for the system.
enum class SystemState : uint8_t {
    BUFFERING,                          // Collect initial buffer before arming
    ARMED,                              // Armed and ready, LED indication
    FLIGHT,                             // Normal flight logging and deploy checks
    CONTINGENCY_COUNTDOWN,              // Wait for ejection timer after glitch
    CONTINGENCY_SAVING,                 // Continue logging after contingency deploy
    DEPLOYED_NORMAL,                    // Deployed at target altitude, await descent
    RESET_COUNTDOWN,                    // Logging for a period after descent
    STOPPED                             // Logging stopped, final state
};

// GLOBAL VARIABLES
#include "GlobalVariables.h"                                                            // Import Global Variables
MS5611           ms5611;                                                                // Barometric sensor object
DataPoint        bufferArray[10];                                                       // Circular buffer for initial data
int              bufferIndex                            = 0;                            // Next insertion index
bool             bufferIsFull                           = false;                        // Buffer full flag
float            referencePressure                      = 0.0F;                         // Baseline pressure at startup
float            previousAltitude                       = 0.0F;                         // Last altitude reading
float            previousAltitudeFiltered               = 0.0f;
long             previousPressure                       = 0L;                           // Last pressure reading
unsigned long    resetCountdownStart                    = 0UL;                          // Timestamp for post-descent logging
SystemState      currentState                           = SystemState::BUFFERING;       // Initial state

// CONTINGENCY GLOBALS
bool             contingencyMode                        = false;                        // Flag for sensor glitch detection
unsigned long    contingencyStart                       = 0UL;                          // Timestamp when contingency began
unsigned long    contingencySaveStart                   = 0UL;                          // Timestamp to start timed logging after deploy

// On every reboot, read and log the hardware reset cause. That tells you if the last run ended due to a watchdog, brown-out, panic, or external reset.
void reportResetReason()
{
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.printf("Last reset reason: %d\n", reason);
    // Optionally map codes to human-readable strings:
    // ESP_RST_POWERON, ESP_RST_BROWNOUT, ESP_RST_WDT, ESP_RST_PANIC, etc.
}

// FUNCTION DECLARATIONS
void initSPIFFS     ();
void saveDataPoint  (const DataPoint &pt);
void flushBuffer    ();
void addToBuffer    (const DataPoint &pt);

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(100);

    // Mount the SPIFFS file system
    initSPIFFS();
    ms5611.enableMedianFilter();

    // Configure LEDs and deploy pin
    pinMode(PIN_GREEN_LED, OUTPUT);   digitalWrite(PIN_GREEN_LED, LOW);
    pinMode(PIN_RED_LED,   OUTPUT);   digitalWrite(PIN_RED_LED,   LOW);
    pinMode(PIN_BUZZER,    OUTPUT);   digitalWrite(PIN_BUZZER,    LOW);

    // Initialize deploy pins
    // Set all deploy pins to OUTPUT and LOW initially
    for (size_t i = 0; i < NUM_DEPLOY_PINS; ++i) {
        pinMode(DEPLOY_PINS_DROUGE[i], OUTPUT); digitalWrite(DEPLOY_PINS_DROUGE[i], LOW);
        pinMode(DEPLOY_PINS_MAIN[i],   OUTPUT); digitalWrite(DEPLOY_PINS_MAIN[i],   LOW);        
    }

    /*/TESTING DEPLOY PINS
    for (size_t ch = 0; ch < DEPLOY_PINS_DROUGE; ++ch) {
        digitalWrite(DEPLOY_PINS[ch], LOW);
    }*/

    // Initialize barometric sensor
    if (!ms5611.begin()) {
        Serial.println("MS5611 initialization failed!");
        while (true) delay(1000); // Halt if sensor unresponsive
    }

    delay(1000); // Allow sensor to stabilize

    // Capture baseline pressure and altitude
    referencePressure           = ms5611.readPressure   ();
    previousAltitude            = ms5611.getAltitude    (referencePressure, referencePressure);
    previousAltitudeFiltered    = ms5611.medianFilter   (previousAltitude);
    previousPressure            = referencePressure;

    
    WiFi.persistent         (false);      // Disable saving any Wi-Fi configuration to Flash so we can update the SSID and Password
    WiFi.softAPdisconnect   (true); // Teardown AP if already up and clear its config

    // Bring up the soft-AP:
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, 1, false);
    delay(100);
    Serial.printf("Wi-Fi AP \"%s\" started; UDP on %s:%u\n", WIFI_SSID, broadcastIP.toString().c_str(), UDP_PORT);

    // Open UDP on that port:
    udp.begin(UDP_PORT);
}

// This is where the fun begins!
// The main loop runs every 300 ms, reads the MS5611 sensor, and manages the state machine.
// MAIN LOOP //
void loop() {
    //ms5611.spikeDetection(true); //Enable the spike detection algorithm of the sensor's library 

    static unsigned long        lastTime    = 0UL;
    unsigned long               now         = millis();

    // Enforce fixed sampling interval
    if ((now - lastTime) < LOOP_INTERVAL_MS) 
        return;

    unsigned long dt = now - lastTime;
    lastTime         = now;

    // READ RAW SENSOR DATA
    // This is where we read the sensor data and calculate altitude.
    float temp              = ms5611.readTemperature    ();                                     // C
    long  pres              = ms5611.readPressure       ();                                        // Pa
    float alt               = ms5611.getAltitude        (pres, referencePressure);                  // m
    float altFilterd        = ms5611.medianFilter       (alt);                                     // Apply Kalman filter to smooth altitude

    // DERIVED TELEMETRY
    float               verticalSpeed       = ((altFilterd - previousAltitudeFiltered) * 1000.0F) / dt;     // Getting the vertical speed in m/s
    float               pRate               = ((pres - previousPressure) * 1000.0F) / dt;                   // Getting the pressure rate in Pa/s
    float               tempK               = temp + 273.15F;                                               // Getting the temperature in Kelvin
    constexpr float     R                   = 287.05F;                                                      // The specific gas constant for dry air in J/(kg·K)
    float               density             = pres / (R * tempK);                                           // kg/m³, using the ideal gas law
    float               dynP                = 0.5F * density * verticalSpeed * verticalSpeed;               // Dynamic pressure in Pascals
    constexpr float     gamma               = 1.4F;                                                         // Ratio of specific heats (k)
    float               aSound              = sqrtf(gamma * R * tempK);                                     // Speed of sound in m/s
    float               mach                = verticalSpeed / aSound;                                       // Calculating the Mach number

    // SENSOR GLITCH DETECTION
    if (!contingencyMode && fabs(altFilterd - previousAltitudeFiltered) >= MAX_ALT_JUMP) {
        contingencyMode         = true;     // Trigger contingency mode if altitude jump exceeds threshold
        contingencyStart        = now;      // Start the contingency timer
        contingencySaveStart    = now;      // Start the save contingency timer
        redBlinking             = true;     // Make the red led blink
        
        Serial.println("* Contingency mode activated due to spike! *");
    }

    // Update previous for next iteration
    previousAltitudeFiltered    = altFilterd;
    previousPressure            = pres;

    // Build the current data point:
    // Brace-Initialization (Uniform initialization) for an aggregated data type, provides one value for each member of the DataPoint.
    // Brace syntax does it all in one, at compile time, and prevents unintended narrowing conversions.
    DataPoint pt = {
        temp, 
        pres, 
        altFilterd,
        verticalSpeed, 
        pRate, 
        density,
        dynP, 
        mach,
        static_cast<uint8_t>(currentState),
        contingencyMode,
        now
    };

    addToBuffer(pt);       // Store in circular buffer
    //saveDataPoint(pt);     // Append to file

    // Format into JSON (or CSV); here’s JSON for clarity
    char payload[200];
    int n = snprintf(payload, sizeof(payload),
        "T:%.2f°C P:%ldPa Alt:%.2fm v:%.2fm/s pR:%.2fPa/s ρ:%.4fkg/m³ q:%.2fPa M:%.3f S:%u C:%u t:%lusec CNCT:%u\n State:%f",
        pt.temperature,
        pt.pressure,
        pt.altitude,
        pt.verticalSpeed,
        pt.pressureRate,
        pt.density,
        pt.dynamicPressure,
        pt.mach,
        pt.stateID,
        pt.contingencyFlag ? 1U : 0U,
        pt.timestamp/1000,
        WiFi.softAPgetStationNum(),
        FlightState

    );

    // Send packets as UDP broadcast
    udp.beginPacket (broadcastIP, UDP_PORT);
    udp.write       ((uint8_t*)payload, n);
    udp.endPacket   ();

    // State machine logic: this handles the different states of the system, such as transitions AND actions in each state. 
    // This is where we handle the state transitions and actions based on the altitude readings.
    // The state machine is designed to handle the different states of the system, such as buffering, saving, and stopping.
    switch (currentState) {

        // THE BUFFERING STATE (PREPARATION FOR SAVING)
        case SystemState::BUFFERING:
            // Wait until rocket reaches arm altitude
            if ((altFilterd >= ALT_ARM_THRESHOLD) /*&& (verticalSpeed > 2.0)*/) {
                digitalWrite(PIN_GREEN_LED, HIGH);
                Serial.println("==> ARMED: altitude >= 3 m");
                flushBuffer();     // Save buffered datax   
                
                saveDataPoint(pt); // Save the current data point
                
                FlightState++;
                currentState = SystemState::ARMED;
            }
            break;

        // THE ARMED STATE (READY FOR FLIGHT)
        case SystemState::ARMED: //(NORMAL FLIGHT)
            FlightState++;
            // Transition immediately to flight
            digitalWrite(PIN_GREEN_LED, HIGH);
            digitalWrite(PIN_RED_LED,   HIGH);
            Serial.println("==> FLIGHT: continuous logging");
            flushBuffer();     // Save buffered data
            saveDataPoint(pt); // Save the current data point
            currentState = SystemState::FLIGHT;
            break;

        // THE FLIGHT STATE (NORMAL FLIGHT LOGGING)
        case SystemState::FLIGHT:
            FlightState++;
            if (!contingencyMode && (altFilterd >= ALT_DEPLOY_THRESHOLD) /*&& (verticalSpeed < 2.0)*/) {
                // LEDs & Logging (same as before)
                digitalWrite(PIN_GREEN_LED, LOW);
                digitalWrite(PIN_RED_LED,   LOW);
                
                greenBlinking   = true;
                redBlinking     = true;
                
                flushBuffer();
                saveDataPoint(pt);

                // DROGUE DEPLOYMENT: fire once at apogee, pulse pins
                if (!drogueDeployed && !drogueFiring) {
                    Serial.println("==> DEPLOY: deploying DROGUE at apogee");

                    for (size_t ch = 0; ch < sizeof(DEPLOY_PINS_DROUGE)/sizeof(int); ++ch) {
                        digitalWrite(DEPLOY_PINS_DROUGE[ch], HIGH);
                    }

                    drogueFiring    = true;
                    drogueFireStart = millis();
                    apogeeAltitude  = altFilterd; // Store apogee for main deploy logic
                }

                // DROGUE OFF: turn off after pulse
                if (drogueFiring && ((millis() - drogueFireStart) >= FIRE_DURATION_MS)) {
                    for (size_t ch = 0; ch < sizeof(DEPLOY_PINS_DROUGE)/sizeof(int); ++ch) {
                        digitalWrite(DEPLOY_PINS_DROUGE[ch], LOW);
                    }
                    drogueFiring    = false;
                    drogueDeployed  = true;
                    Serial.println("==> DROGUE OFF: pins reset after pulse.");
                }

                // MAIN DEPLOYMENT: fire once, 350m below apogee
                if (drogueDeployed && !mainDeployed && !mainFiring
                    && (altFilterd <= (apogeeAltitude - mainDeploymentAltitude))) {
                    Serial.println("==> MAIN DEPLOY: deploying MAIN parachute");

                    for (size_t ch = 0; ch < sizeof(DEPLOY_PINS_MAIN)/sizeof(int); ++ch) {
                        digitalWrite(DEPLOY_PINS_MAIN[ch], HIGH);
                    }
                    mainFiring      = true;
                    mainFireStart   = millis();
                }

                // MAIN OFF: turn off after pulse
                if (mainFiring && ((millis() - mainFireStart) >= FIRE_DURATION_MS)) {
                    for (size_t ch = 0; ch < sizeof(DEPLOY_PINS_MAIN)/sizeof(int); ++ch) {
                        digitalWrite(DEPLOY_PINS_MAIN[ch], LOW);
                    }

                    mainFiring      = false;
                    mainDeployed    = true;

                    Serial.println("==> MAIN OFF: pins reset after pulse.");
                }

                // Change state after full deployment
                if (drogueDeployed && mainDeployed) {
                    currentState = SystemState::DEPLOYED_NORMAL;
                }
            }
            break;


        // THE CONTINGENCY COUNTDOWN STATE (WAITING FOR EJECTION)
        case SystemState::CONTINGENCY_COUNTDOWN:
            // Start the timer only once
            if (contingencyStart == 0) {
                contingencyStart = millis();
                Serial.println("==> CONTINGENCY COUNTDOWN: timer started.");
            }

            // 1. Deploy DROGUE after TIME_BASED_EJECTION_MS
            if (!contingencyDrogueDeployed && !contingencyDrogueFiring &&
                ((millis() - contingencyStart) >= TIME_BASED_EJECTION_MS)) {

                Serial.println("==> CONTINGENCY DEPLOY: firing DROGUE charge(s)");
                for (size_t ch = 0; ch < sizeof(DEPLOY_PINS_DROUGE)/sizeof(int); ++ch) {
                    digitalWrite(DEPLOY_PINS_DROUGE[ch], HIGH);
                }

                contingencyDrogueFiring     = true;
                contingencyDrogueFireStart  = millis();
            }

            // 1a. DROGUE OFF after pulse
            if (contingencyDrogueFiring && ((millis() - contingencyDrogueFireStart) >= FIRE_DURATION_MS)) {
                for (size_t ch = 0; ch < sizeof(DEPLOY_PINS_DROUGE)/sizeof(int); ++ch) {
                    digitalWrite(DEPLOY_PINS_DROUGE[ch], LOW);
                }
                
                contingencyDrogueFiring     = false;
                contingencyDrogueDeployed   = true;
                Serial.println("==> CONTINGENCY DROGUE OFF: pins reset after pulse.");
            }

            // 2. Wait 5 seconds after drogue, then deploy MAIN
            if (contingencyDrogueDeployed && !contingencyMainDeployed && !contingencyMainFiring &&
                ((millis() - (contingencyDrogueFireStart + FIRE_DURATION_MS)) >= CONTINGENCY_WAIT_MS)) {

                Serial.println("==> CONTINGENCY DEPLOY: firing MAIN charge(s)");
                for (size_t ch = 0; ch < sizeof(DEPLOY_PINS_MAIN)/sizeof(int); ++ch) {
                    digitalWrite(DEPLOY_PINS_MAIN[ch], HIGH);
                }

                contingencyMainFiring       = true;
                contingencyMainFireStart    = millis();
            }

            // 2a. MAIN OFF after pulse
            if (contingencyMainFiring && ((millis() - contingencyMainFireStart) >= FIRE_DURATION_MS)) {
                for (size_t ch = 0; ch < sizeof(DEPLOY_PINS_MAIN)/sizeof(int); ++ch) {
                    digitalWrite(DEPLOY_PINS_MAIN[ch], LOW);
                }

                contingencyMainFiring       = false;
                contingencyMainDeployed     = true;

                Serial.println("==> CONTINGENCY MAIN OFF: pins reset after pulse.");
            }

            // After both deployed, transition state
            if (contingencyDrogueDeployed && contingencyMainDeployed) {
                currentState        = SystemState::CONTINGENCY_SAVING;
                contingencyStart    = 0; // Reset for future use (if needed)
            }
            break;


        // THE CONTINGENCY SAVING STATE (LOGGING AFTER EJECTION)
        case SystemState::CONTINGENCY_SAVING:
        
            // Continue logging for defined period
            flushBuffer();     // Save buffered data
            saveDataPoint(pt); // Save the current data point

            if ((now - contingencySaveStart) >= CONTINGENCY_SAVE_PERIOD) {
                Serial.println("==> CONTINGENCY SAVE END: stopping logging");
                currentState = SystemState::STOPPED;
            }
            break;

        // THE DEPLOYED NORMAL STATE (AFTER NORMAL DEPLOY)
        case SystemState::DEPLOYED_NORMAL:
            FlightState++;

            flushBuffer();     // Save buffered data
            saveDataPoint(pt); // Save the current data point
            
            // After normal deploy, wait for descent below reset threshold
            if (altFilterd <= ALT_RESET_THRESHOLD) {
                resetCountdownStart = now;
                Serial.println("==> RESET COUNTDOWN: descent detected");
                currentState = SystemState::RESET_COUNTDOWN;
            }
            break;

        // THE RESET COUNTDOWN STATE (LOGGING AFTER DESCENT)
        case SystemState::RESET_COUNTDOWN:
            FlightState++;
            
            // Final logging period after landing
            if ((now - resetCountdownStart) >= RESET_SAVE_PERIOD) {
                Serial.println("==> RESET COMPLETE: stopping logging");
                saveDataPoint(pt);
                currentState = SystemState::STOPPED;
            }
            break;

        // THE STOPPED STATE (FINAL STATE)
        // This is the final state where we stop all logging and keep the red LED ON.
        case SystemState::STOPPED:
            FlightState++;
            // After reset countdown (green off, red static):
            digitalWrite(PIN_GREEN_LED, LOW);
            greenBlinking = false;
            //greenLedState = false;
            
            redBlinking   = false;
            digitalWrite(PIN_RED_LED, HIGH);
            
            //redLedState   = true;

            // Ensure that the deployments charges are turned offs
            // for (size_t ch = 0; ch < DEPLOY_PINS_DROUGE; ++ch) {
            //         digitalWrite(DEPLOY_PINS[ch], LOW);
            // }
                
            break;
        }

    // Blinking led handler
    if ((now - lastBlinkTime) >= BLINK_INTERVAL) {
        lastBlinkTime = now;

        // Toggle red if it's in blink mode
        if (redBlinking) {
            redLedState = !redLedState;
            digitalWrite(PIN_RED_LED, redLedState);
        }

        // Toggle green if it's in blink mode
        if (greenBlinking) {
            greenLedState = !greenLedState;
            digitalWrite(PIN_GREEN_LED, greenLedState);
        }
    }

    // PRINTING THE PARAMETERS IN REAL=TIME FOR OBSERVATION
    Serial.printf(
        "T:%.2f°C P:%ldPa Alt:%.2fm v:%.2fm/s pR:%.2fPa/s ρ:%.4fkg/m³ q:%.2fPa M:%.3f S:%u C:%u t:%lusec CNCT:%u\n State:%f",
        pt.temperature,
        pt.pressure,
        pt.altitude,
        pt.verticalSpeed,
        pt.pressureRate,
        pt.density,
        pt.dynamicPressure,
        pt.mach,
        pt.stateID,
        pt.contingencyFlag ? 1U : 0U,
        pt.timestamp/1000,
        WiFi.softAPgetStationNum(),
        FlightState
    );
}

// HELPER FUNCTIONS //
// Initialize and mount SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    while (true) delay(1000);
  }
  Serial.println("SPIFFS mounted successfully");
}

// Append one DataPoint to telemetry file
void saveDataPoint(const DataPoint &pt) {
  File f = SPIFFS.open(SPIFFS_FILENAME, FILE_APPEND);

  if (!f) {
    Serial.println("Failed to open data file");
    return;
  }
    // Write formatted telemetry data to SPIFFS file:
    // Fields written (in order):
    // 1. temperature         ("%.2f") = degrees Celsius
    // 2. pressure            ("%ld")  = Pascals
    // 3. altitude            ("%.2f") = meters
    // 4. verticalSpeed       ("%.2f") = meters per second
    // 5. pressureRate        ("%.2f") = Pascals per second
    // 6. density             ("%.4f") = kilograms per cubic meter
    // 7. dynamicPressure     ("%.2f") = Pascals (0.5 * rho * v^2)
    // 8. mach                ("%.3f") = Mach number (v / speed of sound)
    // 9. stateID             ("%u")   = encoded flight state enum
    // 10. contingencyFlag    ("%u")   = 0 = normal, 1 = contingency active
    // 11. timestamp          ("%lu")  = milliseconds since startup
    // 12. connected stations ("%u")  = number of connected Wi-Fi stations
    // This is a CSV-like format for easy parsing later.
    f.printf(
        "%.2f %ld %.2f %.2f %.2f %.4f %.2f %.3f %u %u %lu %u\n",
        pt.temperature,             // degrees Celsius
        pt.pressure,                // Pascals
        pt.altitude,                // meters
        pt.verticalSpeed,           // m/s
        pt.pressureRate,            // Pa/s
        pt.density,                 // kg/m^3
        pt.dynamicPressure,         // Pascals
        pt.mach,                    // Mach
        pt.stateID,                 // flight state
        pt.contingencyFlag ? 1 : 0, // contingency mode flag
        pt.timestamp,                // ms since startup
        WiFi.softAPgetStationNum()
    );

    f.close();
}

// Flush circular buffer to SPIFFS then reset
void flushBuffer() {
    int count = bufferIsFull ? 10 : bufferIndex;
    for (int i = 0; i < count; ++i) saveDataPoint(bufferArray[i]);
        bufferIndex  = 0;
        bufferIsFull = false;
}

// Add a telemetry point to the circular buffer
void addToBuffer(const DataPoint &pt) {
    if (!bufferIsFull) {
        bufferArray[bufferIndex++] = pt;
    if (bufferIndex >= 10) bufferIsFull = true;
    } else {
        // Shift left and insert newest at end
        memmove(bufferArray, bufferArray + 1, sizeof(DataPoint) * 9);
        bufferArray[9] = pt;
    }

}
