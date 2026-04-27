# ESP32 Robort ESP-NOW 

**Wireless Robot Receiver for ESP32-based Platforms**

A complete ESP-NOW based robot firmware that receives wireless commands from a controller, controls motors via PWM, and displays animated robot eyes on an OLED screen. Features fail-safe mechanisms, telemetry feedback, and synchronized eye animations that respond to movement commands.

## Project Information

- **Author:** Fabio Souza
- **License:** MIT License


## Features

- Animated robot eyes with multiple expressions (idle, forward, backward, left, right, timeout)
- ESP-NOW wireless receiver with automatic peer discovery
- PWM motor control for differential drive robots
- Fail-safe mechanism with 500ms command timeout
- Telemetry transmission every 1 second
- Face recovery animations on connection restoration
- Built for ESP32 boards with dual-core processing

## Hardware Requirements

### Microcontroller
- **ESP32-based board** (any ESP32 with ESP-NOW support)

### Display
- **SH1106 OLED Display** (128x64 pixels)
- **Interface:** I2C (TWI)
- **Address:** 0x3C

### Motors and Drivers
- **L298N Dual Motor Driver** or compatible
- **Two DC motors** (differential drive configuration)
- **Power supply:** Appropriate for motors (typically 6-12V)

## Pinout Reference

### GPIO Configuration

| Function | GPIO Pin | Notes |
| --- | --- | --- |
| OLED SDA (I2C Data) | 22 | Pull-up required (internal) |
| OLED SCL (I2C Clock) | 21 | Pull-up required (internal) |
| Motor Left Forward (IN1) | 33 | PWM output |
| Motor Left Backward (IN2) | 27 | PWM output |
| Motor Right Forward (IN3) | 32 | PWM output |
| Motor Right Backward (IN4) | 4 | PWM output |

## Dependencies

This project requires the following libraries. Install them via the Arduino Library Manager:

### Required Libraries

1. **ESPNowProtocol** ⭐
   - **Author:** Fabio Souza
   - **GitHub:** https://github.com/FBSeletronica/ESPNowProtocol
   - **Purpose:** Reliable ESP-NOW wireless communication with automatic retry
   - **Installation:** Clone or download from the GitHub repository

2. **Adafruit SH110X**
   - **Author:** Adafruit Industries
   - **Purpose:** OLED display driver for SH1106/SH1107
   - **Install via:** Arduino Library Manager (search: "Adafruit SH110X")

3. **FluxGarage RoboEyes**
   - **Author:** FluxGarage
   - **Purpose:** Animated robot eyes library
   - **Install via:** Arduino Library Manager (search: "FluxGarage RoboEyes")

### Installation Steps

```bash
# 1. Install ESPNowProtocol library via Arduino Library Manager
# Sketch → Include Library → Manage Libraries
# Search for "ESPNowProtocol" and install the latest version

# 2. Install other dependencies via Arduino IDE
# Sketch → Include Library → Manage Libraries
# Search for and install: Adafruit SH110X, FluxGarage RoboEyes
```

## Wireless Protocol Overview

This project implements the robot side of a two-way ESP-NOW communication system.

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
| Controller | 1 | Sender of control commands |
| Robot (this device) | 2 | Receiver of control commands |

### Message Types

| ID | Name | Direction | Purpose |
| --- | --- | --- | --- |
| 1 | MSG_CONTROL | Controller → Robot | Movement commands with speed |
| 2 | MSG_TELEMETRY | Robot → Controller | Status/acknowledgment feedback |

### Control Payload Structure

```cpp
typedef struct {
  uint8_t cmd;      // Movement command (see below)
  uint8_t speed;    // Motor speed (0-255)
} control_t;
```

### Movement Commands

| Value | Command | Description | Motor Action |
| --- | --- | --- | --- |
| 0 | CMD_STOP | Halt all movement | Both motors stop |
| 1 | CMD_FORWARD | Move forward | Both motors forward |
| 2 | CMD_BACKWARD | Move backward | Both motors backward |
| 3 | CMD_LEFT | Turn left | Left backward, right forward |
| 4 | CMD_RIGHT | Turn right | Left forward, right backward |

## Build Configuration

### Arduino IDE Settings

The project is configured for ESP32 boards:

- **Board Package:** `esp32:esp32`
- **Board:** Any ESP32 board (e.g., `esp32dev`, `esp32doit-devkit-v1`)
- **Upload Port:** `/dev/ttyUSB0` or `/dev/ttyACM0` (your specific COM port)
- **Upload Speed:** 921600 baud

### VS Code / PlatformIO

If you're using VS Code with the Arduino extension, configuration is already available in:
- [.vscode/arduino.json](./.vscode/arduino.json) - IDE settings
- [.vscode/c_cpp_properties.json](./.vscode/c_cpp_properties.json) - IntelliSense configuration

## Getting Started

### 1. Prerequisites
- Arduino IDE 1.8.x or later (or VS Code with Arduino extension)
- USB cable for board programming
- ESP32 board installed in Arduino IDE

