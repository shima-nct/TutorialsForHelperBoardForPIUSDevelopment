# M5Stamp C3U & VESCヘルパーボード チュートリアル

## 1. はじめに
本シリーズでは、M5Stamp C3U Mateマイコンボードと、本教材のために設計された「VESCヘルパーボード」を使用し、組み込みシステムの基礎から応用までを実践的に学びます。
センサーからのアナログ入力処理、ディスプレイへの情報表示、モーター制御で広く利用されるCAN通信、リアルタイム処理のためのFreeRTOSの活用、そして高性能モーターコントローラーであるVESCとの連携など、多岐にわたる技術要素に触れます。
本チュートリアルは、組み込みシステムやInternet of Things (モノのインターネット) デバイス開発に興味のある方を対象としています。

## 2. 本チュートリアルシリーズで学ぶこと
このシリーズでは、以下の4つのチュートリアルを通して、段階的に知識とスキルを習得します。

### Tutorial 1: サムスロットルによるアナログ入力とデータ処理
本チュートリアルでは、サムスロットル（親指で操作する可変抵抗器）の仕組みを理解し、そのアナログ出力をM5Stamp C3Uで読み取ります。読み取ったデータは不安定な場合があるため、移動平均フィルタによるデータ平滑化処理を学びます。また、より高度な処理の準備として、リアルタイムオペレーティングシステム (RTOS) であるFreeRTOSのタスクの概念に触れ、基本的な使い方を体験します。

### Tutorial 2: I2Cディスプレイによる情報表示
Tutorial 1で処理したスロットル値を、7セグメントLEDディスプレイに表示する方法を学びます。ディスプレイとの通信にはI2Cというシリアル通信プロトコルを使用します。I2C通信の基本的な仕組みと、Arduinoでのライブラリを使った制御方法を習得します。ここでもFreeRTOSのタスクを活用し、定期的なデータ更新と表示処理を実現します。

### Tutorial 3: CAN通信の基礎と実践
産業用ネットワークや自動車制御で広く利用されているCAN (Controller Area Network) 通信の基礎を学びます。CAN通信の物理層やデータリンク層の概要を理解し、ESP32に搭載されているTWAI (Two-Wire Automotive Interface) ドライバを使って、2つのM5Stamp C3U Mateボード間で実際にデータを送受信します。具体的には、同期カウンタシステムを実装し、CAN通信によるノード間の協調動作を体験します。

### Tutorial 4: VESCを使ったモーター制御
BLDCモーター制御で人気の高いオープンソースコントローラー「VESC」の基本的な使い方と、M5Stamp C3Uからの制御方法を学びます。VESC Toolを使った設定方法、PWM信号による基本的なモーター駆動、電流制限などの安全機能について触れます。さらに、Tutorial 3で学んだCAN通信を使ってVESCを制御し、Tutorial 1で扱ったスロットルからの入力値を反映させてモーターの回転数を変化させる、より実践的なシステムを構築します。

## 3. 必要なもの
### 3.1. ハードウェア
本チュートリアルの実施には、以下のハードウェアが必要です。
*   M5Stamp C3U Mate (または M5Stamp C3 Mate)
*   VESCヘルパーボード (本教材の著者が設計・配布したもの)
*   USB Type-C ケーブル (M5Stamp C3U Mate用)
*   Qwiicケーブル (SparkFun Qwiic Alphanumeric Display接続用)
*   SparkFun Qwiic Alphanumeric Display (赤色、他の色でも可)
*   VESC互換モーターコントローラー (CAN通信機能付き)
*   BLDCモーター
*   電源 (VESC用、モーター駆動用)
*   CANトランシーバー搭載マイコンボード2台目 (M5Stamp C3U Mateなど、CAN通信チュートリアル用)

### 3.2. ソフトウェア
*   Arduino IDE (バージョン 2.x.x を推奨。最新の安定版をご利用ください)
*   VESC Tool (VESC設定用。使用するVESCのファームウェアに対応したバージョンをご利用ください)
*   Git および GitHub CLI (教材ダウンロード用)

## 4. 開発環境の準備
### 4.1. 教材のダウンロード
本チュートリアルの教材はGitHubで公開されています。以下の手順でダウンロードしてください。

