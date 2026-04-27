/*
 * ESP32 Robot ESP-NOW Control Firmware
 * 
 * Author: Fabio Souza
 * License: MIT
 * 
 * Description:
 * This sketch implements a robot control system using ESP-NOW protocol for wireless communication.
 * The robot features autonomous eye animation on an OLED display that responds to movement commands,
 * motor control via PWM, and a fail-safe mechanism with telemetry feedback.
 * 
 * Dependencies:
 * - ESPNowProtocol: Custom library for ESP-NOW wireless communication
 *   Repository: https://github.com/FBSeletronica/ESPNowProtocol
 * - Adafruit_SH110X: OLED display driver
 * - FluxGarage_RoboEyes: Animated robot eyes library
 * 
 * Features:
 * - Wireless control via ESP-NOW protocol
 * - Motor control with 8-bit PWM resolution
 * - Synchronized eye animation based on movement state
 * - Fail-safe mechanism with 500ms command timeout
 * - Telemetry transmission every 1 second
 * - Multiple face expressions (idle, forward, backward, left, right, timeout)
 * - Face recovery animation on connection restoration
 */

#include <ESPNowProtocol.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <FluxGarage_RoboEyes.h>

// --------------------------------------------------
// MOTOR PINS - L298N Dual Motor Driver Configuration
// These pins control two DC motors via PWM signals
// Motor Left: IN1 (forward), IN2 (backward)
// Motor Right: IN3 (forward), IN4 (backward)
// --------------------------------------------------

#define IN1 33  // Left motor forward
#define IN2 27  // Left motor backward
#define IN3 32  // Right motor forward
#define IN4 4   // Right motor backward

// --------------------------------------------------
// OLED Display Configuration - SH1106 128x64 I2C Display
// Used for displaying animated robot eyes that respond to movement commands
// --------------------------------------------------

#define OLED_SDA 22             // I2C SDA pin
#define OLED_SCL 21             // I2C SCL pin
#define OLED_I2C_ADDRESS 0x3C   // Default I2C address for SH1106
#define SCREEN_WIDTH 128        // Display width in pixels
#define SCREEN_HEIGHT 64        // Display height in pixels
#define OLED_RESET -1           // Reset pin (-1 = no reset pin)
#define OLED_ROTATION 2         // Display rotation (0-3)

// --------------------------------------------------
// PWM Configuration for Motor Control
// ESP32 LEDC (LED PWM Controller) settings
// --------------------------------------------------

#define PWM_FREQ 1000          // PWM frequency in Hz (1 kHz)
#define PWM_RESOLUTION 8       // Resolution in bits (0-255 range for speed values)

// --------------------------------------------------
// ESP-NOW Message Types
// Message IDs for distinguishing packet types in ESP-NOW communication
// --------------------------------------------------

#define MSG_CONTROL    1  // Robot control commands (motor and face state)
#define MSG_TELEMETRY  2  // Status feedback from robot

// --------------------------------------------------
// Motor Control Commands
// Command values sent via ESP-NOW protocol
// --------------------------------------------------

#define CMD_STOP      0  // Stop all motors
#define CMD_FORWARD   1  // Move forward
#define CMD_BACKWARD  2  // Move backward
#define CMD_LEFT      3  // Rotate left (pivot turn)
#define CMD_RIGHT     4  // Rotate right (pivot turn)

// --------------------------------------------------
// Communication Packet Structures
// ESP-NOW payload formats for control and telemetry
// --------------------------------------------------

/**
 * Control message structure
 * Sent from controller to robot via ESP-NOW
 * @param cmd Motor command (CMD_STOP, CMD_FORWARD, etc.)
 * @param speed Motor speed (0-255)
 */
typedef struct {
  uint8_t cmd;    // Motor command
  uint8_t speed;  // Speed value (0-255)
} control_t;

/**
 * Telemetry message structure
 * Sent from robot to controller via ESP-NOW
 * @param status Robot status (1 = operational)
 */
typedef struct {
  uint8_t status; // Robot status flag
} telemetry_t;

// --------------------------------------------------
// HARDWARE INTERFACES AND COMMUNICATION
// --------------------------------------------------

// ESP-NOW Protocol instance (custom library by Fabio Souza)
// Handles wireless communication via ESP-NOW protocol
ESPNowProtocol protocol;

// OLED Display and Eyes Animation
// Uses I2C interface for SH1106 128x64 display
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Animated robot eyes (uses FluxGarage_RoboEyes library)
RoboEyes<Adafruit_SH1106G> roboEyes(display);

