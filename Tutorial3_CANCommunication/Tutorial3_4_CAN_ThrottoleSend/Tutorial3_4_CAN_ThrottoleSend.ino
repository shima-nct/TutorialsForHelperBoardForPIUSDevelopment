#include <Arduino.h>
#include <driver/twai.h>
#include <esp32-hal-adc.h>  // ESP32-C3 でも analogSetPinAttenuation が使えます

// --- ADC 設定 ---
static const gpio_num_t HALL_PIN     = GPIO_NUM_3;  // スロットルセンサー入力
static const int      ADC_RESOLUTION = 4095;       // 12bit
// 減衰：0–2.6V まで測定可能
// （ESP32-C3 の ADC_11db は約0–2.6Vの範囲） 
// → 2.5V前後まで安全に読み取れます
// ※ 必要なら電圧分圧回路を追加してください

// --- CAN (TWAI) 設定 ---
static const gpio_num_t CAN_TX_PIN = GPIO_NUM_1;
static const gpio_num_t CAN_RX_PIN = GPIO_NUM_0;
static const long       CAN_BAUD   = 500000;  // 500 kbps

// TWAI の一般設定・タイミング設定・フィルタ設定
twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
  CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL
);
twai_timing_config_t   t_config = TWAI_TIMING_CONFIG_500KBITS();
twai_filter_config_t   f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

void setup() {
  Serial.begin(115200);

  // ADC 減衰設定（11dB で約0–2.6V測定可）  
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
  int raw = analogRead(HALL_PIN);
  float throttle = raw / (float)ADC_RESOLUTION;
  throttle = constrain(throttle, 0.0f, 1.0f);  // 0.0–1.0

  // 2) CAN メッセージ作成（標準フォーマット, ID=0x01, 4バイトデータ）
  twai_message_t msg = {};
  msg.identifier        = 0x01;
  msg.extd              = false;
  msg.rtr               = false;
  msg.data_length_code  = sizeof(throttle);
  memcpy(msg.data, &throttle, sizeof(throttle));

  // 3) 送信（送信待ち最大 10 ms）
  if (twai_transmit(&msg, pdMS_TO_TICKS(10)) != ESP_OK) {
    Serial.println("TWAI transmit failed");
  }

  delay(10);  // 約100 Hz 更新
}
