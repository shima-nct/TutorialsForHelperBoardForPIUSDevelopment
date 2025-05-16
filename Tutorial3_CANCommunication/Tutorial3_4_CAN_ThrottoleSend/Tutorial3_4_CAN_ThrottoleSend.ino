#include <Arduino.h>
#include <driver/twai.h>
#include <esp32-hal-adc.h>  // ESP32-C3 でも analogSetPinAttenuation が使えます

// --- ADC 設定 ---
constexpr gpio_num_t HALL_PIN     = GPIO_NUM_3;  // スロットルセンサー入力
constexpr int      ADC_RESOLUTION = 4095;       // 12bit
// 減衰：0–2.5V まで測定可能
// （ESP32-C3 の ADC_11db は約0–2.5Vの範囲） 
// → 2.5V前後まで安全に読み取れます
// ※ 必要なら電圧分圧回路を追加してください

// --- CAN (TWAI) 設定 ---
constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_1;
constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_0;
constexpr long       CAN_BAUD   = 500000;  // 500 kbps

constexpr uint32_t CAN_PACKET_SET_DUTY = 0x0;
constexpr uint8_t UNIT_ID = 0x7;

// TWAI の一般設定・タイミング設定・フィルタ設定
twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
  CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL
);
twai_timing_config_t   t_config = TWAI_TIMING_CONFIG_500KBITS();
twai_filter_config_t   f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

void setup() {
  Serial.begin(115200);

  // ADC 減衰設定（11dB で約0–2.5V測定可）  
  analogSetPinAttenuation(HALL_PIN, ADC_11db);

  // TWAI ドライバのインストール
  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("Failed to install TWAI driver");
    while (1);
  }
  // TWAI ドライバ開始
  if (twai_start() != ESP_OK) {
    Serial.println("Failed to start TWAI driver");
    while (1);
  }

  Serial.println("TWAI (CAN) initialized");
}

void loop() {
  // 1) スロットル ADC 読み取り
  constexpr float HALL_MIN = 0.29f;  // スロットル最小値 (29%)
  constexpr float HALL_MAX = 0.92f;  // スロットル最大値 (92%)
  
  int code = analogRead(HALL_PIN);
  float percent = code / (float)ADC_RESOLUTION;
  // 実際の範囲から0-100%に正規化
  float pwm = (percent - HALL_MIN) / (HALL_MAX - HALL_MIN);
  pwm = constrain(pwm, 0.0f, 1.0f);  // 0.0–1.0
  int32_t scaled = (int32_t)(pwm * 100000.0f);
  // 2) CAN メッセージ作成（拡張フォーマット, CAN_PACKET_SET_DUTY=0x00, 4バイトデータ）
  twai_message_t msg;
  memset(&msg, 0, sizeof(msg));
  msg.identifier = (CAN_PACKET_SET_DUTY << 8) | UNIT_ID ;
  msg.flags = TWAI_MSG_FLAG_EXTD;
  // ビッグエンディアンとして詰め直し。
  // "All simple CAN-commands have 4 data bytes which 
  // represent the argument for the commands as a 
  // 32-bit big endian signed number with scaling."
  // https://raw.githubusercontent.com/vedderb/bldc/master/documentation/comm_can.md
  uint8_t buf[4] = {
    (uint8_t)(scaled >> 24),
    (uint8_t)(scaled >> 16),
    (uint8_t)(scaled >>  8),
    (uint8_t)(scaled      )
  };
  memcpy(msg.data, buf, sizeof(buf));
  msg.data_length_code = sizeof(scaled);

  Serial.print("PWM duty/%: ");
  Serial.println(scaled/1000);

  // 3) 送信（送信待ち最大 10 ms）
  if (twai_transmit(&msg, pdMS_TO_TICKS(10)) != ESP_OK) {
    Serial.println("TWAI transmit failed");
  }

  delay(10);  // 約100 Hz 更新
}
