# ESP32 Robot ESP-NOW

**Wireless Robot Receiver for ESP32**

Robot firmware that receives commands from the controller over ESP-NOW, drives
two DC motors, and shows animated eyes on an OLED display. A fail-safe stops the
motors when the link is lost.

![Robot](../images/3.jpg)

## Project Information

- **Author:** Fabio Souza
- **License:** MIT License
- **Version:** 1.3

## Features

- Animated robot eyes that react to movement (idle, forward, fast, backward, left, right, link lost)
- ESP-NOW receiver with automatic peer discovery
- PWM motor control for differential drive
- Fail-safe: motors stop after 500 ms without a command
- Telemetry broadcast every 1 s (used by the controller to show link status)
- Laugh animation when the link is restored

## Hardware Requirements

- **Dual-core ESP32** board (e.g. ESP32-WROOM-32 / D1 mini ESP32).
  ESP32-S2, ESP32-C3 and other single-core chips are not supported: the eye
  animation runs in a FreeRTOS task pinned to core 1.
- **MX1508** dual H-bridge motor driver (or similar with IN1–IN4 inputs)
- **2x N20 DC gear motors** in differential drive
- **SH1106 OLED** display (128x64, I2C, address 0x3C)
- **5 V USB power bank** — powers the ESP32 (USB) and the motor driver

![Robot Schematic](../images/schematic.png)

## Pinout Reference

| Function | GPIO | Notes |
| --- | --- | --- |
| OLED SDA | 22 | |
| OLED SCL | 21 | |
| Left motor forward (IN1) | 33 | PWM |
| Left motor backward (IN2) | 27 | PWM |
| Right motor forward (IN3) | 32 | PWM |
| Right motor backward (IN4) | 4 | PWM |

Note that SDA/SCL are swapped compared with the usual ESP32 default (SDA 21, SCL 22).

## Dependencies

- **Arduino-ESP32 core 3.x** (`esp32:esp32`) — required for `ledcAttach()`
- [ESPNowProtocol](https://github.com/FBSeletronica/ESPNowProtocol) — clone into `~/Arduino/libraries`
- Adafruit GFX Library, Adafruit BusIO, Adafruit SH110X — Library Manager
- FluxGarage RoboEyes — Library Manager

```bash
cd ~/Arduino/libraries
git clone https://github.com/FBSeletronica/ESPNowProtocol.git
```

## Wireless Protocol

| Item | Value |
| --- | --- |
| Robot node ID | 2 |
| Controller node ID | 1 |
| `MSG_CONTROL` (1) | Received from the controller, `{cmd, speed}` |
| `MSG_TELEMETRY` (2) | Broadcast every 1 s, `{status = 1}` |

| Value | Command | Motor action |
| --- | --- | --- |
| 0 | CMD_STOP | Both motors stopped |
| 1 | CMD_FORWARD | Both motors forward |
| 2 | CMD_BACKWARD | Both motors backward |
| 3 | CMD_LEFT | Left backward, right forward (pivot) |
| 4 | CMD_RIGHT | Left forward, right backward (pivot) |

## Firmware Architecture

| Context | Runs on | Responsibility |
| --- | --- | --- |
| `onData()` | WiFi task, core 0 | Stores the latest control packet and its timestamp (inside a critical section) |
| `loop()` | Arduino loop task, core 1 | Executes the stored command, runs the fail-safe, sends telemetry |
| `faceAnimationTask()` | FreeRTOS task, core 1 | Applies the eye expression and calls `roboEyes.update()` every 10 ms |

Motors are only driven from `loop()`, so the ESP-NOW callback and the fail-safe
never write the PWM outputs at the same time.

## Face Animation

| State | When | Expression |
| --- | --- | --- |
| Boot | First 1.2 s | Laugh animation |
| Idle | Stopped | Random eye movement and blinking |
| Forward slow | Forward, speed < 150 | Looking up |
| Forward fast | Forward, speed ≥ 150 | Angry, wider eyes |
| Backward | Backward | Looking down, taller eyes |
| Left / Right | Turning | Looking toward the turn, from the robot's point of view (so a person facing the robot sees the eyes go to the opposite side) |
| Timeout | No command for 500 ms | Tired, with vertical flicker |
| Recovery | First command after a timeout | Laugh animation for 0.8 s |

## Motor Control

- **PWM:** 1 kHz, 8-bit (0–255)
- **Speed range from the controller:** 50–200. If the motors do not start at the
  lowest speed, raise `MIN_SPEED` in the controller firmware.

## Build and Upload

- **Board:** ESP32 Dev Module (`esp32:esp32:esp32`) or your ESP32 board

```bash
arduino-cli compile -u -p /dev/ttyUSB0 -b esp32:esp32:esp32 .
```

Measured with core 3.3.8: 72% of flash, 14% of RAM.

## Testing

1. Power on the robot: the eyes play the laugh animation, then show the tired face
   (no controller yet). Serial prints `Robot ready (v1.3)`.
2. Power on the controller: after peer discovery the eyes change to idle.
3. Direction buttons move the robot and change the eyes.
4. Turn the controller off: within 500 ms the motors stop, the eyes show the tired
   face and the serial prints `Fail-safe: link lost`.

## Troubleshooting

### Robot does not respond to commands

- Check the node IDs: robot 2, controller 1.
- Wait a few seconds after power-up for peer discovery.
- Check the serial log at 115200 baud.

### OLED display not working

- Check the I2C address (0x3C) with an I2C scanner.
- Check SDA/SCL on GPIO 22/21 (swapped compared with the ESP32 default).
- The serial port prints `Falha ao iniciar display SH1106` if the display does not respond.

### Motors not moving

- Check the power bank output and the motor driver power wiring.
- Check the IN1–IN4 connections against the pinout above.
- Try a higher speed: small motors may not start at low PWM duty.

## License

This project is licensed under the MIT License — see [LICENSE](../LICENSE).
