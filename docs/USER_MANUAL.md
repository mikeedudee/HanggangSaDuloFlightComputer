# HANGGANG SA DULO FLIGHT COMPUTER: USER MANUAL

**Document ID:** HSD-UM-001  
**Version:** 1.0 (Flight Ready)  
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
4.  **BROWNOUT RISK:** Do not use 9V "transistor" batteries. Use only high-current 2S (7.4V) or 3S (11.1V) LiPo batteries.

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

---

## **4. PRE-FLIGHT CHECKLIST**

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

## **5. IN-FLIGHT PROCEDURES**

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
