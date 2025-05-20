#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>
#include <driver/twai.h>
#include <esp32-hal-adc.h> // ESP32-C3 でも analogSetPinAttenuation が使えます

// --- ADC 設定 ---
constexpr gpio_num_t HALL_PIN = GPIO_NUM_3; // スロットルセンサー入力
constexpr int ADC_RESOLUTION = 4095;        // 12bit

// --- CAN (TWAI) 設定 ---
constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_1;
constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_0;
constexpr long CAN_BAUD = 500000; // 500 kbps

// --- VESC CAN プロトコル ---
constexpr uint32_t CAN_PACKET_SET_DUTY = 0x0; // デューティコマンド識別子
constexpr uint8_t VESC_ID =
    0x7; // VESC ID: VESC Tool/App Settings/General/VESC ID

// --- スロットル平均化設定 ---
constexpr int SAMPLES_COUNT = 20; // 20msの間に20サンプル（1ms毎）
static int throttle_samples[SAMPLES_COUNT];
static int sample_index = 0;
static float HALL_MIN = 0.29f; // スロットル最小値 (29%)
static float HALL_MAX = 0.92f; // スロットル最大値 (92%)

// TWAI の一般設定・タイミング設定・フィルタ設定
constexpr twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
constexpr twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
constexpr twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

// VESCコントロールタスク
void vescControlTask(void *pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(20); // 20ms間隔

  // タスクの初期化
  xLastWakeTime = xTaskGetTickCount();

  while (1) {
    // スロットル値の平均を計算
    int32_t sum = 0;
    for (int i = 0; i < SAMPLES_COUNT; i++) {
      sum += throttle_samples[i];
    }
    float percent = (sum / (float)(SAMPLES_COUNT * ADC_RESOLUTION));

    // 実際の範囲から0-100%に正規化
    float pwm = (percent - HALL_MIN) / (HALL_MAX - HALL_MIN);
    pwm = constrain(pwm, 0.0f, 1.0f); // 0.0–1.0
    int32_t scaled = (int32_t)(pwm * 100000.0f);

    // CAN メッセージ作成
    twai_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.identifier = (CAN_PACKET_SET_DUTY << 8) | VESC_ID;
    msg.flags = TWAI_MSG_FLAG_EXTD;

    // ビッグエンディアンとしてデータを詰める
    uint8_t buf[4] = {(uint8_t)(scaled >> 24), (uint8_t)(scaled >> 16),
                      (uint8_t)(scaled >> 8), (uint8_t)(scaled)};
    memcpy(msg.data, buf, sizeof(buf));
    msg.data_length_code = sizeof(scaled);

    Serial.print("PWM/%:");
    Serial.println(scaled / 1000);

    // 送信（送信待ち最大 10 ms）
    if (twai_transmit(&msg, pdMS_TO_TICKS(10)) != ESP_OK) {
      Serial.println("TWAI transmit failed");
    }

    // 次の周期まで待機
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// スロットル読み取りタスク
void throttleReadTask(void *pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(1); // 1ms間隔

  // タスクの初期化
  xLastWakeTime = xTaskGetTickCount();

  while (1) {
    // スロットル値を読み取り
    throttle_samples[sample_index] = analogRead(HALL_PIN);
    sample_index = (sample_index + 1) % SAMPLES_COUNT;

    // 次の周期まで待機
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void setup() {
  Serial.begin(115200);

  // ADC 減衰設定（11dB で約0–2.5V測定可）
  analogSetPinAttenuation(HALL_PIN, ADC_11db);

  // スロットルサンプル配列の初期化
  for (int i = 0; i < SAMPLES_COUNT; i++) {
    throttle_samples[i] = 0;
  }

  // TWAI ドライバのインストール
  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("Failed to install TWAI driver");
    while (1)
      ;
  }
  // TWAI ドライバ開始
  if (twai_start() != ESP_OK) {
    Serial.println("Failed to start TWAI driver");
    while (1)
      ;
  }

  Serial.println("TWAI (CAN) initialized");

  // FreeRTOSタスクの作成
  xTaskCreate(throttleReadTask, "ThrottleRead", 2048, NULL, 2, NULL);
  xTaskCreate(vescControlTask, "VESCControl", 2048, NULL, 1, NULL);
}

void loop() {
  // メインループは空です - すべての処理はFreeRTOSタスクで行われます
  vTaskDelay(portMAX_DELAY);
}