### 2. Install Libraries
```bash
# Option A: Use Arduino Library Manager (GUI)
# Sketch → Include Library → Manage Libraries
# Search and install: Adafruit SH110X, FluxGarage RoboEyes

# Option B: Clone ESPNowProtocol manually
cd ~/Arduino/libraries
git clone https://github.com/FBSeletronica/ESPNowProtocol.git
```

### 3. Setup and Upload
1. Open [robot_espnow.ino](./robot_espnow.ino) in Arduino IDE
2. Verify board selection: **Tools → Board → [your ESP32 board]**
3. Verify port selection: **Tools → Port → /dev/ttyUSB0** (or your port)
4. Click **Upload** (or press Ctrl+U)
5. Wait for "Done uploading" message

### 4. Hardware Assembly
1. Connect SDA (GPIO 22) and SCL (GPIO 21) to OLED display
2. Connect motor driver inputs to GPIO pins (33, 27, 32, 4)
3. Connect motors to driver outputs
4. Ensure proper power distribution (3.3V for ESP32, motor voltage for drivers/motors)

### 5. Test the Robot
1. Power on both robot and controller
2. Robot should display boot animation with laugh
3. When controller sends commands:
   - Motors should respond to movement commands
   - Eyes should animate according to direction and speed
   - On timeout (no commands for 500ms), eyes show tired expression and motors stop

## Face Animation System

The robot features animated eyes that respond to movement commands:

- **Boot:** Curiosity expression with laugh animation
- **Idle:** Random eye movement and blinking
- **Forward Slow:** Normal expression looking forward
- **Forward Fast:** Angry expression with larger eyes
- **Backward:** Looking backward with taller eyes
- **Left/Right:** Looking in opposite direction of turn
- **Timeout:** Tired expression when connection lost

## Motor Control

- **PWM Frequency:** 1 kHz
- **Resolution:** 8-bit (0-255 speed range)
- **Driver:** L298N dual H-bridge
- **Configuration:** Differential drive (tank steering)

## Troubleshooting

### Robot Doesn't Respond to Commands

- **Check:** ESPNowProtocol is installed correctly
- **Check:** Node IDs match (robot = 2, controller = 1)
- **Check:** Both devices have unique but compatible MAC addresses
- **Check:** Signal strength - move devices closer together
- **Solution:** Check serial logs for error messages

### OLED Display Not Working

- **Check:** I2C address is 0x3C (verify with I2C scanner)
- **Check:** SDA/SCL connections on correct pins (22/21)
- **Check:** Display has proper 3.3V and GND connections
- **Solution:** Adjust I2C pull-up resistors if needed

### Motors Not Moving

- **Check:** Motor driver has correct power supply
- **Check:** Motor connections to driver outputs
- **Check:** PWM pins are correctly assigned
- **Solution:** Test motor driver independently first

## Technical Details

### Memory Usage

- **Flash:** ~45% (program code + assets)
- **RAM:** ~20% (buffers + variables + FreeRTOS tasks)
- **PROGMEM:** Eye animation bitmaps stored in flash

### Performance Characteristics

- **Command Processing:** <10ms latency
- **Face Animation:** 100 FPS (10ms update interval)
- **Telemetry Rate:** 1 Hz (every 1 second)
- **Fail-safe Timeout:** 500ms
- **PWM Frequency:** 1 kHz

### Code Organization

- **Header comments:** Comprehensive documentation
- **Sections:** Hardware config, protocol, motor control, face animation
- **FreeRTOS:** Dual-core operation (animation on core 1, main loop on core 0)
- **Inline comments:** Strategic explanations for complex logic

## Contributing

Contributions are welcome! Please feel free to:

1. Report issues or bugs
2. Suggest improvements or features
3. Submit pull requests with enhancements
4. Improve documentation

## License

This project is licensed under the **MIT License** - see the [LICENSE](../LICENSE) file for complete terms.

### Summary
- ✅ **Commercial use:** Permitted
- ✅ **Modification:** Permitted
- ✅ **Distribution:** Permitted
- ✅ **Private use:** Permitted
- ⚠️ **Liability:** No warranty provided
- ⚠️ **Attribution:** License and copyright notice must be included

## References

- **ESP32 Documentation:** https://docs.espressif.com/
- **ESPNowProtocol Library:** https://github.com/FBSeletronica/ESPNowProtocol
- **Adafruit Libraries:** https://github.com/adafruit
- **FluxGarage RoboEyes:** https://github.com/FluxGarage/FluxGarage-RoboEyes

## Support

For questions or issues with this project:

1. Check the [Troubleshooting](#troubleshooting) section
2. Review the inline code comments in [robot_espnow.ino](./robot_espnow.ino)
3. Consult the ESPNowProtocol library documentation
4. Open an issue on GitHub

---

**Author:** Fabio Souza  
**Last Updated:** 2026-04-26  
