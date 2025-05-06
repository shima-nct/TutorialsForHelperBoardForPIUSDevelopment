#include <Arduino.h>
#include <driver/twai.h>
#include <esp32-hal-adc.h>

// --- ADC 設定 ---
static const gpio_num_t HALL_PIN     = GPIO_NUM_3;  // スロットル入力（GPIO3）
static const int      ADC_RESOLUTION = 4095;       // 12bit (0–4095)

// --- VESC CAN プロトコル ---
#define CAN_PACKET_SET_DUTY 0     // デューティコマンド識別子
static const uint8_t   VESC_ID          = 1;       // VESC 側の CAN ID

// --- TWAI(CAN) 設定 ---
static const gpio_num_t CAN_TX_PIN = GPIO_NUM_5;
static const gpio_num_t CAN_RX_PIN = GPIO_NUM_4;
static const long       CAN_BAUD   = 500000;       // 500 kbps

// 送信タスク
void canTxTask(void* pvParameters) {
  // 周期制御用
  const TickType_t period = pdMS_TO_TICKS(10);
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    // 1) ADC 読み取り → 正規化
    int raw = analogRead(HALL_PIN);
    float throttle = raw / (float)ADC_RESOLUTION;
    throttle = constrain(throttle, 0.0f, 1.0f);

    // 2) 32bit スケーリング（0～100000）
    int32_t duty_int = (int32_t)(throttle * 100000.0f);

    // 3) Big-endian パッキング
    uint8_t data[4];
    data[0] = (duty_int >> 24) & 0xFF;
    data[1] = (duty_int >> 16) & 0xFF;
    data[2] = (duty_int >>  8) & 0xFF;
    data[3] = (duty_int      ) & 0xFF;

    // 4) TWAI メッセージ作成
    twai_message_t msg = {};
    msg.identifier       = (uint32_t)VESC_ID | ((uint32_t)CAN_PACKET_SET_DUTY << 8);
    msg.extd             = false;
    msg.rtr              = false;
    msg.data_length_code = sizeof(data);
    memcpy(msg.data, data, sizeof(data));

    // 5) 送信（タイムアウト 10 ms）
    twai_transmit(&msg, pdMS_TO_TICKS(10));

    // 次の周期までブロック
    vTaskDelayUntil(&lastWake, period);
  }
}

void setup() {
  Serial.begin(115200);

  // ADC 減衰設定（11dB で約0–2.6V 測定可）
  analogSetPinAttenuation(HALL_PIN, ADC_11db);

  // TWAI ドライバ初期化
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
    CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL
  );
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK ||
      twai_start() != ESP_OK) {
    Serial.println("TWAI init failed");
    while (1) { vTaskDelay(1000); }
  }

  // FreeRTOS タスク生成（送信専用）
  xTaskCreate(
    canTxTask,     // タスク関数
    "CAN_Tx",      // タスク名
    4096,          // スタックサイズ
    nullptr,       // パラメータ
    1,             // 優先度
    nullptr        // タスクハンドル
  );
}

void loop() {
  // FreeRTOS タスクにすべて任せるので何もしない
}
