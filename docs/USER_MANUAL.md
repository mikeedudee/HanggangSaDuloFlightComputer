# HANGGANG SA DULO FLIGHT COMPUTER: USER MANUAL

**Document ID:** HSD-UM-001  
**Version:** 1.1.0 (Flight Ready)  
**Applicability:** Ground Control & Recovery Teams  
**System:** ESP32 / MS5611 Avionics Package

---

## **1. SAFETY WARNINGS (READ FIRST)**

> [!IMPORTANT]
> **⚠️ DANGER: EXPLOSIVES HAZARD**
> This device controls pyrotechnic charges. Failure to follow safety protocols can result in severe injury or death.

1.  **NO USB WITH CHARGES:** NEVER connect the flight computer to USB (computer) while pyrotechnic charges (e-matches) are connected to the screw terminals.
2.  **POWER ON SEQUENCE:** Always power on the flight computer **BEFORE** arming the ejection switches (if using a mechanical arming switch).
3.  **MINIMUM STANDOFF:** Maintain a minimum 5-meter standoff distance when the system is powered and armed on the ground.
4.  **BROWNOUT RISK:** Do not use 9V "transistor" batteries. Use only high-current 2S (7.4V) LiPo batterY.

---

## **2. SYSTEM OVERVIEW**

The HANGGANG SA DULO Flight Computer is an autonomous avionics system designed to manage the ascent and recovery of high-power rockets.

* **Primary Sensor:** MS5611 Barometer (Altitude resolution: ~10cm).
* **Recovery Logic:** Dual Deployment (Drogue at Apogee, Main at 300m AGL).
* **Failsafe:** Automatic "Contingency Deployment" if sensors fail during flight (12-second timer).
* **Telemetry:** Broadcasts flight data via a localized Wi-Fi Hotspot.

---

## **3. LED STATUS INDICATORS ("BEEP CODES")**

The flight computer communicates its state via two LEDs. Memorize these patterns.

| State | Green LED | Red LED | Description | Action Required |
| :--- | :--- | :--- | :--- | :--- |
| **BUFFERING** | **OFF** | **OFF** | System Booting / Ground Idle. | Keep rocket vertical. Wait for lock. |
| **ARMED** | **ON (Solid)** | **OFF** | Ready to Launch (>10m detected). | Clear the launch pad. |
| **FLIGHT** | **OFF** | **ON (Solid)** | Ascent detected. Logging active. | Monitor telemetry. |
| **DEPLOY** | **OFF** | **BLINKING** | Ejection charges firing. | Visual confirmation of chutes. |
| **ERROR** | **OFF** | **FAST BLINK** | Sensor/Hardware Failure. | **DO NOT LAUNCH.** Disarm immediately. |
| **STOPPED** | **OFF** | **ON (Solid)** | Landed & Safe. | Recovery team may approach. |

##  **4. DEFAULT CONFIGURATIONS**

### Pinout Configuration

| Pin | Function | Description |
| :--- | :--- | :--- |
| **19** | `PIN_GREEN_LED` | **ARMED** Indicator (Steady ON) |
| **18** | `PIN_RED_LED` | **FLIGHT/ERROR** Indicator (Blinking) |
| **4** | `PIN_BUZZER` | Audio Beacon (Reserved) |
| **32, 33** | `DEPLOY_PINS_DROGUE` | Drogue Ejection Charge (Active HIGH) |
| **27, 13** | `DEPLOY_PINS_MAIN` | Main Ejection Charge (Active HIGH) |
| **21 (SDA)** | I2C SDA | MS5611 Data |
| **22 (SCL)** | I2C SCL | MS5611 Clock |

### Thresholds and Timing

> [!NOTE]
> All runtime parameters live in `GlobalVariables.h`. Update these in accordance with your flight mission.

