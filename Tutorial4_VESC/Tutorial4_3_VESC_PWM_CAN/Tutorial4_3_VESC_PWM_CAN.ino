#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // FreeRTOSのミューテックス機能を利用するために必要なヘッダファイル
#include <Arduino.h>
#include <driver/twai.h>
#include <esp32-hal-adc.h> // ESP32-C3 でも analogSetPinAttenuation が使えます

// --- ADC 設定 ---
constexpr gpio_num_t HALL_PIN = GPIO_NUM_3; // スロットルセンサー入力
constexpr int ADC_RESOLUTION = 4095;        // 12bit

// --- CAN (TWAI) 設定 ---
constexpr gpio_num_t CAN_TX_PIN = GPIO_NUM_1;
constexpr gpio_num_t CAN_RX_PIN = GPIO_NUM_0;
constexpr long CAN_BAUD = 500000; // 500 kbps

// --- VESC CAN プロトコル ---
constexpr uint32_t CAN_PACKET_SET_DUTY = 0x0; // デューティコマンド識別子
constexpr uint8_t VESC_ID =
    0x7; // VESC ID: VESC Tool/App Settings/General/VESC ID

// --- スロットル平均化設定 ---
// constexpr int SAMPLES_COUNT = 20; // 20msの間に20サンプル（1ms毎）
constexpr int SAMPLES_COUNT = 20; // スロットル入力の平均を取るためのサンプル数。20サンプル（1ms毎に取得）の移動平均を計算します。
// 値を大きくすると入力が滑らかになりますが、制御の応答は少し遅くなる傾向があります。
// static int throttle_samples[SAMPLES_COUNT];
static int throttle_samples[SAMPLES_COUNT]; // スロットル入力の過去の値を格納する配列（リングバッファとして使用）。この配列の値を合計して平均値を求めます。
static int sample_index = 0;
static float HALL_MIN = 0.29f; // スロットル最小値 (29%)
static float HALL_MAX = 0.92f; // スロットル最大値 (92%)

// static SemaphoreHandle_t throttleMutex = NULL; // Mutex for throttle data
static SemaphoreHandle_t throttleMutex = NULL; // スロットル関連データ (throttle_samples配列 と sample_index変数) を保護するためのミューテックスハンドルです。
// これらのデータは複数のタスクからアクセスされるため、データ競合（同時に書き換えようとして値が壊れるなど）を防ぐためにミューテックスによる排他制御を行います。

// TWAI の一般設定・タイミング設定・フィルタ設定
constexpr twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
constexpr twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
constexpr twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

