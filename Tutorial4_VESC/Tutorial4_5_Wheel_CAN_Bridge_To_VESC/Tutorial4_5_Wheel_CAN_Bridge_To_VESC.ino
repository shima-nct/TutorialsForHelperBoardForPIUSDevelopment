#include <Arduino.h>
#include <driver/twai.h>

// このスケッチは、Wheel CAN Bridge が送信する人間操作量CANフレームを受信し、
// VESCが理解できる duty 指令CANフレームへ変換して送信します。
//
// Wheel CAN Bridge:
//   0x2010 DRIVER_INPUT
//   throttle, brake, steering, flags, counter を1フレームにまとめた独自形式
//
// VESC:
//   CAN_PACKET_SET_DUTY
//   duty比を int32 big-endian、scale 100000 で送る標準形式
//
// 実機確認時は、最初にモーターを外すか、VESC側を安全な状態にしてください。

// --- CAN (TWAI) settings for the VESC helper board ---
constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_1;
constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_0;
constexpr long CAN_BAUD = 500000; // 500 kbps

// --- Wheel CAN Bridge DRIVER_INPUT message ---
constexpr uint32_t WCB_DRIVER_INPUT_ID = 0x2010;
constexpr uint8_t WCB_DRIVER_INPUT_DLC = 8;
constexpr int32_t WCB_SCALE = 10000;

constexpr uint8_t WCB_FLAG_ENABLE = 1 << 0;
constexpr uint8_t WCB_FLAG_ESTOP = 1 << 1;
constexpr uint8_t WCB_FLAG_REVERSE = 1 << 2;

// --- VESC CAN protocol ---
constexpr uint32_t CAN_PACKET_SET_DUTY = 0x0;
constexpr uint8_t VESC_ID = 0x7; // Match VESC Tool: App Settings / General / VESC ID
constexpr int32_t VESC_DUTY_SCALE = 100000;

// --- Safety and conversion settings ---
constexpr uint32_t WCB_TIMEOUT_MS = 200;
constexpr float MAX_FORWARD_DUTY = 0.30f;
constexpr float MAX_REVERSE_DUTY = 0.20f;
constexpr float BRAKE_PRIORITY_THRESHOLD = 0.05f;

struct DriverInput {
  // Wheel CAN Bridgeの正規化済み操作量です。
  // throttle/brake は 0.0..1.0、steering は -1.0..1.0 です。
  float throttle = 0.0f;
  float brake = 0.0f;
  float steering = 0.0f;
  bool enable = false;
  bool estop = true;
  bool reverse = false;
  uint8_t counter = 0;
};

static DriverInput latest_input;
static bool latest_input_valid = false;
static uint32_t latest_input_ms = 0;

// 受信タスクとVESC送信タスクが latest_input を共有するため、
// ESP32のクリティカルセクションで短時間だけ保護します。
static portMUX_TYPE input_mux = portMUX_INITIALIZER_UNLOCKED;

constexpr twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
constexpr twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
constexpr twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

static uint16_t read_u16_be(const uint8_t *data) {
  return (static_cast<uint16_t>(data[0]) << 8) | data[1];
}

static int16_t read_i16_be(const uint8_t *data) {
  return static_cast<int16_t>(read_u16_be(data));
}

static void write_i32_be(uint8_t *data, int32_t value) {
  data[0] = static_cast<uint8_t>((value >> 24) & 0xff);
  data[1] = static_cast<uint8_t>((value >> 16) & 0xff);
  data[2] = static_cast<uint8_t>((value >> 8) & 0xff);
  data[3] = static_cast<uint8_t>(value & 0xff);
}

