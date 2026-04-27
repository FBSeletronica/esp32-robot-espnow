# ESP32 ESP-NOW Controller

**Wireless Remote Controller for Franzininho WiFi (ESP32-S2)**

A complete ESP-NOW based handheld controller sketch for the Franzininho WiFi development board. This project implements a reliable wireless communication system to control a robot with real-time feedback display.

## Project Information

- **Author:** Fabio Souza
- **License:** MIT License
- **Version:** 1.0

## Features

- Six-button gamepad interface (directional pad + speed control)
- ESP-NOW wireless protocol with automatic peer discovery
- Real-time OLED display with:
  - Current movement command
  - Motor speed level
  - Signal strength indicator (RSSI)
  - Wireless connection status
- Reliable message delivery with automatic retransmission
- Low-latency command transmission (~100ms minimum interval)
- Connection monitoring with 3-second timeout detection

## Hardware Requirements

### Microcontroller
- **Franzininho WiFi** (ESP32-S2 based development board)

### Display
- **SSD1306 OLED Display** (128x64 pixels)
- **Interface:** I2C (TWI)
- **Address:** 0x3C

### Input Devices
- 6x Push buttons (momentary)
- Internal pullup resistors (GPIO configured as INPUT_PULLUP)

![Controller Pinout Diagram](https://docs.franzininho.com.br/assets/images/Franzininho-wifi-lab01-desenho-392517059b55459854f015253728930c.png)

## Pinout Reference

### GPIO Configuration

| Function | GPIO Pin | Notes |
| --- | --- | --- |
| OLED SDA (I2C Data) | 8 | Pull-up required (internal) |
| OLED SCL (I2C Clock) | 9 | Pull-up required (internal) |
| UP Button | 7 | Active LOW, internal pullup |
| LEFT Button | 6 | Active LOW, internal pullup |
| RIGHT Button | 5 | Active LOW, internal pullup |
| DOWN Button | 4 | Active LOW, internal pullup |
| Button A (Speed+) | 2 | Active LOW, internal pullup |
| Button B (Speed-) | 3 | Active LOW, internal pullup |

## Dependencies

This project requires the following libraries. Install them via the Arduino Library Manager:

### Required Libraries

1. **ESPNowProtocol** ⭐
   - **Author:** Fabio Souza
   - **GitHub:** https://github.com/FBSeletronica/ESPNowProtocol
   - **Purpose:** Reliable ESP-NOW wireless communication with automatic retry
   - **Installation:** Clone or download from the GitHub repository

2. **Adafruit GFX Library**
   - **Author:** Adafruit Industries
   - **Purpose:** Graphics primitives for OLED display
   - **Install via:** Arduino Library Manager (search: "Adafruit GFX")

3. **Adafruit SSD1306**
   - **Author:** Adafruit Industries
   - **Purpose:** OLED display driver
   - **Install via:** Arduino Library Manager (search: "Adafruit SSD1306")

4. **Adafruit BusIO**
   - **Author:** Adafruit Industries
   - **Purpose:** I2C/SPI abstraction layer
   - **Install via:** Arduino Library Manager (search: "Adafruit BusIO")

### Installation Steps

```bash
# 1. Install ESPNowProtocol library via Arduino Library Manager
# Sketch → Include Library → Manage Libraries
# Search for "ESPNowProtocol" and install the latest version

# 2. Install other dependencies via Arduino IDE
# Sketch → Include Library → Manage Libraries
# Search for and install: Adafruit GFX, Adafruit SSD1306, Adafruit BusIO
```

## Wireless Protocol Overview

This project implements a two-way ESP-NOW communication system between the controller and robot.

### Communication Architecture

```
┌──────────────┐  Control Message   ┌───────────┐
│  Controller  ├─────────────────→  │   Robot   │
│  (Node 1)    │                    │  (Node 2) │
│              │  ←─────────────────  │           │
└──────────────┘  Telemetry/Status  └───────────┘
```

### Network Configuration

| Entity | Node ID | Role |
| --- | --- | --- |
| Controller (this device) | 1 | Sender of control commands |
| Robot | 2 | Receiver of control commands |

### Message Types

| ID | Name | Direction | Purpose |
| --- | --- | --- | --- |
| 1 | MSG_CONTROL | Controller → Robot | Movement commands with speed |
| 2 | MSG_TELEMETRY | Robot → Controller | Status/acknowledgment feedback |

### Control Payload Structure

```cpp
typedef struct {
  uint8_t cmd;      // Movement command (see below)
  uint8_t speed;    // Motor speed (50-200 in 10-step increments)
} control_t;
```

### Movement Commands

| Value | Command | Description |
| --- | --- | --- |
| 0 | CMD_STOP | Halt all movement |
| 1 | CMD_FORWARD | Move forward |
| 2 | CMD_BACKWARD | Move backward |
| 3 | CMD_LEFT | Turn left |
| 4 | CMD_RIGHT | Turn right |

### Communication Parameters

| Parameter | Value | Description |
| --- | --- | --- |
| Transmission Mode | Reliable (with retry) | ESPNowProtocol::sendReliable() |
| Min Send Interval | 100 ms | Prevents unnecessary traffic |
| Link Timeout | 3000 ms | Connection lost detection threshold |
| Auto-Discovery | Enabled | Automatic peer detection |
| Display Rate | 50 Hz (20ms) | OLED update frequency |

## Build Configuration

### Arduino IDE Settings

The project is configured for the **Franzininho WiFi (ESP32-S2)** board:

- **Board Package:** `esp32:esp32`
- **Board:** `franzininho_wifi_esp32s2`
- **Upload Port:** `/dev/ttyACM0` (or your specific COM port)
- **Upload Speed:** 921600 baud

### VS Code / PlatformIO

If you're using VS Code with the Arduino extension, configuration is already available in:
- [.vscode/arduino.json](./.vscode/arduino.json) - IDE settings
- [.vscode/c_cpp_properties.json](./.vscode/c_cpp_properties.json) - IntelliSense configuration

## Getting Started

### 1. Prerequisites
- Arduino IDE 1.8.x or later (or VS Code with Arduino extension)
- USB cable for board programming
- Franzininho WiFi board installed in Arduino IDE

### 2. Install Libraries
```bash
# Option A: Use Arduino Library Manager (GUI)
# Sketch → Include Library → Manage Libraries
# Search and install: Adafruit GFX, Adafruit SSD1306, Adafruit BusIO

# Option B: Clone ESPNowProtocol manually
cd ~/Arduino/libraries
git clone https://github.com/FBSeletronica/ESPNowProtocol.git
```

### 3. Setup and Upload
1. Open [control_espnow.ino](./control_espnow.ino) in Arduino IDE
2. Verify board selection: **Tools → Board → franzininho_wifi_esp32s2**
3. Verify port selection: **Tools → Port → /dev/ttyACM0** (or your port)
4. Click **Upload** (or press Ctrl+U)
5. Wait for "Done uploading" message

### 4. Hardware Assembly
1. Connect SDA (GPIO 8) and SCL (GPIO 9) to OLED display
2. Connect the 6 buttons to their respective GPIO pins (see Pinout section)
3. Ensure proper power distribution (3.3V for all components)

### 5. Test the Controller
1. Power on both controller and robot
2. OLED should display splash screen, then show controller status
3. Press directional buttons - should see command change on display
4. Press A/B buttons - should see speed change on display
5. When robot responds with telemetry:
   - `LINK: OK` appears on display
   - RSSI value updates with signal strength

## Troubleshooting

### Controller Doesn't Respond

- **Check:** Serial port is correct (Tools → Port)
- **Check:** USB cable is properly connected
- **Check:** Board selection matches target hardware
- **Solution:** Try uploading at lower baud rate

### OLED Display Not Working

- **Check:** I2C address is 0x3C (verify with I2C scanner)
- **Check:** SDA/SCL connections on correct pins (8/9)
- **Check:** Display has proper 3.3V and GND connections
- **Solution:** Adjust I2C pull-up resistors if needed

### Robot Not Receiving Commands

- **Check:** ESPNowProtocol is installed correctly
- **Check:** Robot is powered on and running compatible firmware
- **Check:** Both devices have unique but compatible Node IDs
- **Check:** Signal strength - move devices closer together
- **Solution:** Check robot receiver logs for error messages

## Technical Details

### Memory Usage

- **Flash:** ~40% (program code + assets)
- **RAM:** ~15% (buffers + variables)
- **PROGMEM:** Franzininho logo bitmap stored in flash

### Performance Characteristics

- **Button Sampling:** 50 Hz (20ms interval)
- **Display Update:** 50 Hz (20ms interval)
- **Command Transmission:** 10 Hz min (100ms interval, throttled)
- **Latency:** ~20ms median (button press to transmission)
- **Range:** ~100m line-of-sight (typical ESP32 WiFi range)

### Code Organization

- **Header comments:** Comprehensive documentation with author, license, dependencies
- **Sections:** Clearly marked hardware config, protocol, variables, functions
- **Functions:** Detailed docstrings explaining purpose and parameters
- **Inline comments:** Strategic explanations for complex logic

## Contributing

Contributions are welcome! Please feel free to:

1. Report issues or bugs
2. Suggest improvements or features
3. Submit pull requests with enhancements
4. Improve documentation

## License

This project is licensed under the **MIT License** - see the [LICENSE](./LICENSE) file for complete terms.

### Summary
- ✅ **Commercial use:** Permitted
- ✅ **Modification:** Permitted
- ✅ **Distribution:** Permitted
- ✅ **Private use:** Permitted
- ⚠️ **Liability:** No warranty provided
- ⚠️ **Attribution:** License and copyright notice must be included

## References

- **Franzininho Project:** https://github.com/Franzininho
- **ESP32 Documentation:** https://docs.espressif.com/
- **ESPNowProtocol Library:** https://github.com/FBSeletronica/ESPNowProtocol
- **Adafruit Libraries:** https://github.com/adafruit

## Support

For questions or issues with this project:

1. Check the [Troubleshooting](#troubleshooting) section
2. Review the inline code comments in [control_espnow.ino](./control_espnow.ino)
3. Consult the ESPNowProtocol library documentation
4. Open an issue on GitHub

---

**Author:** Fabio Souza  
**Last Updated:** 2026-04-26  

