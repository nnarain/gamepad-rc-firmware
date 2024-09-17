/**
  Firmware for converting commands from Bluetooth gamepads into SBUS serial commands

  @author Natesh Narain <nnaraindev@gmail.com>
*/

#include <Bluepad32.h>

#define OutputSerial Serial
#define CON_LED_PIN 25
#define STAT_LED_PIN 26

#define DEAD_MANS_SWITCH_ACTIVE_THRESHOLD 100

class ChannelBuffer
{
  static const uint8_t HEADER_SIZE = 1;
  static const uint8_t FOOTER_SIZE = 1;
  static const uint8_t NUM_CHNLS = 4;
  static const uint8_t BUF_SIZE = HEADER_SIZE + FOOTER_SIZE + (sizeof(int16_t) * NUM_CHNLS);
  static const uint8_t HEADER = 0;
  static const uint8_t FOOTER = BUF_SIZE - 1;
public:
  ChannelBuffer()
  {
    buf_[HEADER] = 0x0F;
    buf_[FOOTER] = 0x00;
  }

  void setChannel(const uint8_t chnl, int16_t value)
  {
    if (chnl >= NUM_CHNLS)
    {
      return;
    }
    // Offset by 1 since the header is first
    const auto chnl_idx = (chnl * 2) + 1;
    //
    if (chnl_idx >= FOOTER - 1)
    {
      return;
    }

    // Little endian
    buf_[chnl_idx] = (value & 0x00FF);
    buf_[chnl_idx + 1] = (value >> 8) & 0x00FF;
  }

  const uint8_t* getBuf() const
  {
    return buf_;
  }

  const size_t getSize() const
  {
    return BUF_SIZE;
  }
private:
  uint8_t buf_[BUF_SIZE];
};

//! The connected gamepad (only supports on at a time)
GamepadPtr gamepad = nullptr;
//! The data buffer to store the channel data
ChannelBuffer chnl_buffer_;

uint32_t last_led_time = 0;
bool led_state = false;

bool dead_mans_switch_active = false;

// Arduino setup function. Runs in CPU 1
void setup() {
  Serial.begin(115200);
  OutputSerial.begin(115200);

  pinMode(CON_LED_PIN, OUTPUT);
  pinMode(STAT_LED_PIN, OUTPUT);

  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

  BP32.forgetBluetoothKeys();
}

void loop() {
  BP32.update();

  if (gamepad && gamepad->isConnected()) {
    processRcData(gamepad, chnl_buffer_);

    // Only send data if the dead man's switch is active
    // It will be up to the receiving device to properly handle how to fail safe
    if (dead_mans_switch_active)
    {
      OutputSerial.write(reinterpret_cast<const char*>(chnl_buffer_.getBuf()), chnl_buffer_.getSize());
    }
  }

  const auto now = millis();
  if (now >= last_led_time + 500) {
    digitalWrite(STAT_LED_PIN, led_state);
    last_led_time = now;
    led_state = !led_state;
  }

  // The main loop must have some kind of "yield to lower priority task" event.
  // Otherwise the watchdog will get triggered.
  // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
  // Detailed info here:
  // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

  // vTaskDelay(1);
  delay(150);
}

void processRcData(GamepadPtr gamepad, ChannelBuffer& rc_data)
{
  // Expected value range is [-511, 512]

  // Pitch
  const auto chnl0 = static_cast<int16_t>(gamepad->axisY());
  // Roll
  const auto chnl1 = static_cast<int16_t>(gamepad->axisX());
  // Yaw
  const auto chnl2 = static_cast<int16_t>(gamepad->axisRX());
  // Raw vertical velocity
  const auto chnl3 = static_cast<int16_t>(gamepad->axisRY());

  // Use the left trigger as the dead man's switch
  const auto dead_mans_switch_raw = gamepad->brake();
  // Determine if the switch is pressed
  dead_mans_switch_active = abs32(dead_mans_switch_raw) > DEAD_MANS_SWITCH_ACTIVE_THRESHOLD;

  rc_data.setChannel(0, chnl0);
  rc_data.setChannel(1, chnl1);
  rc_data.setChannel(2, chnl2);
  rc_data.setChannel(3, chnl3);
}

void onConnectedGamepad(GamepadPtr gp) {
  if (!gamepad)
  {
    GamepadProperties properties = gp->getProperties();
    Serial.printf("Gamepad model: %s, VID=0x%04x, PID=0x%04x\n",
                  gp->getModelName().c_str(), properties.vendor_id,
                  properties.product_id);
    gamepad = gp;

    // Set the gamepad LED
    gamepad->setColorLED(0, 255, 0);

    // Indicate the gamepad is connected
    digitalWrite(CON_LED_PIN, HIGH);
  }
}

void onDisconnectedGamepad(GamepadPtr gp) {
  if (gamepad != nullptr && gamepad == gp)
  {
    gamepad = nullptr;

    // Indicate the gamepad has disconnected
    digitalWrite(CON_LED_PIN, LOW);
  }
}
