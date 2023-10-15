/**
  Firmware for converting commands from Bluetooth gamepads into SBUS serial commands

  @author Natesh Narain <nnaraindev@gmail.com>
*/

#include <Bluepad32.h>
#include <sbus.h>

#define SBUS_INT_MAX 2047

//! The connected gamepad (only supports on at a time)
GamepadPtr gamepad = nullptr;

//! SBUS transmitter
bfs::SbusTx sbus(&Serial1, 16, 17, false);
//! SBUS data to transmit
bfs::SbusData rc_data;

// Arduino setup function. Runs in CPU 1
void setup() {
  Serial.begin(115200);
  sbus.Begin();

  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

  BP32.forgetBluetoothKeys();
}

void loop() {
  BP32.update();

  if (gamepad && gamepad->isConnected()) {
    processRcData(gamepad, rc_data);

    sbus.data(rc_data);
    sbus.Write();
  }

  // The main loop must have some kind of "yield to lower priority task" event.
  // Otherwise the watchdog will get triggered.
  // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
  // Detailed info here:
  // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

  // vTaskDelay(1);
  delay(150);
}

void processRcData(GamepadPtr gamepad, bfs::SbusData& rc_data)
{
  // Pitch
  rc_data.ch[0] = map_value(gamepad->axisY(), -511, 512, 0, SBUS_INT_MAX);
  // roll
  rc_data.ch[1] = map_value(gamepad->axisY(), -511, 512, 0, SBUS_INT_MAX);
  // Yaw
  rc_data.ch[2] = map_value(gamepad->axisRX(), -511, 512, 0, SBUS_INT_MAX);
  // Raw vertical throttle
  rc_data.ch[3] = map_value(gamepad->axisRY(), -511, 512, 0, SBUS_INT_MAX);
}

void onConnectedGamepad(GamepadPtr gp) {
  if (!gamepad)
  {
    GamepadProperties properties = gp->getProperties();
    Serial.printf("Gamepad model: %s, VID=0x%04x, PID=0x%04x\n",
                  gp->getModelName().c_str(), properties.vendor_id,
                  properties.product_id);
    gamepad = gp;
    // TODO(nnarain): Set CON LED

    // Set the gamepad LED
    gamepad->setColorLED(0, 255, 0);
  }
}

void onDisconnectedGamepad(GamepadPtr gp) {
  if (gamepad != nullptr && gamepad == gp)
  {
    gamepad = nullptr;
  }
}

inline double map_value(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