static float clamp_float(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

static bool decode_wheel_can_bridge(const twai_message_t &msg, DriverInput &out) {
  // 目的のWheel CAN Bridgeフレーム以外は無視します。
  // extended ID、非RTR、ID、DLCをすべて確認します。
  if (!msg.extd || msg.rtr || msg.identifier != WCB_DRIVER_INPUT_ID ||
      msg.data_length_code != WCB_DRIVER_INPUT_DLC) {
    return false;
  }

  const uint16_t throttle_raw = read_u16_be(&msg.data[0]);
  const uint16_t brake_raw = read_u16_be(&msg.data[2]);
  const int16_t steering_raw = read_i16_be(&msg.data[4]);
  const uint8_t flags = msg.data[6];

  out.throttle = clamp_float(throttle_raw / static_cast<float>(WCB_SCALE), 0.0f, 1.0f);
  out.brake = clamp_float(brake_raw / static_cast<float>(WCB_SCALE), 0.0f, 1.0f);
  out.steering = clamp_float(steering_raw / static_cast<float>(WCB_SCALE), -1.0f, 1.0f);

  // flagsは受信側安全判定に使います。
  // enableが立っていない、またはestopが立っている場合はVESCへdutyを出しません。
  out.enable = (flags & WCB_FLAG_ENABLE) != 0;
  out.estop = (flags & WCB_FLAG_ESTOP) != 0;
  out.reverse = (flags & WCB_FLAG_REVERSE) != 0;
  out.counter = msg.data[7];
  return true;
}

static float driver_input_to_duty(const DriverInput &input, bool fresh) {
  // 通信断、無効状態、非常停止では必ずduty 0にします。
  if (!fresh || !input.enable || input.estop) {
    return 0.0f;
  }

  // ブレーキ優先です。アクセルとブレーキが同時に入った場合もduty 0にします。
  if (input.brake > BRAKE_PRIORITY_THRESHOLD) {
    return 0.0f;
  }

  // このサンプルではスロットルをVESC dutyへ単純変換します。
  // 実車では最大duty、加速度制限、電流制限、受信側timeoutを必ず調整してください。
  const float limit = input.reverse ? MAX_REVERSE_DUTY : MAX_FORWARD_DUTY;
  const float duty = clamp_float(input.throttle, 0.0f, 1.0f) * limit;
  return input.reverse ? -duty : duty;
}

static void send_vesc_duty(float duty) {
  // VESCの単純CANコマンドは、拡張IDに command_id と VESC ID を入れ、
  // 4バイトの int32 big-endian をpayloadとして送ります。
  twai_message_t msg = {};
  msg.identifier = (CAN_PACKET_SET_DUTY << 8) | VESC_ID;
  msg.flags = TWAI_MSG_FLAG_EXTD;
  msg.data_length_code = sizeof(int32_t);

  const int32_t scaled = static_cast<int32_t>(duty * VESC_DUTY_SCALE);
  write_i32_be(msg.data, scaled);

  if (twai_transmit(&msg, pdMS_TO_TICKS(10)) != ESP_OK) {
    Serial.println("TWAI transmit failed");
  }
}

static void wheel_can_receive_task(void *arg) {
  (void)arg;
  twai_message_t msg;

  for (;;) {
    // Wheel CAN Bridgeからの入力フレームを待ちます。
    // 関係ないCANフレームは decode_wheel_can_bridge() 側で無視します。
    if (twai_receive(&msg, pdMS_TO_TICKS(1000)) != ESP_OK) {
      continue;
    }

    DriverInput decoded;
    if (!decode_wheel_can_bridge(msg, decoded)) {
      continue;
    }

    portENTER_CRITICAL(&input_mux);
    latest_input = decoded;
    latest_input_valid = true;
    latest_input_ms = millis();
    portEXIT_CRITICAL(&input_mux);
  }
}

static void vesc_control_task(void *arg) {
  (void)arg;
  TickType_t last_wake = xTaskGetTickCount();

  for (;;) {
    DriverInput input;
    bool valid;
    uint32_t age_ms;

    portENTER_CRITICAL(&input_mux);
    input = latest_input;
    valid = latest_input_valid;
    age_ms = millis() - latest_input_ms;
    portEXIT_CRITICAL(&input_mux);

    // 200ms以上Wheel CAN Bridgeから届かなければ通信断としてduty 0にします。
    const bool fresh = valid && age_ms <= WCB_TIMEOUT_MS;
    const float duty = driver_input_to_duty(input, fresh);
    send_vesc_duty(duty);

    // 1秒に1回だけログを出します。
    // 20ms周期の制御タスクで毎回Serial出力すると周期が乱れやすいためです。
    static uint32_t log_count = 0;
    if ((log_count++ % 50) == 0) {
      Serial.printf("WCB thr=%.3f brk=%.3f str=%+.3f en=%u estop=%u rev=%u fresh=%u -> duty=%.3f\n",
                    input.throttle, input.brake, input.steering,
                    input.enable ? 1 : 0, input.estop ? 1 : 0, input.reverse ? 1 : 0,
                    fresh ? 1 : 0, duty);
    }

    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(20));
  }
}

void setup() {
  Serial.begin(115200);

  // TWAIはESP32内蔵のCANコントローラです。
  // VESCヘルパーボードでは外付けトランシーバを通してCANバスへ接続されます。
  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("Failed to install TWAI driver");
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  if (twai_start() != ESP_OK) {
    Serial.println("Failed to start TWAI driver");
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  Serial.println("Wheel CAN Bridge to VESC duty bridge");
  Serial.println("RX: 0x2010 DRIVER_INPUT, TX: VESC CAN_PACKET_SET_DUTY");

  // 受信タスクと送信タスクを分けることで、受信待ちで20ms周期送信が止まらないようにします。
  xTaskCreate(wheel_can_receive_task, "WCBReceive", 4096, NULL, 5, NULL);
  xTaskCreate(vesc_control_task, "VESCControl", 4096, NULL, 4, NULL);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