// FreeRTOS task handle for face animation task
TaskHandle_t faceAnimationTaskHandle = nullptr;

// --------------------------------------------------
// STATE MANAGEMENT AND TIMING
// --------------------------------------------------

// Communication timing
unsigned long lastCommandTime = 0;  // Timestamp of last received command
unsigned long lastTelemetry = 0;    // Timestamp of last sent telemetry

// --------------------------------------------------
// Timing Configuration (milliseconds and thresholds)
// --------------------------------------------------

#define COMMAND_TIMEOUT 500              // Fail-safe timeout: stop if no command received
#define TELEMETRY_INTERVAL 1000          // Send telemetry every 1 second
#define FACE_BOOT_DURATION 1200          // Boot animation duration
#define FACE_RECOVERY_DURATION 800       // Recovery animation duration on reconnect
#define FACE_IDLE_SPEED_THRESHOLD 20     // Speed below which robot is considered idle
#define FACE_FAST_SPEED_THRESHOLD 150    // Speed threshold for angry face expression
#define ANGRY_SPEED_THRESHOLD 200        // (Currently unused - candidate for refactoring)

/**
 * Face animation states
 * Enum representing different eye expressions based on robot movement
 * Each state triggers specific animations and eye configurations
 */
enum FaceState : uint8_t
{
  FACE_BOOT = 0,       // Startup boot animation with curiosity
  FACE_IDLE,           // Idle state with random eye movement
  FACE_FORWARD_SLOW,   // Forward movement with normal expression
  FACE_FORWARD_FAST,   // Forward movement at high speed - angry expression
  FACE_BACKWARD,       // Backward movement
  FACE_LEFT,           // Left rotation movement
  FACE_RIGHT,          // Right rotation movement
  FACE_TIMEOUT         // Connection timeout - tired expression
};

// --------------------------------------------------
// FACE ANIMATION STATE VARIABLES
// Used by faceAnimationTask (runs on core 1) - marked volatile for thread safety
// --------------------------------------------------

// Display initialization flag
volatile bool faceDisplayReady = false;

// Target face state (updated by main loop, read by animation task)
volatile FaceState desiredFaceState = FACE_BOOT;

// Last received command and speed (for recovery animations)
volatile uint8_t lastControlCmd = CMD_STOP;
volatile uint8_t lastControlSpeed = 0;

// Trigger recovery animation when connection is restored after loss
volatile bool faceRecoveryPending = false;

// Communication link status (true when actively receiving commands)
bool hasControlLink = false;

// --------------------------------------------------
// FACE STATE MACHINE
// Resolution and application of animated eye expressions
// --------------------------------------------------

/**
 * Resolve the target face state based on motor command and speed
 * This function determines which eye expression the robot should display
 * 
 * @param cmd Motor command (CMD_STOP, CMD_FORWARD, CMD_BACKWARD, CMD_LEFT, CMD_RIGHT)
 * @param speed Motor speed value (0-255)
 * @return FaceState The appropriate face state for the given command
 * 
 * Logic:
 * - If stopped or below idle threshold (20) -> FACE_IDLE with random eye movement
 * - FORWARD command:
 *   - Speed >= 150 -> FACE_FORWARD_FAST (angry, increased eye width, flicker effect)
 *   - Speed < 150 -> FACE_FORWARD_SLOW (normal expression, looking forward)
 * - Other directions -> FACE_BACKWARD, FACE_LEFT, FACE_RIGHT
 */

FaceState resolveFaceState(uint8_t cmd, uint8_t speed)
{
  if (cmd == CMD_STOP || speed <= FACE_IDLE_SPEED_THRESHOLD)
  {
    return FACE_IDLE;
  }

  switch (cmd)
  {
    case CMD_FORWARD:
      if (speed >= FACE_FAST_SPEED_THRESHOLD)
      {
        return FACE_FORWARD_FAST;
      }
      return FACE_FORWARD_SLOW;

    case CMD_BACKWARD:
      return FACE_BACKWARD;

    case CMD_LEFT:
      return FACE_LEFT;

    case CMD_RIGHT:
      return FACE_RIGHT;

    case CMD_STOP:
    default:
      return FACE_IDLE;
  }
}

/**
 * Update the desired face state based on new command
 * Stores the command and calculates new face state via resolveFaceState()
 * 
 * @param cmd Motor command
 * @param speed Motor speed
 */
void updateFaceState(uint8_t cmd, uint8_t speed)
{
  lastControlCmd = cmd;
  lastControlSpeed = speed;
  desiredFaceState = resolveFaceState(cmd, speed);
}

