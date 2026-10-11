# ESP32 ESP-NOW Controller

**Wireless Remote Controller for Franzininho WiFi (ESP32-S2)**

Handheld controller firmware for the Franzininho WiFi LAB01. It sends movement
commands to the robot over ESP-NOW and shows command, speed, signal strength,
and link status on the OLED display.

![Controller and robot](../images/2.jpg)

## Project Information

- **Author:** Fabio Souza
- **License:** MIT License
- **Version:** 1.3

## Features

- Six-button interface (directional pad + speed control) with debounced speed buttons
- ESP-NOW with automatic peer discovery (no MAC address configuration)
- Commands sent immediately on change and repeated every 100 ms as keep-alive
- OLED status screen: command, speed, RSSI, link status (redrawn only on change)
- Link loss detection: `LINK: LOST` after 3 s without telemetry from the robot

## Hardware Requirements

- **Franzininho WiFi LAB01** (ESP32-S2), which already includes:
  - SSD1306 OLED display (128x64, I2C, address 0x3C)
  - Push buttons used as directional pad and A/B

![Franzininho WiFi LAB01](https://docs.franzininho.com.br/assets/images/Franzininho-wifi-lab01-desenho-392517059b55459854f015253728930c.png)

## Pinout Reference

| Function | GPIO | Notes |
| --- | --- | --- |
| OLED SDA | 8 | |
| OLED SCL | 9 | |
| UP button | 7 | Active LOW, internal pull-up |
| LEFT button | 6 | Active LOW, internal pull-up |
| RIGHT button | 5 | Active LOW, internal pull-up |
| DOWN button | 4 | Active LOW, internal pull-up |
| Button A (speed +) | 2 | Active LOW, internal pull-up |
| Button B (speed −) | 3 | Active LOW, internal pull-up |

## Dependencies

- **Arduino-ESP32 core 3.x** (`esp32:esp32`)
- Libraries, all from the Arduino Library Manager:
  [ESPNowProtocol](https://github.com/FBSeletronica/ESPNowProtocol) (1.2.1 or later),
  Adafruit GFX Library, Adafruit SSD1306, Adafruit BusIO

```bash
arduino-cli lib install "ESPNowProtocol" "Adafruit GFX Library" "Adafruit SSD1306" "Adafruit BusIO"
```

## Wireless Protocol

```
┌──────────────┐   MSG_CONTROL {cmd, speed}   ┌───────────┐
│  Controller  ├────────────────────────────→ │   Robot   │
│  (Node 1)    │                              │  (Node 2) │
│              │ ←────────────────────────────┤           │
└──────────────┘   MSG_TELEMETRY {status}     └───────────┘
```

### Control payload

```cpp
typedef struct {
  uint8_t cmd;      // CMD_STOP..CMD_RIGHT
  uint8_t speed;    // 50-200, step 10
} control_t;
```

| Value | Command | Description |
| --- | --- | --- |
| 0 | CMD_STOP | No direction button pressed |
| 1 | CMD_FORWARD | UP |
| 2 | CMD_BACKWARD | DOWN |
| 3 | CMD_LEFT | LEFT |
| 4 | CMD_RIGHT | RIGHT |

If more than one direction button is pressed, the priority is UP > DOWN > LEFT > RIGHT.

### Timing

| Parameter | Value | Description |
| --- | --- | --- |
| Transmission | `protocol.send()`, no ACK | The state is resent periodically, so retries are not needed |
| Keep-alive interval | 100 ms | Unchanged command is resent at this rate |
| Link timeout | 3000 ms | No telemetry → `LINK: LOST` |
| Button sampling | ~50 Hz (20 ms loop) | |
| Debounce (A/B) | 50 ms | |

The robot stops its motors if it receives no command for 500 ms, so the
keep-alive interval must stay well below that.

## Build and Upload

- **Board:** Franzininho WiFi (`esp32:esp32:franzininho_wifi_esp32s2`)

```bash
arduino-cli compile -u -p /dev/ttyACM0 -b esp32:esp32:franzininho_wifi_esp32s2 .
```

In the Arduino IDE: open `control_espnow.ino`, select **Tools → Board → Franzininho WiFi**,
select the port and upload.

Measured with core 3.3.8: 71% of flash, 21% of RAM.

## Testing

1. Power on controller and robot.
2. The OLED shows the Franzininho logo for 2 s, then the status screen.
3. Direction buttons change `CMD`; A/B change `SPD`.
4. When the robot telemetry arrives, `LINK: OK` and `RSSI` update.
   Peer discovery can take a few seconds after power-up.

## Troubleshooting

### OLED display not working

- Check the I2C address (0x3C) with an I2C scanner.
- Check SDA/SCL on GPIO 8/9.
- The serial port prints `Falha ao iniciar display SSD1306` if the display does not respond.
  The controller keeps working without the display.

### Robot not receiving commands

- Check that the robot firmware is running (serial: `Robot ready`).
- Wait a few seconds after power-up for peer discovery.
- Check the node IDs: controller 1, robot 2.
- Move the devices closer and watch the RSSI value.

## License

This project is licensed under the MIT License — see [LICENSE](../LICENSE).
