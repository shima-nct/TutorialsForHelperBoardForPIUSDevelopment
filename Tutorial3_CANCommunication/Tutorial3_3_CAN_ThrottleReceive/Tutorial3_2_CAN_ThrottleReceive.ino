#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_Alphanumeric_Display.h>  // Qwiic Alphanumeric Display ライブラリ
#include <driver/twai.h>

HT16K33 display;                      // LED 表示オブジェクト
const uint8_t DISPLAY_ADDR = 0x70;    // デフォルト I²C アドレス

// --- CAN(TWAI) 設定 ---
static const gpio_num_t CAN_TX_PIN = GPIO_NUM_5;
static const gpio_num_t CAN_RX_PIN = GPIO_NUM_4;
static const long       CAN_BAUD   = 500000;     // 500 kbps

// VESC からのスロットル ID
static const uint32_t VESC_THROTTLE_ID = 0x01;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // ディスプレイ初期化
  if (!display.begin(DISPLAY_ADDR)) {
    Serial.println("Display not found!");
    while (1);
  }
  display.setBrightness(15);
  display.clear();

  // TWAI ドライバ初期化
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
    CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL
  );
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK ||
      twai_start() != ESP_OK) {
    Serial.println("TWAI init failed");
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
  }
  Serial.println("TWAI ready, waiting for throttle data...");
}

void loop() {
  twai_message_t msg;
  // メッセージをブロック待ち（タイムアウト無し）
  if (twai_receive(&msg, portMAX_DELAY) == ESP_OK) {
    // スロットル ID のフレームだけ処理
    if (!msg.extd && !msg.rtr && msg.identifier == VESC_THROTTLE_ID
        && msg.data_length_code == sizeof(float)) {
      float throttle;
      memcpy(&throttle, msg.data, sizeof(throttle));  // IEEE754 リトルエンディアン

      // 表示用文字列作成 (幅 4, 小数点以下 2 桁)
      char buf[5];
      dtostrf(throttle, 4, 2, buf);

      // LED に表示
      display.print(buf);
      Serial.printf("Received throttle: %.3f -> \"%s\"\n", throttle, buf);
    }
  }
}
