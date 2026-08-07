#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LiquidCrystal.h>  // Adafruit LCD Backpack ライブラリ

constexpr uint8_t I2C_SDA = GPIO_NUM_8;   // GPIO8 (G8)
constexpr uint8_t I2C_SCL = GPIO_NUM_10;  // GPIO10 (G10)

// A0～A2 のジャンパなし。実際の I2C アドレスは 0x20。
// Adafruit_LiquidCrystal の引数 0 はアドレスオフセットを表す。
Adafruit_LiquidCrystal lcd(0);

unsigned long startMillis = 0;

// 100ms ごとに経過時間を計算して表示するタスク
void displayTask(void* pvParameters) {
  TickType_t lastWakeTime = xTaskGetTickCount();
  constexpr TickType_t period = pdMS_TO_TICKS(100);

  for (;;) {
    const unsigned long now = millis();
    const float elapsed = (now - startMillis) / 1000.0f;

    lcd.setCursor(0, 1);
    lcd.print("                ");  // 前の表示を消す
    lcd.setCursor(0, 1);
    lcd.print(elapsed, 1);
    lcd.print(" s");
    Serial.println(elapsed, 1);

    // 次の周期まで待機し、周期を維持する
    vTaskDelayUntil(&lastWakeTime, period);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  // 16文字×2行のキャラクタLCDを初期化
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Elapsed time:");

  startMillis = millis();

  xTaskCreate(
    displayTask,    // タスク関数
    "DisplayTask",  // タスク名
    2048,            // スタックサイズ（バイト）
    nullptr,         // 引数
    1,               // 優先度
    nullptr          // タスクハンドル
  );
}

void loop() {
  // メインループは使わない
}
