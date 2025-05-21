/*
  M5Stamp C3U Mate: 2ノード CAN 同期カウンタ送受信スクリプト
  - 10ms ごとに 32bit カウンタをインクリメントし、CANフレームで自ノードのカウンタ値を発出します。
  - 100ms ごとに自ノードのカウンタ値をシリアルポートに表示します。
  - 他ノードからのカウンタ値（CANフレーム）を受信します。
  - 受信したカウンタ値と前回受信値との差が一定値 (JUMP_THRESHOLD) を超える（飛び値）場合は、エラーをシリアルポートに表示します。
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
    Serial.println("Error: TWAI init failed"); // TWAIドライバの初期化または開始に失敗しました。
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
  }
}

// periodicTask: CANメッセージの周期的送受信と内部カウンタの管理を行うタスク
// このタスクはFreeRTOSによってバックグラウンドで定期的に実行されます。
// 10ms周期でカウンタを更新してCANで送信し、相手ノードからのCANメッセージを受信します。
// また、100ms周期でカウンタの値をシリアルポートに出力します。
void periodicTask(void* pvParameters) {
  TickType_t period   = pdMS_TO_TICKS(10);
  TickType_t lastWake = xTaskGetTickCount(); // 現在の時刻（ティック数）を取得。vTaskDelayUntilの基準時刻として使用。
  static int serial_print_countdown = 0;

  for (;;) {
    // 1) カウンタインクリメント
    counter++;

    // 100msごと (10周期ごと) にシリアル表示
    if (serial_print_countdown == 0) {
      Serial.printf("Counter value: %u\n", counter);
      serial_print_countdown = 9; // カウントダウンをリセットします (次に0になるまで10サイクル分)。
    } else {
      serial_print_countdown--;
    }

    // 2) カウンタ値を CAN フレームで送信
    twai_message_t msg = {};          // 送信するCANメッセージ構造体を初期化
    msg.identifier       = THIS_NODE_ID; // このノードのCAN IDを設定
    msg.extd             = false;        // 標準IDフォーマット (11ビットID) を使用 (trueなら拡張ID)
    msg.rtr              = false;        // データフレームを指定 (trueならリモートフレーム)
    msg.data_length_code = sizeof(counter); // データ長をカウンタ変数のサイズ (4バイト) に設定
    memcpy(msg.data, &counter, sizeof(counter)); // メッセージのデータ部にカウンタの値をコピー
    
    // 設定したCANメッセージを送信します。pdMS_TO_TICKS(10) は送信タイムアウト時間 (10ms) です。
    twai_transmit(&msg, pdMS_TO_TICKS(10));

    // 3) 他ノードのフレームを受信（タイムアウト 10ms）
    twai_message_t rx;
    if (twai_receive(&rx, period) == ESP_OK) {
      // 受信したメッセージが期待する条件を満たしているか確認します:
      // - 標準IDであるか (!rx.extd)
      // - データフレームであるか (!rx.rtr)
      // - IDが相手ノードのものか (rx.identifier == PEER_NODE_ID)
      // - データ長が期待通りか (rx.data_length_code == sizeof(uint32_t))
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
                        lastPeerVal, peerVal); // 受信データに予期せぬ飛び（カウンタ値の不連続）が検出されました。
        }
        lastPeerVal = peerVal;
      }
    } else {
      Serial.println("Warning: no frame received from peer"); // 相手ノードから期待時間内にCANフレームを受信できませんでした。
    }

    // 次周期まで待機
    // vTaskDelayUntilは、指定した時刻 (lastWake + period) まで現在のタスクをブロック（スリープ）させます。
    // これにより、ループ内の処理時間に多少のばらつきがあっても、タスク全体の実行周期を正確に保つことができます。
    // (例: periodが10msなら、lastWake から正確に10ms後まで待機し、次の処理を開始する)
    // lastWake はこの関数内で自動的に次の起床時刻に更新されます。
    // delay()関数と異なり、他のタスクの実行状況の影響を受けにくいのが特徴です。
    vTaskDelayUntil(&lastWake, period);
  }
}

void setup() {
  Serial.begin(115200);
  setupCAN();

  // FreeRTOS タスク生成
  // periodicTask関数を "PeriodicCAN" という名前のタスクとして、並行処理を開始します。
  // 第3引数 (4096) はタスクに割り当てるスタックメモリのサイズ（バイト単位）です。タスクが必要とするメモリ量に応じて調整します。
  // 第4引数 (nullptr) はタスク関数に渡すパラメータですが、このタスクでは使用しません。
  // 第5引数 (1) はタスクの優先度です。数値が大きいほど優先度が高くなります（ESP32では通常0が最低、configMAX_PRIORITIES-1が最高）。
  // 第6引数 (nullptr) はタスクハンドルを格納する変数のポインタですが、ここでは使用しません。タスクを後から操作する場合に利用します。
  // 第7引数 (0) はタスクを実行するCPUコアのIDです。ESP32-C3はシングルコアなので0を指定します。デュアルコアCPUの場合は0または1を指定できます。
  // FreeRTOSのタスクについてさらに詳しく知りたい場合は、公式ドキュメントやウェブ上の解説記事を参照してください。
  // (例: 「FreeRTOS タスクとは」「FreeRTOS タスクスケジューリング」などで検索)
  // ESP-IDFの公式ドキュメント(英語): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
  xTaskCreatePinnedToCore(
    periodicTask,   // タスク関数: 実行する関数のポインタ
    "PeriodicCAN",  // タスク名: デバッグ時に役立つタスクの名称（文字列）
    4096,           // スタックサイズ: タスクが使用するスタックのサイズ（バイト単位）
    nullptr,        // 引数: タスク関数に渡すパラメータのポインタ (void*)
    1,              // 優先度: タスクの優先順位
    nullptr,        // タスクハンドル: 生成されたタスクのハンドルを格納する変数のポインタ
    0               // コア ID: タスクを実行するCPUコア (0 または 1、ESP32-C3では0)
  );
}

void loop() {
  // 何もしない
  // FreeRTOSを使用する場合、メインループ(loop関数)は通常、タスクの初期化や監視など、
  // FreeRTOSスケジューラが管理しない処理に使用されるか、あるいは空のままにします。
  // このスケッチでは全ての主要な処理が periodicTask で行われるため、loopは空です。
  vTaskDelay(portMAX_DELAY); // loopタスクを無期限にブロックし、他のタスクにCPU時間を譲る
}
