#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/twai.h"

//--- ハードウェア設定 ---//
// TWAI (CAN) ピン
#define CAN_TX_IO GPIO_NUM_21
#define CAN_RX_IO GPIO_NUM_22

// 7セグメント用ピン（共通カソード4桁＋セグメント8本）
const gpio_num_t digit_pins[4]   = { GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19 };
const gpio_num_t segment_pins[8] = {
    GPIO_NUM_2,  // a
    GPIO_NUM_4,  // b
    GPIO_NUM_5,  // c
    GPIO_NUM_15, // d
    GPIO_NUM_13, // e
    GPIO_NUM_12, // f
    GPIO_NUM_14, // g
    GPIO_NUM_27  // dp (未使用なら省略可)
};

// 数字 0–9 のセグメントパターン（a–g のビットマスク）
const uint8_t digit_map[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

// 受信した PWM 値を格納する変数（0〜9999 表示対応）
static volatile uint16_t pwm_value = 0;

//--- CAN 受信タスク ---//
void can_rx_task(void *arg)
{
    twai_message_t msg;
    while (1) {
        if (twai_receive(&msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
            // 例: データ長 2 バイトで uint16_t の PWM 値を送信している想定
            if (msg.data_length_code == 2) {
                uint16_t v = (msg.data[0] << 8) | msg.data[1];
                if (v <= 9999) {
                    pwm_value = v;
                }
            }
        }
    }
}

//--- 7セグ表示タスク ---//
void display_task(void *arg)
{
    uint16_t local_value;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        local_value = pwm_value;  // アトミックに読み出し
        // 4 桁分取り出す
        uint8_t digits[4] = {
            (local_value / 1000) % 10,
            (local_value /  100) % 10,
            (local_value /   10) % 10,
            (local_value        ) % 10
        };
        for (int d = 0; d < 4; d++) {
            // まず全桁 OFF
            for (int i = 0; i < 4; i++) {
                gpio_set_level(digit_pins[i], 1);
            }
            // セグメント出力
            uint8_t pattern = digit_map[digits[d]];
            for (int s = 0; s < 7; s++) {
                // common cathode: 1 で消灯, 0 で点灯
                gpio_set_level(segment_pins[s], (pattern & (1 << s)) ? 0 : 1);
            }
            // 小数点(dp)は消灯
            gpio_set_level(segment_pins[7], 1);

            // 該当桁を ON
            gpio_set_level(digit_pins[d], 0);

            // 1ms 表示
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        // 次の更新まで 50ms 間隔（この間隔で表示値が更新される）
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    // 1) 7セグ用 GPIO 初期化
    for (int i = 0; i < 4; i++) {
        gpio_set_direction(digit_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(digit_pins[i], 1);
    }
    for (int i = 0; i < 8; i++) {
        gpio_set_direction(segment_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(segment_pins[i], 1);
    }

    // 2) TWAI (CAN) ドライバ初期化
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_IO, CAN_RX_IO, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config  = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config  = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start();

    // 3) タスク生成
    xTaskCreatePinnedToCore(can_rx_task, "can_rx", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(display_task, "disp",   4096, NULL, 5, NULL, 1);
}
