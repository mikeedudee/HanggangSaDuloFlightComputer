# HANGGANG SA DULO FLIGHT COMPUTER

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP32-blue" alt="Platform: ESP32">
  <img src="https://img.shields.io/badge/Framework-Arduino%20Core-00979D?logo=arduino" alt="Arduino Core">
  <img src="https://img.shields.io/badge/Alt_Telemetry-WiFi%20SoftAP-2ea44f" alt="Wi‑Fi SoftAP">
  <img src="https://img.shields.io/badge/Language-C++%20(Arduino)-00599C?logo=c%2B%2B" alt="C++ (Arduino)">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-brightgreen.svg" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/Domain-Safety--Critical-%23d73a49" alt="Safety‑Critical">
</p>

An Arduino-compatible **flight computer** firmware for high-powered model rockets and CanSats, targeting **ESP32** (ESP32‑WROOM / DevKitC). The system manages arming, ascent/apogee detection, dual‑parachute deployment (drogue & main), and **telemetry via ESP32 Wi‑Fi SoftAP**. This code should also be compatible with other microcontrollers with a few tweaks.

> **Safety-critical**: This firmware controls energetic devices (pyrotechnic charges). Bench‑test with inert loads (e.g., lamps/resistors) and **remove all energetic materials** during development. Follow your local range safety code.

---

## Hardware Platform — ESP32

- Tested on **ESP32 Dev Module** (ESP32‑WROOM).  
- Logic level: **3.3 V**.  
- Use **external pyro drivers** (low‑side MOSFET + current‑rated supply or a dedicated pyro board). **Do not** drive e‑matches directly from the ESP32.

---

## Features (at a glance)

- **Altitude‑gated arming** and **time‑based contingency ejection**.
- **Dual‑channel pyro outputs** for both drogue and main (redundancy per chute).
- **Configurable deployment thresholds** (altitude and timers).
- **Telemetry Wi‑Fi AP** for ground connectivity (change the credentials before flight).
- **Status indicators** (green/red LEDs) and buzzer placeholder.
- Simple timing model with a fixed **main loop period** for predictable behavior.

---

## Default Configuration

All runtime parameters live in `GlobalVariables.h`. Update these for your vehicle before flight.

### Thresholds and Timing

| Parameter | Default | Meaning |
|---|---:|---|
| `ALT_ARM_THRESHOLD` | 10 m | Arming altitude. Below this value, ejection is inhibited. |
| `ALT_CONTINGENCY_START` | 100 m | Above this altitude, the contingency timer may start. |
| `ALT_DEPLOY_THRESHOLD` | 5 m | Nominal (low‑altitude) deployment threshold (use with caution). |
| `ALT_RESET_THRESHOLD` | 50 m | Reset/logging stop threshold during descent. |
| `MAX_ALT_JUMP` | 30 m | Reject unrealistically large altitude jumps. |
| `LOOP_INTERVAL_MS` | 300 ms | Main loop cadence. |
| `TIME_BASED_EJECTION_MS` | 12,000 ms | Contingency ejection delay once started. |
| `FIRE_DURATION_MS` | 500 ms | Pyro firing pulse duration per channel. |
| `CONTINGENCY_WAIT_MS` | 5,000 ms | Gap between contingency drogue and main fires. |
| `CONTINGENCY_SAVE_PERIOD` | 120,000 ms | Log period after contingency deployment. |
| `RESET_SAVE_PERIOD` | 3,000 ms | Log period after reset on descent. |

> **Main deployment setpoint**: `mainDeploymentAltitude = 300` meters (configure to your vehicle’s needs).

### Pin Map (default)

| Function | Pin(s) |
|---|---|
| Green LED (armed) | 19 |
| Red LED (in‑flight) | 18 |
| Buzzer (reserved) | 4 |
| Drogue pyro channels | 32, 33 |
| Main pyro channels | 27, 13 |

> Adjust pin numbers for your MCU and pyro interface board. Always verify continuity and firing polarity with inert tests.

### Telemetry Wi‑Fi

- The firmware exposes a Wi‑Fi Access Point. **Edit the SSID and password in `GlobalVariables.h`** before releasing any build publicly or flying.
- You may disable or relocate the telemetry stack if your operations require radio silence.

---

## Build & Flash

**Option A — Arduino IDE**  
1. Open `FlightComputerIntegratedFinalVersion.ino`.  
2. Select your target board and port.  
3. Place `GlobalVariables.h` alongside the sketch.  
4. Compile and upload.

**Option B — PlatformIO (VS Code)**  
1. Create a new project for your board/MCU.  
2. Add both source files to `src/`.  
3. Build and upload via the PlatformIO toolbar.

> Use an MCU and toolchain appropriate for your hardware (Teensy/ESP32/Arduino‑compatible). Verify I/O voltage levels and current capability for your pyro drivers.

---

## Operating Concept

1. **Initialization**: System boots, sensors and I/O initialize, LEDs indicate status.  
2. **Arming**: Once altitude exceeds `ALT_ARM_THRESHOLD`, ejection logic arms.  
3. **Ascent/Apogee**: The firmware monitors altitude trends and thresholds.  
4. **Deployment**:  
   - **Nominal**: Deploy per configured altitude thresholds and logic.  
   - **Contingency**: If conditions warrant, a timer triggers ejection after `TIME_BASED_EJECTION_MS`. Drogue fires, then main after `CONTINGENCY_WAIT_MS`.  
5. **Descent/Reset**: Below `ALT_RESET_THRESHOLD`, logging cadence reduces and the system returns to safe state.

> Verify your specific deploy logic in the code and adapt thresholds to your vehicle’s performance envelope and regulations.

---

## Ground Testing Checklist

- Replace e‑matches with **inert loads** (bulbs/resistors).  
- Confirm **LED and continuity** behavior.  
- Confirm **each pyro channel** fires for `FIRE_DURATION_MS`.  
- Verify **no firing** occurs below arming altitude.  
- Exercise **contingency timer** logic with simulated altitude data.  
- Test telemetry AP association and data link, or disable if not used.

---

## Safety Notes

- Comply with your range safety officer (RSO) and national codes.  
- Use **redundant power** and robust connectors for pyro and sensors.  
- Protect lines with **fuses** and fly‑back suppression where applicable.  
- Document and practice **Abort/Disarm procedures**.  
- Maintain a **clear separation** between flight code and ground‑only debug features.

---

## Contributing

- Use small, testable modules and defensive checks.  
- Prefer constant‑time loops and avoid blocking delays in the main loop.  
- Add unit or bench tests for state transitions and firing conditions.  
- Document any changes to thresholds/pins in the README and header.

---

## License

**MIT License**

Copyright (c) 2024–2025 HANGGANG SA DULO

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

---

## Acknowledgements

Built by the HANGGANG SA DULO Flight Team. Lessons learned are continually folded back into safer, clearer code and documentation.
