/*
  M5Stamp C3U Mate: 2ノード CAN 同期カウンタ送受信スクリプト
  - 100ms ごとに 32bit カウンタをインクリメント
  - 自ノードのカウンタ値を CAN フレームで発出
  - 他ノードのカウンタ値を受信
  - 前回受信値との差が 1 を超える（飛び値）場合はエラー表示
*/

// 共通設定
#include <Arduino.h>
#include <driver/twai.h>

// ノード固有設定（それぞれのボードで書き換える）
static const uint32_t THIS_NODE_ID  = 0x100;  // 自ノードの CAN ID
static const uint32_t PEER_NODE_ID  = 0x200;  // 相手ノードの CAN ID

// CAN ピン・ボーレート
static const gpio_num_t CAN_TX_PIN = GPIO_NUM_1;
static const gpio_num_t CAN_RX_PIN = GPIO_NUM_0;
static const long       CAN_BAUD   = 500000;  // 500 kbps

// 飛び値判定閾値
static const uint32_t JUMP_THRESHOLD = 1;      // 前後差が 1 を超えたら飛び

// 変数
static uint32_t counter      = 0;  // 自カウンタ
static uint32_t lastPeerVal  = 0;  // 前回受信値

// TWAI 初期化
void setupCAN() {
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
    CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL
  );
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK ||
      twai_start() != ESP_OK) {
    Serial.println("Error: TWAI init failed");
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
  }
}

// 10ms 周期タスク (CAN送受信・カウンタ更新), 100ms 周期 (シリアル表示)
void periodicTask(void* pvParameters) {
  TickType_t period   = pdMS_TO_TICKS(10);
  TickType_t lastWake = xTaskGetTickCount();
  static int serial_print_countdown = 0;

  for (;;) {
    // 1) カウンタインクリメント
    counter++;

    // 100msごと (10周期ごと) にシリアル表示
    if (serial_print_countdown == 0) {
      Serial.printf("Counter value: %u\n", counter);
      serial_print_countdown = 9; // Reset countdown (0-9 makes 10 cycles)
    } else {
      serial_print_countdown--;
    }

    // 2) カウンタ値を CAN フレームで送信
    twai_message_t msg = {};
    msg.identifier       = THIS_NODE_ID;
    msg.extd             = false;
    msg.rtr              = false;
    msg.data_length_code = sizeof(counter);
    memcpy(msg.data, &counter, sizeof(counter));
    twai_transmit(&msg, pdMS_TO_TICKS(10));

    // 3) 他ノードのフレームを受信（タイムアウト 10ms）
    twai_message_t rx;
    if (twai_receive(&rx, period) == ESP_OK) {
      if (!rx.extd && !rx.rtr && rx.identifier == PEER_NODE_ID
          && rx.data_length_code == sizeof(uint32_t)) {
        uint32_t peerVal;
        memcpy(&peerVal, rx.data, sizeof(peerVal));

        // 4) 飛び値判定
        uint32_t diff = (peerVal > lastPeerVal)
                        ? (peerVal - lastPeerVal)
                        : (lastPeerVal - peerVal);
        if (diff > JUMP_THRESHOLD) {
          Serial.printf("Data Error: jump detected! prev=%u, now=%u\n",
                        lastPeerVal, peerVal);
        }
        lastPeerVal = peerVal;
      }
    } else {
      Serial.println("Warning: no frame received from peer");
    }

    // 次周期まで待機
    vTaskDelayUntil(&lastWake, period);
  }
}

void setup() {
  Serial.begin(115200);
  setupCAN();

  // FreeRTOS タスク生成
  xTaskCreatePinnedToCore(
    periodicTask,   // タスク関数
    "PeriodicCAN",  // タスク名
    4096,           // スタックサイズ
    nullptr,        // 引数
    1,              // 優先度
    nullptr,        // タスクハンドル
    0               // コア ID (ESP32-C3 はシングルコア)
  );
}

void loop() {
  // 何もしない
}