/**
 * Apply face state to the robot's eyes
 * Configures all eye properties and animations for the given face state
 * Called by faceAnimationTask whenever the desired state changes
 * 
 * @param state The FaceState to apply
 * 
 * Each state configures:
 * - Eye mood (DEFAULT, ANGRY, TIRED, HAPPY)
 * - Eye position (N, S, E, W, NE, SE, NW, SW, DEFAULT)
 * - Idle mode animation (random eye movement)
 * - Eye dimensions (width, height, border radius)
 * - Flicker effects (horizontal/vertical tremor)
 * - Curiosity and autoblinker settings
 */
void applyFaceState(FaceState state)
{
  if (!faceDisplayReady)
  {
    return;
  }

  roboEyes.setWidth(36, 36);
  roboEyes.setHeight(36, 36);
  roboEyes.setBorderradius(8, 8);
  roboEyes.setSpacebetween(10);
  roboEyes.setCyclops(OFF);
  roboEyes.setCuriosity(OFF);
  roboEyes.setHFlicker(OFF, 0);
  roboEyes.setVFlicker(OFF, 0);

  switch (state)
  {
    case FACE_BOOT:
      roboEyes.setMood(DEFAULT);
      roboEyes.setPosition(DEFAULT);
      roboEyes.setIdleMode(OFF, 0, 0);
      roboEyes.setCuriosity(ON);
      // Boot state doesn't set autoblinker - handled in task startup
      break;

    case FACE_IDLE:
      roboEyes.setMood(DEFAULT);
      roboEyes.setPosition(DEFAULT);
      roboEyes.setCuriosity(ON);
      // Random eye repositioning every 2-4 seconds
      roboEyes.setIdleMode(ON, 2, 2);
      // Natural random blinking every 3-5 seconds
      roboEyes.setAutoblinker(ON, 3, 2);
      break;

    case FACE_FORWARD_SLOW:
      roboEyes.setMood(DEFAULT);
      roboEyes.setPosition(N);  // Looking forward
      roboEyes.setCuriosity(ON);
      roboEyes.setIdleMode(OFF, 0, 0);  // No random movement during movement
      roboEyes.setAutoblinker(OFF, 3, 2);  // No blinking during movement
      break;

    case FACE_FORWARD_FAST:
      roboEyes.setMood(ANGRY);  // Aggressive expression for high speed
      roboEyes.setPosition(N);  // Looking forward
      roboEyes.setCuriosity(ON);
      roboEyes.setIdleMode(OFF, 0, 0);
      roboEyes.setAutoblinker(OFF, 3, 2);
      roboEyes.setWidth(40, 40);  // Larger eyes for aggressive look
      break;

    case FACE_BACKWARD:
      roboEyes.setMood(DEFAULT);
      roboEyes.setPosition(S);  // Looking backward
      roboEyes.setCuriosity(ON);
      roboEyes.setIdleMode(OFF, 0, 0);
      roboEyes.setHeight(40, 40);  // Taller eyes
      roboEyes.setAutoblinker(OFF, 3, 2);
      break;

    case FACE_LEFT:
      roboEyes.setMood(DEFAULT);
      roboEyes.setPosition(E);  // Looking right (reverse of left motor command)
      roboEyes.setCuriosity(ON);
      roboEyes.setIdleMode(OFF, 0, 0);
      roboEyes.setAutoblinker(OFF, 3, 2);
      break;

    case FACE_RIGHT:
      roboEyes.setMood(DEFAULT);
      roboEyes.setPosition(W);  // Looking left (reverse of right motor command)
      roboEyes.setCuriosity(ON);
      roboEyes.setIdleMode(OFF, 0, 0);
      roboEyes.setAutoblinker(OFF, 3, 2);
      break;

    case FACE_TIMEOUT:
      roboEyes.setMood(TIRED);  // Tired/sad expression when connection lost
      roboEyes.setPosition(DEFAULT);
      roboEyes.setIdleMode(OFF, 0, 0);
      roboEyes.setVFlicker(ON, 1);  // Vertical tremor for tired effect
      break;
  }
}

/**
 * Initialize and configure the OLED display
 * Called once during face animation task startup
 * Sets up I2C communication, initializes SH1106 display, and applies boot face state
 */
void setupFaceDisplay()
{
  Wire.begin(OLED_SDA, OLED_SCL);
  delay(250);

  if (!display.begin(OLED_I2C_ADDRESS, true))
  {
    Serial.println("Falha ao iniciar display SH1106");
    return;
  }

  display.setRotation(OLED_ROTATION);
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  faceDisplayReady = true;
  applyFaceState(FACE_BOOT);

  Serial.println("Face OLED pronta");
}

