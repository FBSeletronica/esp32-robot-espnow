# ESP-NOW Robot Control System

A wireless robot control system using the ESP-NOW protocol for ESP32-based devices.
This repository includes two firmware projects:

- **control_espnow/**: handheld controller with OLED display and buttons
- **robot_espnow/**: robot receiver that executes movement commands

The system is designed for low-latency communication without relying on a traditional Wi-Fi network.

![ESP-NOW Robot Control System](images/1.jpg)

## Key Features

- 6-button interface for direction and speed control
- Reliable ESP-NOW protocol with automatic peer discovery
- OLED display showing command, speed, RSSI, and link status
- Bidirectional communication with telemetry feedback
- Low-latency command transmission with reliable retransmission
- Automatic link loss detection and timeout handling
- Compatible with Franzininho WiFi (ESP32-S2) and other ESP32 boards

## Hardware Requirements

### Controller

- [Franzininho WiFi LAB01](https://docs.franzininho.com.br/docs/franzininho-wifi-lab01/) (ESP32-S2)
- SSD1306 OLED display (128x64, I2C)
- 6x momentary push buttons

### Robot

- ESP32-based board compatible with ESP-NOW
- Motor drivers and actuators according to your hardware setup

![Robot Schematic](images/schematic.png)

## Software Dependencies

Install these libraries in the Arduino IDE or via Library Manager:

- **ESPNowProtocol**
- **Adafruit GFX Library**
- **Adafruit SSD1306**
- **Adafruit BusIO**
- **Adafruit SH110X**
- **FluxGarage RoboEyes**

Also install ESP32 board support in the Arduino IDE.

## Repository Structure

```
robo_espnow_lib/
├── control_espnow/
│   ├── control_espnow.ino
│   └── README.md
├── robot_espnow/
│   ├── robot_espnow.ino
│   └── README.md
├── LICENSE
└── README.md
```

## Getting Started

1. Open the Arduino IDE or VS Code with the Arduino extension.
2. Install ESP32 board support.
3. Install the libraries listed above.
4. Clone this repository:

```bash
git clone https://github.com/FBSeletronica/esp32_robot_espnow.git
cd esp32_robot_espnow

```


## Flash the firmware

### Controller

1. Open `control_espnow/control_espnow.ino`.
2. Select board: **Franzininho WiFi (ESP32-S2)**.
3. Select the correct port.
4. Upload the sketch.

### Robot

1. Open `robot_espnow/robot_espnow.ino`.
2. Select a compatible ESP32 board.
3. Upload the sketch.

## Testing the system

1. Power on both devices.
2. The controller should show the startup screen.
3. The display should show:
   - mode
   - current command
   - speed
   - RSSI
   - link status
4. Press direction buttons and use buttons A/B to change speed.
5. When the robot responds, the link should change to `OK`.

## Command quick reference

| Button | Command   | Value |
| ------ | --------- | ----- |
| UP     | Forward   | 1     |
| DOWN   | Backward  | 2     |
| LEFT   | Left      | 3     |
| RIGHT  | Right     | 4     |
| none   | Stop      | 0     |

### Speed control

- Minimum: 50
- Maximum: 200
- Step: 10
- Starting speed: 50

## Development

### Project architecture

The firmware is designed with a modular structure:

- Hardware abstraction: GPIO, I2C, OLED, and buttons
- Communication layer: ESPNowProtocol and message handling
- Application logic: command processing and speed control
- User interface: display rendering and input reading

### Code structure

1. `setup()` initializes the display, buttons, and ESP-NOW.
2. `loop()` reads buttons, determines the command, sends the message, and updates the state.
3. `onData()` processes telemetry received from the robot.

### Debugging

- Use `Serial.begin(115200)` and `Serial.println()` for debug output.
- Display values on the OLED when needed.
- Check `ESP.getFreeHeap()` for memory usage.
- Use an I2C scanner if the OLED does not appear.

### Test checklist

- [ ] Compiles without warnings
- [ ] Uploads successfully
- [ ] OLED shows the startup and status screens
- [ ] Buttons respond correctly
- [ ] Speed adjusts correctly
- [ ] RSSI updates with telemetry
- [ ] Link status changes to OK when connected
- [ ] Link timeout works
- [ ] No freezes during operation

## ESP-NOW communication

- Controller: Node ID 1
- Robot: Node ID 2
- Control message: `MSG_CONTROL`
- Telemetry message: `MSG_TELEMETRY`

## Additional documentation

- `control_espnow/README.md` — controller-specific documentation
- `robot_espnow/README.md` — robot-specific documentation

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
