#include <Wire.h>
#include <SparkFun_Alphanumeric_Display.h>  // SparkFun Qwiic Alphanumeric Display ライブラリ :contentReference[oaicite:1]{index=1}

HT16K33 display;                          // 表示オブジェクト
const uint8_t DISPLAY_ADDR = 0x70;        // デフォルト I²C アドレス

unsigned long prevMillis = 0;             // 前回更新時刻
unsigned long startMillis;                // 起動時刻

void setup() {
  Serial.begin(115200);
  Wire.begin();
  // ディスプレイ初期化
  if (!display.begin(DISPLAY_ADDR)) {
    Serial.println("Display not found!");
    while (1);
  }
  display.setBrightness(15);              // 輝度最大（0～15）
  display.clear();                        // 画面クリア
  startMillis = millis();                 // 起点を記録
}

void loop() {
  unsigned long now = millis();
  // 100ms ごとに更新
  if (now - prevMillis >= 100) {
    prevMillis = now;

    // 経過時間（秒）を計算
    float elapsed = (now - startMillis) / 1000.0f;

    // 4桁・小数点1桁で文字列化（例: "12.3"）
    char buf[5];
    dtostrf(elapsed, 4, 1, buf);

    // 表示
    display.print(buf);
    // ※ display.print() で即時書き込みされます
  }
}