/**
 * Face animation task - runs on core 1
 * This FreeRTOS task handles all eye animation updates and state transitions
 * 
 * Features:
 * - Boot animation with laugh sound on startup
 * - Transitions between face states with smooth animations
 * - Recovery animation when connection is restored
 * - 10ms update interval for smooth 100 FPS rendering
 * 
 * @param parameter Task parameter (unused)
 */
void faceAnimationTask(void *parameter)
{
  (void)parameter;

  setupFaceDisplay();

  FaceState appliedState = FACE_BOOT;
  unsigned long bootStartedAt = millis();
  unsigned long recoveryStartedAt = 0;
  bool recoveryActive = false;
  bool bootAnimationPlayed = false;

  for (;;)
  {
    unsigned long now = millis();

    if (!bootAnimationPlayed && faceDisplayReady)
    {
      roboEyes.anim_laugh();
      bootAnimationPlayed = true;
    }

    if (recoveryActive)
    {
      if (now - recoveryStartedAt >= FACE_RECOVERY_DURATION)
      {
        recoveryActive = false;
        applyFaceState((FaceState)desiredFaceState);
        appliedState = (FaceState)desiredFaceState;
      }
    }
    else if (faceRecoveryPending)
    {
      faceRecoveryPending = false;
      roboEyes.anim_laugh();
      recoveryStartedAt = now;
      recoveryActive = true;
    }
    else if (appliedState == FACE_BOOT && now - bootStartedAt >= FACE_BOOT_DURATION)
    {
      desiredFaceState = FACE_IDLE;
      applyFaceState(FACE_IDLE);
      appliedState = FACE_IDLE;
    }
    else if (appliedState != desiredFaceState)
    {
      applyFaceState((FaceState)desiredFaceState);
      appliedState = (FaceState)desiredFaceState;
    }

    roboEyes.update();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/**
 * Start the face animation task
 * Creates a FreeRTOS task pinned to core 1 with 4KB stack
 * Priority: 1 (medium priority)
 */
void startFaceAnimationTask()
{
  xTaskCreatePinnedToCore(
    faceAnimationTask,
    "faceAnimation",
    4096,
    nullptr,
    1,
    &faceAnimationTaskHandle,
    1
  );
}

// --------------------------------------------------
// PWM MOTOR CONTROL
// PWM configuration and motor speed functions
// --------------------------------------------------

/**
 * Initialize PWM for all four motor pins
 * Uses ESP32 LEDC (LED PWM Controller) with 1 kHz frequency and 8-bit resolution
 * This allows smooth speed control from 0-255
 */
void setupPWM()
{
  ledcAttach(IN1, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(IN2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(IN3, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(IN4, PWM_FREQ, PWM_RESOLUTION);
}

// --------------------------------------------------
// MOTOR CONTROL FUNCTIONS
// L298N driver functions for left and right motors
// --------------------------------------------------

/**
 * Set left motor speed and direction
 * 
 * @param speed Motor speed (-255 to 255)
 *             Positive: forward (IN1 high, IN2 low)
 *             Negative: backward (IN1 low, IN2 high)
 *             Zero: stop (both pins low)
 */

void setMotorLeft(int speed)
{
  speed = constrain(speed, -255, 255);

  if (speed > 0)
  {
    ledcWrite(IN1, speed);
    ledcWrite(IN2, 0);
  }
  else if (speed < 0)
  {
    ledcWrite(IN1, 0);
    ledcWrite(IN2, -speed);
  }
  else
  {
    ledcWrite(IN1, 0);
    ledcWrite(IN2, 0);
  }
}

/**
 * Set right motor speed and direction
 * Same behavior as setMotorLeft()
 * 
 * @param speed Motor speed (-255 to 255)
 */
void setMotorRight(int speed)
{
  speed = constrain(speed, -255, 255);

  if (speed > 0)
  {
    ledcWrite(IN3, speed);
    ledcWrite(IN4, 0);
  }
  else if (speed < 0)
  {
    ledcWrite(IN3, 0);
    ledcWrite(IN4, -speed);
  }
  else
  {
    ledcWrite(IN3, 0);
    ledcWrite(IN4, 0);
  }
}

/**
 * Stop both motors immediately
 * Used by fail-safe mechanism and during idle state
 */
void stopMotors()
{
  setMotorLeft(0);
  setMotorRight(0);
}

// --------------------------------------------------
// MOVEMENT COMMANDS
// Motor control commands received via ESP-NOW
// --------------------------------------------------

/**
 * Execute motor command with speed
 * Called whenever a new control packet is received
 * 
 * @param cmd Motor command (CMD_STOP, CMD_FORWARD, CMD_BACKWARD, CMD_LEFT, CMD_RIGHT)
 * @param speed Motor speed (0-255)
 * 
 * Motor mappings:
 * - CMD_FORWARD: Both motors forward at same speed (straight line)
 * - CMD_BACKWARD: Both motors backward at same speed (straight line)
 * - CMD_LEFT: Left motor backward, Right motor forward (pivot left)
 * - CMD_RIGHT: Left motor forward, Right motor backward (pivot right)
 * - CMD_STOP: Both motors stopped
 */

void executeCommand(uint8_t cmd, uint8_t speed)
{
  Serial.print("CMD: ");
  Serial.print(cmd);
  Serial.print(" | Speed: ");
  Serial.println(speed);
  updateFaceState(cmd, speed);

  switch (cmd)
  {
    case CMD_FORWARD:
      setMotorLeft(speed);
      setMotorRight(speed);
      break;

    case CMD_BACKWARD:
      setMotorLeft(-speed);
      setMotorRight(-speed);
      break;

    case CMD_LEFT:
      setMotorLeft(-speed);
      setMotorRight(speed);
      break;

    case CMD_RIGHT:
      setMotorLeft(speed);
      setMotorRight(-speed);
      break;

    case CMD_STOP:
    default:
      stopMotors();
      break;
  }
}

// --------------------------------------------------
// ESP-NOW CALLBACK HANDLERS
// Communication protocol handlers
// --------------------------------------------------

/**
 * ESP-NOW data receive callback
 * Called automatically when a packet is received via ESP-NOW (uses interrupt)
 * 
 * @param src MAC address of sender (as array index - 1-255)
 * @param id Message type ID (MSG_CONTROL or MSG_TELEMETRY)
 * @param data Payload pointer
 * @param len Payload length in bytes
 * @param rssi Received signal strength indicator
 * 
 * Behavior:
 * - Only processes MSG_CONTROL packets
 * - Validates packet length matches control_t struct size
 * - Updates last command timestamp for fail-safe mechanism
 * - Sets hasControlLink flag to enable telemetry
 * - Triggers recovery animation if connection was previously lost (wasDisconnected)
 * - Logs received command and RSSI to serial
 */

void onData(uint8_t src, uint8_t id, const uint8_t *data, uint8_t len, int8_t rssi)
{
  if (id != MSG_CONTROL) return;

  if (len != sizeof(control_t)) return;

  control_t ctrl;
  memcpy(&ctrl, data, sizeof(ctrl));

  bool wasDisconnected = !hasControlLink || (millis() - lastCommandTime > COMMAND_TIMEOUT);
  lastCommandTime = millis();
  hasControlLink = true;
  if (wasDisconnected)
  {
    faceRecoveryPending = true;
  }

  Serial.print("From: ");
  Serial.print(src);
  Serial.print(" | RSSI: ");
  Serial.println(rssi);

  executeCommand(ctrl.cmd, ctrl.speed);
}

// --------------------------------------------------

/**
 * System initialization
 * Sets up:
 * - Serial communication at 115200 baud
 * - Motor control pins and PWM
 * - ESP-NOW protocol and node ID
 * - Face animation task on core 1
 */
void setup()
{
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  setupPWM();
  stopMotors();

  protocol.begin();
  protocol.setNodeId(2); // ROBÔ

  protocol.enableAutoDiscovery(true);
  protocol.onReceive(onData);

  lastCommandTime = millis();
  startFaceAnimationTask();

  Serial.println("Robot ready (v1.2 face task)");
}

// --------------------------------------------------

/**
 * Main event loop
 * Runs continuously on core 0
 * 
 * Responsibilities:
 * - Process ESP-NOW protocol events
 * - Implement fail-safe mechanism (stop on command timeout)
 * - Send periodic telemetry to keep link alive
 */
void loop()
{
  protocol.loop();

  unsigned long now = millis();

  // FAIL-SAFE (rápido)
  if (now - lastCommandTime > COMMAND_TIMEOUT)
  {
    stopMotors();
    updateFaceState(CMD_STOP, 0);
    desiredFaceState = FACE_TIMEOUT;
    hasControlLink = false;
  }

  // TELEMETRIA (mantém link vivo)
  if (now - lastTelemetry > TELEMETRY_INTERVAL)
  {
    telemetry_t t;
    t.status = 1;

    protocol.send(255, MSG_TELEMETRY, (uint8_t*)&t, sizeof(t));

    lastTelemetry = now;
  }
}
