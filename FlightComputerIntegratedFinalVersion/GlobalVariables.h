#ifndef GLOBALVARIABLES_H
#define GLOBALVARIABLES_H

// BROADCAST A Wi-Fi access point for telemetry data
static const char * const               WIFI_SSID                       = "HANGGANG SA DULO Flight Computer Telemetry Wi-Fi";
static const char * const               WIFI_PASSWORD                   = "HSDGRP09";

// CONSTANTS & THRESHOLDS 
// Altitude thresholds (in meters)
static constexpr float                  ALT_ARM_THRESHOLD               = 10.0F;         // Altitude to arm ejection system
static constexpr float                  ALT_CONTINGENCY_START           = 100.0F;        // Altitude to begin contingency countdown
static constexpr float                  ALT_DEPLOY_THRESHOLD            = 5.0F;       // Normal deployment altitude
static constexpr float                  ALT_RESET_THRESHOLD             = 50.0F;         // Altitude below which to reset and stop logging
static constexpr float                  MAX_ALT_JUMP                    = 30.0F;        // Maximum realistic jump between readings

// Timing intervals (in milliseconds)
static constexpr unsigned long          LOOP_INTERVAL_MS                = 300UL;        // Main loop period rotation
static constexpr unsigned long          CONTINGENCY_SAVE_PERIOD         = 120000UL;     // Logging period after contingency deploy
static constexpr unsigned long          RESET_SAVE_PERIOD               = 3000UL;       // Logging period after reset on descent
static constexpr unsigned long          TIME_BASED_EJECTION_MS          = 12000UL;       // Contingency timer duration
                                                                                        // based on the previous flight data of 
                                                                                        // the previous rocket launches (8-10m)

// PIN ASSIGNMENTS
static constexpr int                    PIN_GREEN_LED                   = 19;           // LED turns on when armed
static constexpr int                    PIN_RED_LED                     = 18;           // LED on during flight after arming
static constexpr int                    PIN_BUZZER                      = 4;            // (unused for now, but reserved)

// PIN ASSIGNMENTS FOR DEPLOYMENT CHARGES
static constexpr int                    DEPLOY_PINS_DROUGE[]            = { 32, 33 };   // Array of deployment pins for charges Drouge
static constexpr int                    DEPLOY_PINS_MAIN[]              = { 27, 13};     // Array of deployment pins for charges Main
static constexpr size_t                 NUM_DEPLOY_PINS                 = (sizeof(DEPLOY_PINS_DROUGE) + sizeof(DEPLOY_PINS_MAIN))/sizeof(DEPLOY_PINS_DROUGE[0]);            // Number of deploy pins
//static constexpr size_t                 NUM_DEPLOY_PINs_MAIN            = 2;
int                                     deploymentChargeDrouge          = 0;            // Deployment charge pin (not used in this version, but reserved)
uint8_t                                 mainDeploymentAltitude          = 300;            // Descent deployment altitude (in meters) for the main parachute
uint8_t                                 DeploymentAltitudeMain          = 0.0;            // Drouge deployment altitude (in meters)
// For timing the charge activation
static bool                             drogueDeployed                  = false;
static bool                             drogueFiring                    = false;
static unsigned long                    drogueFireStart                 = 0;

static bool                             mainDeployed                    = false;
static bool                             mainFiring                      = false;
static unsigned long                    mainFireStart                   = 0;

static float                            apogeeAltitude                  = 0.0f;  // To store apogee for main deployment

static constexpr unsigned long          FIRE_DURATION_MS                = 500; // Duration for firing pyro pin
static bool                             contingencyDrogueDeployed       = false;
static bool                             contingencyDrogueFiring         = false;
static unsigned long                    contingencyDrogueFireStart      = 0;

static bool                             contingencyMainDeployed         = false;
static bool                             contingencyMainFiring           = false;
static unsigned long                    contingencyMainFireStart        = 0;
static constexpr unsigned long          CONTINGENCY_WAIT_MS             = 5000; // Wait 5s between drogue and main


// LED BLINKERS GLOBAL VARIABLES
static constexpr unsigned long          BLINK_INTERVAL                  = 500UL;        // blink toggle every 500 ms
unsigned long                           lastBlinkTime                   = 0UL;          // Tracks timing for the blink toggles
int                                     FlightState                     = 0;

#endif