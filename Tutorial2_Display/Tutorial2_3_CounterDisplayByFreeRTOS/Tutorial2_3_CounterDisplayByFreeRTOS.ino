#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_Alphanumeric_Display.h>  // Qwiic Alphanumeric Display ライブラリ :contentReference[oaicite:0]{index=0}

HT16K33 display;                          // 表示オブジェクト
const uint8_t DISPLAY_ADDR = 0x70;        // デフォルト I²C アドレス

unsigned long startMillis;                // 起動時刻

// 100ms ごとに経過時間を計算して表示するタスク
void displayTask(void* pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriod = pdMS_TO_TICKS(100);  // 100ms 周期

  char buf[5];
  for (;;) {
    unsigned long now = millis();
    float elapsed = (now - startMillis) / 1000.0f;
    dtostrf(elapsed, 4, 1, buf);
    display.print(buf);
    // タスクを次の周期までブロック（周期実行） :contentReference[oaicite:1]{index=1}
    vTaskDelayUntil(&xLastWakeTime, xPeriod);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // ディスプレイ初期化
  if (!display.begin(DISPLAY_ADDR)) {
    Serial.println("Display not found!");
    while (1);
  }
  display.setBrightness(15);  // 輝度最大
  display.clear();

  startMillis = millis();     // 起点記録

  // 表示タスク生成
  xTaskCreate(
    displayTask,    // タスク関数
    "DisplayTask",  // タスク名
    2048,           // スタックサイズ（バイト）
    nullptr,        // 引数
    1,              // 優先度
    nullptr         // タスクハンドル
  );
}

void loop() {
  // メインループは使わない
}
