#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_Alphanumeric_Display.h>  // Qwiic Alphanumeric Display ライブラリ
#include <driver/twai.h>

constexpr uint8_t I2C_SDA = GPIO_NUM_8;          // GPIO8  (G8)
constexpr uint8_t I2C_SCL = GPIO_NUM_10;         // GPIO10 (G10)

HT16K33 display;                      // LED 表示オブジェクト
constexpr uint8_t DISPLAY_ADDR = 0x70;    // デフォルト I²C アドレス

// --- CAN(TWAI) 設定 ---
constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_1;
constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_0;
constexpr long       CAN_BAUD   = 500000;     // 500 kbps

// VESC からのスロットル ID
constexpr uint32_t CAN_PACKET_SET_DUTY = 0x0;
constexpr uint8_t UNIT_ID = 0x7;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

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
    if (msg.extd && !msg.rtr && msg.identifier == ((CAN_PACKET_SET_DUTY << 8) | UNIT_ID)
        && msg.data_length_code == sizeof(uint32_t)) {
          uint8_t data[4];
          memcpy(data, msg.data, sizeof(data));
          int32_t scaled;
          scaled = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

      // 表示用文字列作成 (4文字分の幅)
      int32_t percent = scaled / 1000;
      char percent_str[5];
      
      if (percent < 0) {
        // 負の場合
        int32_t abs_val = -percent;
        if (abs_val < 10) {
          sprintf(percent_str, "-  %1d", abs_val);      // -  1
        } else if (abs_val < 100) {
          sprintf(percent_str, "- %2d", abs_val);       // - 12
        } else {
          sprintf(percent_str, "-%3d", abs_val);       // -123
        }
      } else {
        // 正の場合
        if (percent < 10) {
          sprintf(percent_str, "   %1d", percent);    //    1
        } else if (percent < 100) {
          sprintf(percent_str, "  %2d", percent);     //   12
        } else {
          sprintf(percent_str, " %3d", percent);      //  123
        }
      }

      // LED に表示
      display.print(percent_str);
      Serial.printf("Received PWM duty: %d -> \"%s\"\n", percent, percent_str);
    }
  }
}
