#include <Arduino.h>
#include <Adafruit_LiquidCrystal.h>  // Adafruit LCD Backpack ライブラリ
#include <Wire.h>
#include <driver/twai.h>

constexpr uint8_t I2C_SDA = GPIO_NUM_8;   // GPIO8 (G8)
constexpr uint8_t I2C_SCL = GPIO_NUM_10;  // GPIO10 (G10)

// A0～A2 のジャンパなし。実際の I2C アドレスは 0x20。
// Adafruit_LiquidCrystal の引数 0 はアドレスオフセットを表す。
Adafruit_LiquidCrystal lcd(0);

// --- CAN(TWAI) 設定 ---
constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_1;
constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_0;

// VESC からのスロットル ID
constexpr uint32_t CAN_PACKET_SET_DUTY = 0x0;
constexpr uint8_t VESC_ID = 0x7;

// 共有変数
volatile int16_t percent = 0;

// --- CAN受信タスク ---
void can_task(void* arg) {
  twai_message_t msg;

  for (;;) {
    if (twai_receive(&msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
      if (msg.extd && !msg.rtr &&
          msg.identifier == ((CAN_PACKET_SET_DUTY << 8) | VESC_ID) &&
          msg.data_length_code == sizeof(uint32_t)) {
        const int32_t scaled =
            (static_cast<int32_t>(msg.data[0]) << 24) |
            (static_cast<int32_t>(msg.data[1]) << 16) |
            (static_cast<int32_t>(msg.data[2]) << 8) |
            static_cast<int32_t>(msg.data[3]);
        percent = scaled / 1000;
      }
    }
  }
}

// --- LCD表示タスク ---
void display_task(void* arg) {
  TickType_t lastWakeTime = xTaskGetTickCount();

  for (;;) {
    const int16_t local_value = percent;
    char display_str[5];

    if (local_value < 0) {
      snprintf(display_str, sizeof(display_str), "-%3d", abs(local_value));
    } else {
      snprintf(display_str, sizeof(display_str), " %3d", local_value);
    }

    lcd.setCursor(0, 0);
    lcd.print("PWM:            ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(display_str);
    lcd.print(" %");

    Serial.print("Received_PWM/%:");
    Serial.println(local_value);

    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(100));
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  // 16文字×2行のキャラクタLCDを初期化
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  lcd.clear();

  // CAN初期化
  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("Failed to install CAN driver");
    return;
  }
  Serial.println("CAN Driver installed");

  if (twai_start() != ESP_OK) {
    Serial.println("Failed to start CAN");
    return;
  }
  Serial.println("CAN Started");

  xTaskCreate(can_task, "can_task", 4096, nullptr, 5, nullptr);
  xTaskCreate(display_task, "display_task", 4096, nullptr, 5, nullptr);
}

void loop() {
  // メインループは使わず、FreeRTOSタスクで処理する
  vTaskDelay(portMAX_DELAY);
}
