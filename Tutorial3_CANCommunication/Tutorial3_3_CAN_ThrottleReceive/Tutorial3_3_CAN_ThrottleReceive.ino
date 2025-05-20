#include <Arduino.h>
#include <SparkFun_Alphanumeric_Display.h> // Qwiic Alphanumeric Display ライブラリ
#include <Wire.h>
#include <driver/twai.h>


constexpr uint8_t I2C_SDA = GPIO_NUM_8;  // GPIO8  (G8)
constexpr uint8_t I2C_SCL = GPIO_NUM_10; // GPIO10 (G10)

HT16K33 display;                       // LED 表示オブジェクト
constexpr uint8_t DISPLAY_ADDR = 0x70; // デフォルト I²C アドレス

// --- CAN(TWAI) 設定 ---
constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_1;
constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_0;
constexpr long CAN_BAUD = 500000; // 500 kbps

// VESC からのスロットル ID
constexpr uint32_t CAN_PACKET_SET_DUTY = 0x0;
constexpr uint8_t VESC_ID = 0x7;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  // ディスプレイ初期化
  if (!display.begin(DISPLAY_ADDR)) {
    Serial.println("Display not found!");
    while (1)
      ;
  }
  display.setBrightness(15);
  display.clear();

  // TWAI ドライバ初期化
  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK ||
      twai_start() != ESP_OK) {
    Serial.println("TWAI init failed");
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
  Serial.println("TWAI ready, waiting for throttle data...");
}

void loop() {
  twai_message_t msg;
  // メッセージをブロック待ち（タイムアウト無し）
  if (twai_receive(&msg, portMAX_DELAY) == ESP_OK) {
    // スロットル ID のフレームだけ処理
    if (msg.extd && !msg.rtr &&
        msg.identifier == ((CAN_PACKET_SET_DUTY << 8) | VESC_ID) &&
        msg.data_length_code == sizeof(uint32_t)) {
      uint8_t data[4];
      memcpy(data, msg.data, sizeof(data));
      int32_t scaled;
      scaled = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

      // 表示用文字列作成 (4文字分の幅)
      int32_t percent = scaled / 1000;
      char display_str[5];

      for (;;) {
        if (percent < 0) {
          sprintf(display_str, "-%3d", abs(percent));
        } else {
          sprintf(display_str, " %3d", percent);
        }
        // LED に表示
        display.print(display_str);
      }

      Serial.print("Received_PWM/%:");
      Serial.println(percent);
    }
  }
}
