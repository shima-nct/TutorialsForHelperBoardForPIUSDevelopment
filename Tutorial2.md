# 表示装置を使う
本チュートリアルでは、様々な表示装置の特徴と使用方法、およびそれらを制御するための通信方式について学びます。また、実際の機器との通信を通じて、デジタル通信の基礎を理解します。

## 表示装置

### 7セグメントLED（7セグLED）
7セグメントLEDは、7つのLEDセグメントを組み合わせて数字や一部のアルファベットを表示できる表示素子です。構造がシンプルで視認性が高く、コストも低いため、シンプルな数値表示に広く使用されています。

![alt text](https://www.rohm.co.jp/documents/11401/7006618/led_what7_img_01.gif/0d989e59-0d8d-4d4f-6881-6dda5acc4433?t=1559699425710)
-- <cite>[7セグメントLEDとは？](https://www.rohm.co.jp/electronics-basics/led/led_what7)


### キャラクタLCD
キャラクタLCDは、文字やシンボルを表示するための液晶ディスプレイです。あらかじめ定義された文字セットを使用して、テキストベースの情報を表示することができます。一般的に16×2や20×4などの固定サイズの文字マトリクスで構成されています。

![alt text](https://gihyo.jp/assets/images/dev/serial/01/micom-linux/0014/001.jpg)
-- <cite>[組込みLinuxでLCDを制御してみよう](https://gihyo.jp/dev/serial/01/micom-linux/0014)

### グラフィックLCD
グラフィックLCDは、ピクセル単位で表示を制御できる液晶ディスプレイです。文字だけでなく、画像やグラフなどの自由な図形を表示することができます。解像度やカラー表示能力によって様々な種類があります。

![alt text](https://cdn-learn.adafruit.com/assets/assets/000/119/123/original/adafruit_products_2088-11.gif?1677772363)
-- <cite>[Adafruit 1.44" Color TFT with Micro SD Socke](https://learn.adafruit.com/adafruit-1-44-color-tft-with-micro-sd-socket)

## 基板上のIC間で用いられるシリアル通信

### UART
UART（Universal Asynchronous Receiver/Transmitter）は、非同期シリアル通信の標準的なプロトコルです。2本の信号線（TX/RX）を使用して、双方向のデータ通信を行います。設定が簡単で柔軟性が高いため、デバッグやシンプルな機器間通信によく使用されます。

![](https://www.rohm.co.jp/documents/11401/10929576/img_03.png/1c1ab6a1-11d4-2046-6ccb-da233adeac1f?t=1661926925861)
-- <cite>[UARTとは](https://www.rohm.co.jp/electronics-basics/micon/mi_what9)

### I2C
I2C（Inter-Integrated Circuit）は、2本の信号線（SCL/SDA）を使用するシリアルバス規格です。マスター・スレーブ方式で、1本のバスに複数のデバイスを接続できます。各デバイスにアドレスが割り当てられ、アドレス指定によって特定のデバイスと通信を行います。

![](https://www.rohm.co.jp/documents/11401/10929442/img_01.png/308b4cc6-1be9-e158-ac26-ecbbb36a57bc?t=1661924710819)
--<cite>[I2Cとは](https://www.rohm.co.jp/electronics-basics/micon/mi_what7)

### SPI
SPI（Serial Peripheral Interface）は、4本の信号線（MOSI/MISO/SCK/CS）を使用する高速なシリアル通信規格です。全二重通信が可能で、I2Cよりも高速なデータ転送が可能です。ただし、デバイスごとにCS（Chip Select）線が必要となります。

![](https://www.rohm.co.jp/documents/11401/10929514/img_01.png/0e5eef44-29d5-8d43-d3a7-39e368fdd4a0?t=1661926229380)
--<cite>[SPIとは](https://www.rohm.co.jp/electronics-basics/micon/mi_what8)

### Qwiic/STEMMA QT
Qwiic/STEMMA QTは、I2C通信を基にした規格化された接続システムです。極性の誤接続を防ぐコネクタと標準化された電源電圧を採用し、センサーやディスプレイなどの接続を簡単かつ安全に行うことができます。

[Qwiic Connect System](https://www.sparkfun.com/qwiic)
[STEMMA QT](https://learn.adafruit.com/introducing-adafruit-stemma-qt/what-is-stemma)


### EYESPI

```
Adafruitの最新のディスプレイ・ブレイクアウトには、新しい機能が搭載されています。それは、フリップトップ・コネクタ付きの18ピン "EYE SPI "標準FPCコネクタです。これは「ディスプレイ用STEMMA QT」のようなもので、SPIピンを多用するディスプレイ配線を素早く接続して拡張するためのものです。この場合、多くのSPIピンが必要で、長い距離を使用したいので、答えは18ピンの0.5mmピッチFPCコネクタです。
```
-- <cite>[Adafruit Eyespi Breakout Board](https://learn.adafruit.com/adafruit-eyespi-breakout-board/overview)

## 課題

### 課題1

#### 1.1
USBデバイス・クラスおよび、USB communications device classについて説明しなさい。
USBデバイスクラスは、USBデバイスの機能と通信方法を標準化した仕様です。特にCDC（Communications Device Class）は、シリアル通信機器をUSB経由で接続するための重要な規格です。

#### 1.2
ArduinoなどのマイコンボードをPCにつないだ際に、マイコンボードとの通信に用いられる通信ポートの探し方を説明しなさい。
PCとマイコンボードの通信ポートの特定は、デバイスマネージャーやシリアルポートツールを使用して行います。正しいポートを見つけることは、デバイスとの通信を確立する上で重要な最初のステップです。

#### 1.3
I2C、SPIのバスラインはオープンドレインで駆動されています。このオープンドレインについて説明しなさい。説明にはプルアップ、プルダウンについての説明も含まれていること。
オープンドレイン回路は、デジタル信号線の駆動方式の一つです。プルアップ抵抗により信号線をHigh状態に保持し、必要に応じてLow状態にプルダウンする仕組みについて理解することは、I2CやSPI通信の基礎となります。

#### 1.4
I2C Standard-mode(100kbit/s)の場合におけるプルアップ抵抗の最小値、最大値の計算方法を説明しなさい。また、プルアップ抵抗の値の大小はどのような特性とのトレードオフなのかを説明しなさい。
I2C通信におけるプルアップ抵抗の設計は、信号の立ち上がり時間と消費電力のバランスに大きく影響します。適切な抵抗値の選定は、安定した通信を実現する上で重要なポイントとなります。

## 課題2

### 2.1
SparkFun Qwiic Alphanumeric Displayは、I2C通信を使用した4桁の英数字ディスプレイモジュールです。SparkFun Qwiic Alphanumeric Displayを用いて数値を表示するプログラムを動かしなさい。

このモジュールを使用して、実際のI2C通信とディスプレイ制御を体験します。

### 2.2
SparkFun Qwiic Alphanumeric DisplayとのI2C接続のバスラインをブレッドボードに引き出し、オシロスコープで観察し、何らかの表示を指令しているフレームをキャプチャしなさい。また、キャプチャしたデータとSparkFun Qwiic Alphanumeric Displayで用いられているVK16K33のデータシートからキャプチャされたフレームの指令を読み解きなさい。

### 2.3
FreeRTOSを用いて0.1s毎の経過時間をSparkFun Qwiic Alphanumeric Displayに表示するプログラムを動かしなさい。

FreeRTOSのタスク生成と周期実行、ディスプレイ制御の基本を理解します。また、実際のディスプレイ制御と通信を体験します。
