# HANGGANG SA DULO High-Powered Rocket Flight Computer

<p align="center">
  <img src="https://github.com/user-attachments/assets/70a5d529-9199-425e-9350-270b1d5ea5c9" width="45%" />
  <img src="https://github.com/user-attachments/assets/2a2643f3-505d-4e83-90fd-6f29317d1129" width="45%" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Version-1.1.0-blue" alt="Version">
  <img src="https://img.shields.io/badge/License-Open_Access-green" alt="License">
  <img src="https://img.shields.io/badge/Compliance-NASA_Power_of_Ten-red" alt="Standards">
  <br> <img src="https://img.shields.io/badge/Platform-ESP32-orange" alt="Platform">
  <img src="https://img.shields.io/badge/IDE-Arduino-00979D?logo=arduino&logoColor=white" alt="Arduino IDE">
  <img src="https://img.shields.io/badge/Editor-VS_Code-007ACC?logo=visual-studio-code&logoColor=white" alt="VS Code">
   <img src="https://img.shields.io/badge/Domain-Safety--Critical-%23d73a49" alt="Safety‑Critical">
   <img src="https://img.shields.io/badge/Alt_Telemetry-WiFi%20SoftAP-2ea44f" alt="Wi‑Fi SoftAP">
</p>

<p align="center">
  <a href="#flight-demonstration">Flight Demonstration</a> •
  <a href="#the-hanggang-sa-dulo-team">TEAM</a> •
  <a href="#overview">Overview</a> •
  <a href="#hardware-specifications">Hardware</a> •
  <a href="#software-architecture">Architecture</a> •
  <a href="#installation--setup">Installation</a> •
  <a href="#safety-warnings">Warnings</a> •
  <a href="#critical-incident-report">CIR</a> •
  <a href="#license">License</a>
</p>

