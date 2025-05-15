#include <Wire.h>
#include <SparkFun_Alphanumeric_Display.h>  // SparkFun Qwiic Alphanumeric Display ライブラリ

constexpr uint8_t I2C_SDA = GPIO_NUM_8;          // GPIO8  (G8)
constexpr uint8_t I2C_SCL = GPIO_NUM_10;         // GPIO10 (G10)

HT16K33 display;                          // 表示オブジェクト
const uint8_t DISPLAY_ADDR = 0x70;        // デフォルト I²C アドレス

unsigned long prevMillis = 0;             // 前回更新時刻
unsigned long startMillis = 0;            // 起動時刻

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
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
    float elapsed = fmod((now - startMillis) / 1000.0f, 1000.0f);

    // 小数点1桁で文字列化
    char buf[6];
    dtostrf(elapsed, 5, 1, buf);
    display.print(buf);
    Serial.println(buf);
  }
}
