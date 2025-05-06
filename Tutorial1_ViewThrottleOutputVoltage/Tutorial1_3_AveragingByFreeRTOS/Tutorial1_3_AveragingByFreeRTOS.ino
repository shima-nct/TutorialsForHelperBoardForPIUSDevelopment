/*
  M5Stamp C3U Mate: FreeRTOS で 2ms ごとに ADC → 10サンプル平均 → 約20ms ごとに表示
  VCC → 5V, GND → GND, OUT → G3（GPIO3）
*/

#include <Arduino.h>
// Arduino.h をインクルードしておけば、内部で以下の FreeRTOS ヘッダがすでに取り込まれている。

static const gpio_num_t HALL_PIN = GPIO_NUM_3;  // ADC入力ピン

// サンプリングタスク
void samplingTask(void* pvParameters) {
  const TickType_t xPeriod = pdMS_TO_TICKS(2);     // 2ms
  TickType_t xLastWakeTime = xTaskGetTickCount();

  uint32_t sum = 0;
  uint8_t count = 0;

  for (;;) {
    int raw = analogRead(HALL_PIN);
    sum += raw;
    count++;

    if (count >= 10) {
      // 12bit (0–4095) → 0–2.5V スケール
      float avgRaw    = sum / 10.0f;
      float voltage   = avgRaw * (2.5f / 4095.0f);

      Serial.printf("AvgRaw: %6.1f, V: %.3f V\n", avgRaw, voltage);

      sum   = 0;
      count = 0;
    }

    // 次の周期まで待つ
    vTaskDelayUntil(&xLastWakeTime, xPeriod);
  }
}

void setup() {
  Serial.begin(115200);

  // 減衰設定：ADC_11db で最大約2.5Vまで測定可能に
  analogSetPinAttenuation(HALL_PIN, ADC_11db);

  // サンプリングタスク生成（コア1で動かしますが、ESP32-C3 はシングルコアです）
  xTaskCreate(
    samplingTask,    // タスク関数
    "Sampling2ms",   // タスク名
    2048,            // スタックサイズ(bytes)
    nullptr,         // 引数
    1,               // 優先度
    nullptr          // タスクハンドル
  );
}

void loop() {
  // loop() は使いません
}
