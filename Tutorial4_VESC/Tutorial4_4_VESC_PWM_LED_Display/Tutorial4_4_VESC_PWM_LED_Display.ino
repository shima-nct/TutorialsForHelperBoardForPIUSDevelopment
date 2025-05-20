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

// 共有変数
volatile int16_t percent = 0;

//--- CAN受信タスク ---//
void can_task(void *arg) {
  twai_message_t msg;
  for (;;) {
    if (twai_receive(&msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
      if (msg.extd && !msg.rtr &&
          msg.identifier == ((CAN_PACKET_SET_DUTY << 8) | VESC_ID) &&
          msg.data_length_code == sizeof(uint32_t)) {
        uint8_t data[4];
        memcpy(data, msg.data, sizeof(data));
        int32_t scaled;
        scaled = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        percent = scaled / 1000;
      }
    }
  }
}

//--- Alphanumeric表示タスク ---//
void display_task(void *arg) {
  int16_t local_value;
  char display_str[5];
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    local_value = percent; // アトミックに読み出し
    if (local_value < 0) {
      sprintf(display_str, "-%3d", abs(local_value));
    } else {
      sprintf(display_str, " %3d", local_value);
    }
    // LED に表示
    display.print(display_str);
    
    Serial.print("Received_PWM/%:");
    Serial.println(local_value);
    // 次の更新まで 100ms 間隔（この間隔で表示値が更新される）
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  // Qwiic Alphanumeric Display初期化
  if (display.begin() == false) {
    Serial.println("Qwiic Alphanumeric Display not detected");
    while (1)
      ;
  }
  display.clear();
  display.setBrightness(10); // 0-15の範囲で明るさを設定

  // CAN初期化
  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // CAN ドライバーのインストール
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("CAN Driver installed");
  } else {
    Serial.println("Failed to install CAN driver");
    return;
  }

  // CAN 開始
  if (twai_start() == ESP_OK) {
    Serial.println("CAN Started");
  } else {
    Serial.println("Failed to start CAN");
    return;
  }

  // タスク作成
  xTaskCreate(can_task, "can_task", 4096, NULL, 5, NULL);
  xTaskCreate(display_task, "display_task", 4096, NULL, 5, NULL);
}

void loop() {
  // メインループは空です - すべての処理はFreeRTOSタスクで行われます
  vTaskDelay(portMAX_DELAY);
}