| Parameter | Default | Meaning |
|---|---:|---|
| `ALT_ARM_THRESHOLD_M` | 10 m | Arming altitude. Below this value, ejection is inhibited. |
| `ALT_DEPLOY_THRESHOLD_M` | 5 m | Nominal (low‑altitude) deployment threshold (use with caution). |
| `ALT_RESET_THRESHOLD_M` | 50 m | Reset/logging stop threshold during descent. |
| `MAX_ALT_JUMP_M` | 30 m | Reject unrealistically large altitude jumps. |
| `LOOP_INTERVAL_MS` | 300 ms | Main loop cadence. |
| `TIME_BASED_EJECTION_MS` | 12,000 ms | Contingency ejection delay once started. |
| `FIRE_DURATION_MS` | 500 ms | Pyro firing pulse duration per channel. |
| `CONTINGENCY_WAIT_MS` | 5,000 ms | Gap between contingency drogue and main fires. |
| `CONTINGENCY_SAVE_PERIOD_MS` | 120,000 ms | Log period after contingency deployment. |
| `RESET_SAVE_PERIOD_MS` | 3,000 ms | Log period after reset on descent. |

### Wi-Fi Configuration

| Parameter | Default Value |
|---|---:|
| `WIFI_SSID` | HANGGANG SA DULO Telemetry |
| `WIFI_PASSWORD` | HSDGRP09 |
| `UDP_PORT` | 4210 |
| `BROADCAST_IP` | 192, 168, 4, 255 |

>[!NOTE]
> **Main deployment setpoint**: `MAIN_DEPLOY_ALT_AGL_M = 300` meters (configure to your vehicle’s needs).
> Likewise, in the Wi-Fi parameters, you change to your liking.

> [!IMPORTANT]
> **⚠️ WARNING:** Ensure your deployment hardware (MOSFETs/Relays) is **Active HIGH**. If using Active LOW relays, the firing logic in `GlobalVariables.h` must be inverted, or charges will fire on boot.

---

## **5. PRE-FLIGHT CHECKLIST**

### **Phase I: Bench Prep (Lab)**
* [ ] **Battery Check:** LiPo voltage is >3.8V per cell (e.g., >7.6V for 2S).
* [ ] **Storage Check:** SPIFFS flash memory is cleared.
* [ ] **Wiring Check:** Deployment charges connected to correct terminals:
    * **DROGUE:** Pins 32, 33
    * **MAIN:** Pins 27, 13
* [ ] **Mounting:** Board allows airflow to the barometer but blocks direct wind/light.

### **Phase II: Pad Setup (Field)**
1.  [ ] **Rail Loading:** Load rocket onto the launch rail.
2.  [ ] **Power On:** Turn on the Flight Computer.
3.  [ ] **Connectivity:** Ground Station operator connects to Wi-Fi SSID: `HANGGANG SA DULO Telemetry`.
4.  [ ] **Verification:** Check Ground Station dashboard:
    * Altitude reads ~0m (Relative).
5.  [ ] **Wait for Idle:** Ensure LEDs are **OFF** (Buffering Mode).
6.  [ ] **Arming:** Close the mechanical arming switch (if equipped).
7.  [ ] **Clearance:** Clear the area.

---

## **6. IN-FLIGHT PROCEDURES**

### **Monitoring Telemetry**
The Ground Station (Python/Mobile App) will receive UDP packets containing:
`T:28.50 P:101325 A:15.40 V:45.20 M:0.13 S:1 C:0 T:12500`

* **A (Altitude):** Must increase rapidly.
* **S (State):** Watch for transition from `0` (Buffer) -> `1` (Armed) -> `2` (Flight).
* **C (Contingency):** If `1`, the system has detected a sensor glitch and is running on backup timer.

### **Emergency Situations**
* **Rocket fails to launch:** Wait 15 minutes before approaching (Hang-fire safety).
* **Drogue fails to deploy:** System will attempt Main deployment at 300m automatically.
* **No Telemetry:** The system operates autonomously. Loss of Wi-Fi does **not** affect recovery.

---

**© 2025 HANGGANG SA DULO Project Team**