**方法1: GitHub CLIを利用する (推奨)**
1.  GitHub CLIをインストールします。手順は[GitHub CLIの公式ドキュメント](https://cli.github.com/manual/installation)を参照してください。
2.  ターミナルまたはコマンドプロンプトで以下のコマンドを実行します。リポジトリのURLは、本教材がホストされている実際のURLに置き換えてください。
    ```powershell
    gh repo clone <リポジトリのURL> M5Stamp_VESC_Tutorial
    cd M5Stamp_VESC_Tutorial
    ```

**方法2: ZIPファイルとしてダウンロードする**
1.  本教材のGitHubリポジトリページにアクセスします。
2.  「Code」ボタンをクリックし、「Download ZIP」を選択します。
3.  ダウンロードしたZIPファイルを任意の場所に解凍します。

### 4.2. Arduino IDEのインストールと設定
#### 4.2.1. Arduino IDE本体のインストール
Arduinoプログラムを作成・書き込みするための統合開発環境(IDE)です。
以下の公式サイトからご自身のOSに合った最新版 (バージョン2.x.x推奨) をダウンロードし、インストールしてください。
*   [Arduino IDE ダウンロードページ](https://www.arduino.cc/en/software)

インストール後、一度起動して基本的な設定（言語など）を確認することを推奨します。

#### 4.2.2. M5Stackボードサポートの追加
M5Stamp C3U Mate をArduino IDEで使用するためには、M5Stackのボード情報を追加する必要があります。
1.  Arduino IDEを開き、「ファイル」メニュー (Windows) または「Arduino IDE」メニュー (macOS) から「設定...」を選択します。
2.  「追加のボードマネージャのURL」の欄に以下のURLを入力し、「OK」をクリックします。
    ```
    https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
    ```
    <img src="https://user-images.githubusercontent.com/88629497/191185858-33084258-2e34-45e1-b97f-47f17593efdf.png" width="600">
    *図: Arduino IDE 設定画面でのボードマネージャURL入力*
3.  「ツール」メニューから「ボード」 > 「ボードマネージャ...」を選択します。
4.  検索欄に「M5Stack」と入力します。
5.  表示された「M5Stack Boards」を選択し、「インストール」をクリックします。（既にインストール済みの場合はバージョンが表示されます。最新版への更新もここで行えます。）
    <img src="https://user-images.githubusercontent.com/88629497/191186116-20b87062-f71a-497f-978d-912f0299079e.png" width="600">
    *図: Arduino IDE ボードマネージャでのM5Stackボード検索とインストール*
6.  インストール完了後、「閉じる」をクリックします。

#### 4.2.3. ライブラリのインストール
本チュートリアルでは、以下のライブラリを使用します。
*   **SparkFun Qwiic Alphanumeric Display Library:** 英数字ディスプレイを制御するためのライブラリです。
    1.  Arduino IDEで「ツール」メニューから「ライブラリを管理...」を選択します。
    2.  検索欄に「SparkFun Qwiic Alphanumeric Display」と入力します。
    3.  「SparkFun Qwiic Alphanumeric Display by SparkFun Electronics」を選択し、「インストール」をクリックします。
        <img src="https://user-images.githubusercontent.com/88629497/191186296-168a832f-1943-443c-867f-051f18141b98.png" width="600">
        *図: Arduino IDE ライブラリマネージャでのSparkFun Qwiic Alphanumeric Displayライブラリ検索とインストール*
*   **ESP32-TWAI-CANライブラリ:** ESP32のTWAI (CAN) 機能を使用するためのライブラリです。Tutorial3以降で使用します。
    *   このライブラリはArduinoのライブラリマネージャからは直接インストールできません。
    *   ESP32ボードサポート (M5Stackのボードサポートに含まれる) に標準で`driver/twai.h`として含まれています。
    *   もし別途インストールが必要な場合は、[ESP32-TWAI-CAN GitHubリポジトリ](https://github.com/collin80/ESP32-TWAI-CAN) からZIPファイルをダウンロードし、「スケッチ」メニュー > 「ライブラリをインクルード」 > 「.ZIPライブラリをインクルード...」から追加してください。ただし、通常はM5Stackボードサポートに含まれるバージョンで動作します。

#### 4.2.4. COMポートの設定
M5Stamp C3U MateをPCに接続し、Arduino IDEが正しく認識しているか確認します。
1.  M5Stamp C3U MateをUSBケーブルでPCに接続します。
2.  Arduino IDEで「ツール」メニューから「ボード」を選択し、「M5Stack Arduino」の中から「M5Stamp C3」を選択します。(C3U MateもC3として認識されます)
    <img src="https://user-images.githubusercontent.com/88629497/191186589-275a5221-35d8-410d-94c8-504ee32e242a.png" width="600">
    *図: Arduino IDE ボード選択メニューでのM5Stamp C3選択*
3.  次に「ツール」メニューから「シリアルポート」を選択し、M5Stamp C3U Mateが接続されているCOMポートを選択します。
    *   Windowsの場合: `COM3` や `COM4` などと表示されます。デバイスマネージャーで確認できます。
    *   macOSの場合: `/dev/cu.usbserial-XXXXXXXX` のように表示されます。
    *   Linuxの場合: `/dev/ttyUSBX` や `/dev/ttyACMX` のように表示されます。
    <img src="https://user-images.githubusercontent.com/88629497/191186735-1e3e361a-6682-4f6a-918f-eba57f38a233.png" width="600">
    *図: Arduino IDE シリアルポート選択メニュー*
    *   **注意:** M5Stamp C3U Mateは、書き込みモードに入るために特定の操作が必要な場合があります。多くの場合、USB接続時にリセットボタンを押すか、特定のピンをGNDに接続しながら起動する必要があります。詳細はM5Stackの公式ドキュメントを参照してください。一度書き込みモードで認識されれば、次回以降は自動的に認識されることが多いです。

### 4.3. Arduinoの基本
#### 4.3.1. スケッチとは
Arduinoでは、作成するプログラムを「スケッチ」と呼びます。スケッチは`.ino`という拡張子のファイルに記述します。
ArduinoのスケッチはC/C++言語をベースにしており、Arduino独自の関数や構造が追加されています。
基本的なスケッチは、以下の2つの主要な関数で構成されます。
*   `setup()`: スケッチが起動したときに一度だけ実行される関数です。変数の初期化やライブラリの開始、ピンモードの設定などを行います。
*   `loop()`: `setup()`関数が完了した後、繰り返し実行される関数です。主要な処理はこの関数内に記述します。

#### 4.3.2. シリアルモニタ
シリアルモニタは、ArduinoボードとPC間でテキストデータを送受信するためのウィンドウです。
プログラムのデバッグ情報を表示したり、センサーの値をリアルタイムで確認したりするのに便利です。
Arduino IDEの右上にある虫眼鏡アイコンをクリックするか、「ツール」メニューから「シリアルモニタ」を選択すると開きます。
<img src="https://user-images.githubusercontent.com/88629497/191186938-001626a8-821f-4c56-a294-0e79c642540e.png" width="400">
*図: Arduino IDE シリアルモニタアイコン*
シリアルモニタを使用する際は、スケッチ内で設定した通信速度（ボーレート、例: `Serial.begin(115200);`）と、シリアルモニタウィンドウ右下のボーレート設定が一致していることを確認してください。

## 5. ハードウェアの概要
### 5.1. M5Stamp C3U Mateの概要
M5Stamp C3U Mateは、Espressif Systems社のESP32-C3FN4マイクロコントローラ (MCU) を搭載した小型のIoT開発ボードです。MCUとは、Microcontroller Unitの略で、CPU、メモリ、入出力ポートなどが一つのチップに集積された小型のコンピュータのようなものです。
**主な仕様:**
*   **MCU:** ESP32-C3FN4 (RISC-V 32ビットシングルコアプロセッサ、最大クロック周波数 160MHz)
*   **無線通信:** Wi-Fi (IEEE 802.11b/g/n), Bluetooth 5 (LE)
*   **ポート:** USB Type-C (プログラム書き込み、シリアル通信、電源供給)
*   **内蔵LED:** プログラマブルRGB LED (Neopixel、GPIO2に接続)
*   **GPIO:** 複数の汎用入出力ピン (General Purpose Input/Output) を備えており、デジタル入出力、ADC (アナログ入力)、I2C、SPI、UARTなどの機能を利用できます。
M5Stamp C3U Mateの正確なピン配置や詳細な技術情報は、M5Stackの公式サイトで確認してください: [M5Stamp C3U User Guide](https://docs.m5stack.com/en/core/stamp_c3u)

### 5.2. VESCヘルパーボード
VESCヘルパーボードは、M5Stamp C3U MateとVESC (Vedder Electronic Speed Controller) やその他の周辺機器との接続を容易にするために設計された拡張ボードです。
**主な機能:**
*   **アナログ入力:** スロットルセンサーなどのアナログ信号を読み取るための専用ピン (ADC_IN1: GPIO3, ADC_IN2: GPIO4)。分圧回路により、5Vまでのアナログ信号をESP32-C3のADC入力範囲に収めます。
*   **CANトランシーバー:** VESCとのCAN通信を可能にするためのSN65HVD230DR CANトランシーバーを搭載。
*   **I2Cポート:** Qwiicコネクタ (Grove互換) を2つ搭載し、I2Cデバイス（例: SparkFun Qwiic Alphanumeric Display）を容易に接続可能。
*   **電源:** 5Vおよび3.3Vの電源ピン。
*   **その他:** VESCのTx/Rxピン (UART通信用、ソフトウェアシリアル等で使用可能)、汎用GPIOピン。

**ピン配置図 (主要機能):**
```text
M5Stamp C3U Mate
+-----------------------+
|                       |
|  ADC_IN1 (GPIO3) o----|--- スロットル入力1 (5Vトレラント)
|  ADC_IN2 (GPIO4) o----|--- スロットル入力2 (5Vトレラント)
|                       |
|  CAN_TX  (GPIO1) o----|--- CAN_H (VESC接続)
|  CAN_RX  (GPIO0) o----|--- CAN_L (VESC接続)
|                       |
|  SDA     (GPIO8) o----|--- Qwiic/Grove (SDA)
|  SCL     (GPIO9) o----|--- Qwiic/Grove (SCL)
|                       |
+-----------------------+
```
*(上記は主要なピンの抜粋です。詳細な回路図は後述のリンクを参照してください)*

**回路図と基板情報:**
このボードの設計ファイル (KiCad) やガーバーファイルは以下のURLで公開されています。
<!-- TODO: OSHWLABの正しいプロジェクトリンクに置き換えてください -->
*   [OSHWLAB (JLCPCB): M5Stamp C3 VESC Helper Board](https://oshwlab.com/your_username/your_project_name)
    *(注意: 上記のURLはプレースホルダーです。実際のプロジェクトのURLに置き換える必要があります)*

**基板写真:**
<img src="https://user-images.githubusercontent.com/88629497/191187303-969f314d-9939-4892-b190-9f1e7c528e24.png" width="400">
*図: VESCヘルパーボードの外観写真*

## 6. チュートリアルの進め方
本チュートリアルシリーズは、Tutorial 1からTutorial 4へと順番に進めることを推奨します。各チュートリアルは前のチュートリアルの内容を基礎としているため、段階的に学習を進めることで、理解を深めることができます。
各チュートリアルには、学習内容の理解度を確認し、応用力を養うための「課題」が用意されています。これらの課題に取り組むことで、単にコードを書き写すだけでなく、動作原理や改善点を考察する力が身につきます。
基本的な学習の流れは以下の通りです。
1.  各チュートリアルの説明を読む。
2.  提供されているサンプルスケッチをArduino IDEで開く。
3.  スケッチの内容を理解し、必要に応じて変更・追加する。
4.  M5Stamp C3U Mateにスケッチをコンパイルして書き込む。
5.  シリアルモニタや実際に接続したハードウェア（LED、ディスプレイ、モーターなど）の動作を確認する。
6.  課題に取り組む。

疑問点や不明な点が生じた場合は、各チュートリアルの解説、提供されている参考リンク、ArduinoやESP32、FreeRTOSの公式ドキュメントなどを参照してください。

## 7. （補足）Arduino C++プログラミングについて
Arduinoで使用するプログラミング言語は、C++がベースです。C++は非常に強力で多機能な言語ですが、Arduinoでは初心者にも扱いやすいよう、いくつかの便利な関数やライブラリが事前に用意されています。
基本的なArduinoスケッチは、`setup()`関数と`loop()`関数から構成されます。
*   `setup()`: マイコンが起動したときに一度だけ実行されます。主に初期設定（ピンモードの設定、シリアル通信の開始、ライブラリの初期化など）を行います。
*   `loop()`: `setup()`の実行後、繰り返し無限に実行されます。プログラムの本体となる処理をここに記述します。

プログラミングを進める上で、以下のC++の基本的な要素の理解が重要です。
*   **変数とデータ型:** `int` (整数)、`float` (浮動小数点数)、`bool` (真偽値)、`char` (文字)、`String` (文字列) など、様々な種類のデータを扱うためのものです。
*   **関数:** 特定の処理をまとめて名前を付けたものです。`digitalWrite()` や `Serial.println()` など、Arduinoが提供する多くの便利な関数があります。自分で関数を作成することも可能です。
*   **制御構造:** `if-else` (条件分岐)、`for`ループ、`while`ループ (繰り返し処理) など、プログラムの流れを制御するための構文です。
*   **クラスとオブジェクト:** ライブラリを使用する際、例えば `SparkFun_Alphanumeric_Display display;` のように記述することがあります。これは `SparkFun_Alphanumeric_Display` という「クラス（設計図）」から `display` という「オブジェクト（実体）」を作成する、というC++の重要な概念に基づいています。

より詳しいArduinoのプログラミングについては、以下の公式リファレンスが参考になります。
*   [Arduino 言語リファレンス (日本語)](https://www.arduino.cc/reference/ja/)
*   [Arduino Language Reference (English)](https://www.arduino.cc/reference/en/)

本チュートリアルでは、これらの基礎を踏まえつつ、実際のハードウェアを動かしながら実践的なプログラミングスキルを習得することを目指します。
```