// VESCコントロールタスク: 平均化されたスロットル値を読み取り、VESC制御用のCANメッセージを定期的に送信するタスク
// 20ms周期で実行され、スロットル値に基づいて計算されたデューティ比をVESCに指令します。
void vescControlTask(void *pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(20); // 20ms間隔

  // タスクの初期化: 初回の起床時刻を現在の時刻に設定
  xLastWakeTime = xTaskGetTickCount();

  while (1) {
    // スロットル入力の平均値を計算します。
    // アナログセンサーからの入力値は、電源ノイズやセンサー自体の特性により細かく変動することがあります。
    // このような入力のブレを抑え、より安定した制御を行うために、複数回読み取った値の平均を使用します。
    // ここでは、直近の `SAMPLES_COUNT` 回分の入力値の合計を求め、その後平均値を算出します（単純移動平均）。
    int32_t sum = 0; // スロットル値の合計を格納する変数。ループ前に必ず0で初期化します。
    // throttle_samples 配列を読み取る前にミューテックスを取得します。
    // これにより、throttleReadTaskが配列を更新している途中で読み取ってしまうことを防ぎます。
    if (xSemaphoreTake(throttleMutex, portMAX_DELAY) == pdTRUE) { // pdTRUE は取得成功を示す
      for (int i = 0; i < SAMPLES_COUNT; i++) {
        sum += throttle_samples[i];
      }
      // 読み取りが完了したので、ミューテックスを解放します。
      // xSemaphoreGiveを実行すると、待機していた他のタスクがミューテックスを取得できるようになります。
      xSemaphoreGive(throttleMutex);
    } else {
      // Failed to get mutex, perhaps log an error or skip this cycle
      // For this tutorial, we assume it's always acquired with portMAX_DELAY
      // If not, sum will remain 0, leading to 0 PWM.
      // ミューテックスの取得に失敗した場合の処理（通常は起こりにくいが、堅牢な設計のためには考慮する）
      Serial.println("Error: vescControlTask failed to take throttleMutex");
      // Optionally, continue to ensure vTaskDelayUntil is called
      // continue;
    }

    // Calculations and CAN transmission should be outside the mutex lock
    float percent = (sum / (float)(SAMPLES_COUNT * ADC_RESOLUTION));

    // 実際の範囲から0-100%に正規化
    float pwm = (percent - HALL_MIN) / (HALL_MAX - HALL_MIN);
    pwm = constrain(pwm, 0.0f, 1.0f); // 0.0–1.0
    int32_t scaled = (int32_t)(pwm * 100000.0f);

    // CAN メッセージ作成
    twai_message_t msg;
    memset(&msg, 0, sizeof(msg)); // メッセージ構造体をゼロクリア

    // CANメッセージのIDを設定します。
    // VESCのCAN通信プロトコルでは、多くの場合、コマンドの種類を示すIDと、
    // 制御対象のVESCユニットのIDを組み合わせてメッセージIDを構成します。
    // ここでは `CAN_PACKET_SET_DUTY` (デューティ比設定コマンド、値は0) を上位8ビットにシフトし、
    // VESCユニットのID (`VESC_ID`) と論理和を取ることで、最終的なメッセージIDとしています。
    // 例: VESC_IDが7の場合、IDは (0 << 8) | 7 = 0x007 となります。
    // VESCのドキュメントによると、デューティ比設定コマンドのIDは単にVESC IDそのものになる場合があります。
    // このコードではコマンドIDとVESC IDを明確に分けていますが、送信されるIDはVESC IDと同じになります。
    msg.identifier = (CAN_PACKET_SET_DUTY << 8) | VESC_ID;

    // このメッセージが拡張IDフォーマット (29ビットID) であることを示します。
    // VESCの標準的なCAN通信は拡張IDを使用します。
    msg.flags = TWAI_MSG_FLAG_EXTD;

    // VESCに送信するデータ (scaled: 32ビット整数) をビッグエンディアン形式のバイト配列に変換します。
    // ビッグエンディアンとは、数値の最上位バイト（MSB）がメモリの先頭アドレスに格納される方式です。
    // (例: 0x12345678 の場合、buf[0]=0x12, buf[1]=0x34, buf[2]=0x56, buf[3]=0x78 となる)
    // VESCのCANプロトコルでは、多くのコマンドデータがこの形式で送信される必要があります。
    uint8_t buf[4] = {(uint8_t)(scaled >> 24), (uint8_t)(scaled >> 16),
                      (uint8_t)(scaled >> 8), (uint8_t)(scaled)};
    // 作成したバイト配列をCANメッセージのデータフィールドにコピーします。
    memcpy(msg.data, buf, sizeof(buf));
    // データ長をセットします (ここでは32ビット整数なので4バイト)。
    msg.data_length_code = sizeof(scaled);

    Serial.print("PWM/%:");
    Serial.println(scaled / 1000);

    // 送信（送信待ち最大 10 ms）
    if (twai_transmit(&msg, pdMS_TO_TICKS(10)) != ESP_OK) {
      Serial.println("TWAI transmit failed"); // CANメッセージの送信に失敗した場合のエラー表示
    }

    // 次の周期まで待機
    // 20ms周期を正確に保つため、vTaskDelayUntilを使用します。
    // この関数は、指定された起床時刻 (xLastWakeTime) になるまでタスクをブロックします。
    // ループ内の処理時間が変動しても、周期の精度が維持されます。
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// スロットル読み取りタスク: スロットルセンサーからのアナログ値を高速 (1ms周期) で読み取り、
// 平均化処理のために `throttle_samples` 配列に格納するタスク。
void throttleReadTask(void *pvParameters) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(1); // 1ms間隔

  // タスクの初期化: 初回の起床時刻を現在の時刻に設定
  xLastWakeTime = xTaskGetTickCount();

  while (1) {
    // ミューテックスを取得して、共有データ (throttle_samples, sample_index) へのアクセスを保護します。
    // xSemaphoreTakeは、指定されたミューテックスが利用可能になるまで待機し（ここではportMAX_DELAYで無期限待機）、取得します。
    // これにより、他のタスク (vescControlTask) が同時に throttle_samples を読み取ろうとすることを防ぎ、データの整合性を保ちます。
    if (xSemaphoreTake(throttleMutex, portMAX_DELAY) == pdTRUE) { // pdTRUE は取得成功を示す
      throttle_samples[sample_index] = analogRead(HALL_PIN);
      sample_index = (sample_index + 1) % SAMPLES_COUNT;
      // 共有データの操作が完了したので、ミューテックスを解放します。
      // xSemaphoreGiveを実行すると、待機していた他のタスクがミューテックスを取得できるようになります。
      // ミューテックスは、保護するクリティカルセクション（共有リソースへのアクセス区間）を最小限にし、速やかに解放することが重要です。
      xSemaphoreGive(throttleMutex);
    }
    // 次の周期まで待機
    // 1ms周期を正確に保つため、vTaskDelayUntilを使用します。
    // これにより、高頻度でのセンサー読み取りが安定して行われます。
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void setup() {
  Serial.begin(115200);

  // スロットルデータアクセス用ミューテックスの作成
  // xSemaphoreCreateMutexは、ミューテックス（相互排他セマフォ）を作成するFreeRTOS APIです。
  // ミューテックスは、複数のタスクが共有リソース（ここでは throttle_samples 配列と sample_index 変数）へ同時にアクセスすることを防ぎ、
  // データの一貫性を保つために使用されます。これにより、タスク間の安全なデータ共有が可能になります。
  // FreeRTOSのミューテックスについてさらに詳しく知りたい場合は、公式ドキュメントやウェブ上の解説記事を参照してください。
  // (例: 「FreeRTOS ミューテックスとは」「FreeRTOS排他制御」などで検索)
  // ESP-IDFの公式ドキュメント(英語): https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html#mutexes
  throttleMutex = xSemaphoreCreateMutex();
  // ミューテックスの作成に成功したかを確認します。
  // もしNULLが返ってきた場合、メモリ不足などでミューテックスの作成に失敗しています。
  // このような場合、プログラムは期待通りに動作しない可能性が高いため、エラー処理を行います。
  if (throttleMutex == NULL) {
      Serial.println("Error: Failed to create throttleMutex"); // エラーメッセージをシリアルに出力
      while(1); // 無限ループでプログラムを停止（エラー発生時は安全のため、システムを停止させる）
  }

  // ADC 減衰設定（11dB で約0–2.5V測定可）
  analogSetPinAttenuation(HALL_PIN, ADC_11db);

  // スロットルサンプル配列の初期化
  for (int i = 0; i < SAMPLES_COUNT; i++) {
    throttle_samples[i] = 0;
  }

  // TWAI ドライバのインストール
  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("Failed to install TWAI driver");
    while (1)
      ;
  }
  // TWAI ドライバ開始
  if (twai_start() != ESP_OK) {
    Serial.println("Failed to start TWAI driver");
    while (1)
      ;
  }

  Serial.println("TWAI (CAN) initialized");

  // FreeRTOSタスクの作成
  // xTaskCreate は FreeRTOS で新しいタスクを生成するための関数です。
  // これにより、複数の処理（この場合はスロットル読み取りとVESC制御）を並行して実行できます。
  // 各タスクは独立したスタック空間を持ち、OSスケジューラによって優先度に基づいて実行時間が割り当てられます。
  // (各パラメータの詳細は Tutorial3_... の xTaskCreatePinnedToCore のコメントを参照)

  // FreeRTOSタスクの作成 (スロットル読み取りタスク)
  // throttleReadTask関数を "ThrottleRead" という名前のタスクとして並行処理を開始します。
  // 優先度2で実行され、1ms周期でスロットル値を読み取ります。センサーデータの取得は比較的高頻度で行うため、制御タスクより若干高い優先度を設定しています。
  xTaskCreate(throttleReadTask,    // タスクとして実行する関数
              "ThrottleRead",      // タスク名 (デバッグ用)
              2048,                // スタックサイズ (バイト単位)
              NULL,                // タスクに渡すパラメータ (今回はなし)
              2,                   // タスクの優先度 (数値が大きいほど高優先度)
              NULL);               // タスクハンドル (今回は使用せず)

  // FreeRTOSタスクの作成 (VESC制御タスク)
  // vescControlTask関数を "VESCControl" という名前のタスクとして並行処理を開始します。
  // 優先度1で実行され、20ms周期でCANメッセージを送信します。こちらは制御指令の送信で、スロットル読み取りよりは低頻度です。
  xTaskCreate(vescControlTask,     // タスクとして実行する関数
              "VESCControl",       // タスク名
              2048,                // スタックサイズ
              NULL,                // パラメータ
              1,                   // 優先度
              NULL);               // タスクハンドル
}

void loop() {
  // メインループ (Arduino の loop() 関数) は、FreeRTOSタスクが動作している間は実質的に使用されません。
  // FreeRTOSスケジューラがタスクの実行管理を行うため、loop() 内の処理は通常実行機会がほとんどありません。
  // vTaskDelay(portMAX_DELAY) を呼び出すことで、loopタスク（メインタスク）を無期限スリープ状態にし、
  // CPUリソースを他のアプリケーションタスク（throttleReadTask や vescControlTask）に完全に譲渡します。
  // これがFreeRTOS環境での一般的な loop() の実装方法です。
  vTaskDelay(portMAX_DELAY);
}