<a id="flight-demonstration"></a>
## 🎥 Flight Demonstration
**[▶️ Click Here to Watch the Launch Video](https://drive.google.com/file/d/1ntdnOv3MPNH1HT6hlQWhRAF-xXCvTr7M/view?usp=sharing)**

---

<a id="the-hanggang-sa-dulo-team"></a>
## 👥 The HANGGANG SA DULO Team

**Project Core Team:**
* **Francis Mike John Camogao** ([GitHub](https://github.com/mikeedudee))
* **Jneil Dulo**
* **Mark Vincent Yap**
* **Chester Hahne Conde**
* **Ezralph Legara**

---

<a id="overview"></a>
## 📖 Overview

**STATUS:** Flight Ready (subject to HIL verification).

The **HANGGANG SA DULO Flight Computer** is a safety-critical avionics software suite designed for high-power rocketry missions. It is programmed to run on the **ESP32** microcontroller platform but can also be modified to be compatible with other microcontrollers (e.g., any Teensy or Arduino), utilizing the **MS5611** high-precision barometric sensor for altitude determination.

This system implements a deterministic **Finite State Machine (FSM)** to manage flight phases, from ground idling to dual-deployment recovery. It is built strictly following the **NASA Power of Ten** coding standards to ensure maximum reliability in harsh operating environments.

### 🆕 What's New in v1.1.0
* **Modular Architecture:** Codebase refactored into isolated modules (`Sensors`, `FlightLogic`, `GlobalVariables`) for improved maintainability.
* **Safety Upgrade:** Implemented Hardware Watchdog Timer (WDT) to prevent system freezes.
* **Incident Fix:** Added mandatory Active-Low initialization for Pyro channels to prevent premature firing (See *Critical Incident Report*).

### 🚀 Key Features

* **Dual Deployment Recovery:** Independent logic for **Drogue** (at Apogee) and **Main** parachutes (at 300m AGL).
* **Safety-Critical Architecture:**
    * **Watchdog Timer (WDT):** 5-second hardware timeout to auto-reset system freezes.
    * **Defensive I/O:** Active-Low initialization and strictly timed pyro firing (1000ms duration).
    * **Memory Safety:** Zero dynamic allocation (`malloc`/`new`) or `String` class usage to prevent heap fragmentation.
* **Fault Tolerance:**
    * **Glitch Detection:** Filters altitude spikes (>30m jump) to prevent false triggers.
    * **Contingency Mode:** Fallback timer-based deployment (12s) if sensors fail during ascent.
* **Telemetry:**
    * **Wi-Fi Broadcast:** UDP packets sent via Soft-AP ("HANGGANG SA DULO Telemetry").
    * **Black Box Logging:** Buffered writes to onboard Flash (SPIFFS) to capture high-speed descent data.

### Version 1.0.0
  * [You can download the original unrefactored and unclean version (click me)](https://github.com/mikeedudee/HanggangSaDuloFlightComputer/tree/v1.0.0).

---

<a id="hardware-specifications"></a>
## 🛠️ Hardware Specifications

The software is optimized for the following hardware configuration:

| Component | Specification | Role |
| :--- | :--- | :--- |
| **MCU** | ESP32-WROOM-32 | Central Processing Unit |
| **Sensor** | MS5611 (I2C) | Barometric Pressure & Temperature |
| **Storage** | SPIFFS (Onboard Flash) | Telemetry Data Logging |
| **Power** | 2S/3S LiPo | System Power |

### 🔌 Pinout Configuration

| Pin | Function | Description |
| :--- | :--- | :--- |
| **19** | `PIN_GREEN_LED` | **ARMED** Indicator (Steady ON) |
| **18** | `PIN_RED_LED` | **FLIGHT/ERROR** Indicator (Blinking) |
| **4** | `PIN_BUZZER` | Audio Beacon (Reserved) |
| **32, 33** | `DEPLOY_PINS_DROGUE` | Drogue Ejection Charge (Active HIGH) |
| **27, 13** | `DEPLOY_PINS_MAIN` | Main Ejection Charge (Active HIGH) |
| **21 (SDA)** | I2C SDA | MS5611 Data |
| **22 (SCL)** | I2C SCL | MS5611 Clock |

> [!IMPORTANT]
> **⚠️ WARNING:** Ensure your deployment hardware (MOSFETs/Relays) is **Active HIGH**. If using Active LOW relays, the firing logic in `GlobalVariables.h` must be inverted, or charges will fire on boot.

---

<a id="software-architecture"></a>
## ⚙️ Software Architecture

### Finite State Machine (FSM)
The system operates in a linear, non-recursive state machine:

1.  **BUFFERING:** Initial state. Sensors calibrating. Waiting for >10m altitude to ARM.
2.  **ARMED:** Rocket has crossed `ALT_ARM_THRESHOLD_M` (10m). Green LED ON.
3.  **FLIGHT:** Continuous logging. Monitoring for apogee.
4.  **DEPLOYED_NORMAL:** Drogue and Main have fired successfully. Waiting for landing.
5.  **CONTINGENCY_COUNTDOWN:** Sensor failure detected. Timer started for backup deployment.
6.  **CONTINGENCY_SAVING:** Logging post-failure data.
7.  **RESET_COUNTDOWN:** Landed detected (<50m). Saving final buffer.
8.  **STOPPED:** Mission complete. Logs saved. Red LED static.

### Standards Compliance
This repository adheres to **NASA "Power of Ten" coding standards**:
* **Visual Alignment:** All variable declarations are vertically aligned.
* **Modular Design:** Functionality separated into `Sensors.ino`, `FlightLogic.ino`, and `GlobalVariables.h`.
* **No Recursion:** Strict iterative logic.
* **Bounded Loops:** All `while` loops include timeout counters.

---

<a id="installation--setup"></a>
## 📥 Installation & Setup

### Prerequisites
1.  **Arduino IDE** (v2.0 or later)
2.  **ESP32 Board Manager** (Expressif Systems)

### Required Libraries
Install these via the Arduino Library Manager:
* `MS5611` I used my own custom MS5611 library for this one. You can download the library [here](https://github.com/mikeedudee/MS5611-Mike-Refactored).
* `WiFi` (Built-in)
* `FS` & `SPIFFS` (Built-in)
* `esp_task_wdt` (Built-in)

### Flashing Instructions
1.  Clone/Download this repository.
2.  **IMPORTANT:** Open the folder `HSD_Flight_Computer` and select `HSD_Flight_Computer.ino`.
3.  *Note: The Arduino IDE will automatically open the associated module tabs (`FlightLogic`, `Sensors`).*
4.  Connect your ESP32 via USB.
5.  Click **Select Board > ESP32**.
6.  Upload the code.

---

<a id="safety-warnings"></a>
## ⚠️ Safety Warnings

* **EXPLOSIVES HAZARD:** Do **NOT** connect e-matches or pyrotechnics while the board is connected to USB or during code upload.
* **TESTING:** Always perform a "Dry Run" (using LEDs instead of charges) after any code modification.
* **VOLTAGE:** Ensure the ESP32 is powered by a stable 3.3V source. Brownouts may trigger the Watchdog Timer (WDT) and reset the system mid-flight.
* **CALIBRATION:** The system calibrates ground pressure immediately on boot. Do not power on the system in a pressurized environment (e.g., inside a sealed avionics bay) until ready for launch.

---
<a id="critical-incident-report"></a>
## 📝 Critical Incident Report (Lessons Learned)

> [!IMPORTANT]
> **⚠️ INCIDENT: PREMATURE CHARGE FIRING**
> **Mission:** HANGGANG SA DULO (JUNE 2025)
> **Status:** RESOLVED (Successful Flight Achieved)

During the setup and initialization phase on the launchpad, our team experienced a **premature firing incident** where the ejection charges fired twice immediately upon powering on the flight computer.

**Root Cause:**
The issue was traced to an unchecked line of code in the `setup()` sequence. The GPIO pins controlling the MOSFETs were not explicitly initialized to their "SAFE" (LOW) state before the main loop began, or leftover simulation logic (toggling pins HIGH) was inadvertently left active.

<p align="center">
  <img src="https://github.com/user-attachments/assets/22998e9e-7481-4da3-9d6b-dce9e85b2dd8" alt="Incident Code Snippet" width="80%">
</p>

**The Fix (Mandatory Check):**
To prevent this, you **MUST** explicitly force all pyro pins to their `OFF` state (typically `LOW` for N-Channel MOSFETs) immediately inside the `setup()` function. Do not rely on the default floating state of the microcontroller.

**Correct Implementation:**
Ensure your `setup()` function contains this exact safety block before any other logic:

```cpp
// Pyro Safety Initialization
// CRITICAL: Ensure this is set to LOW (OFF) immediately on boot.
for (size_t i = 0; i < NUM_DROGUE_PINS; ++i) {
    pinMode(DEPLOY_PINS_DROGUE[i], OUTPUT);
    digitalWrite(DEPLOY_PINS_DROGUE[i], LOW);
}

for (size_t i = 0; i < NUM_MAIN_PINS; ++i) {
    pinMode(DEPLOY_PINS_MAIN[i], OUTPUT);
    digitalWrite(DEPLOY_PINS_MAIN[i], LOW);
}
```

---
<a id="license"></a>
## 📄 License

This project is released under a custom **Open Access License**.
See the [LICENSE](LICENSE) file for full details.

**Copyright © 2025 HANGGANG SA DULO Project Team**
