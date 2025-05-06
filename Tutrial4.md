# VESCによるモーター制御

## VESCとは

## VESC ToolによるVESC制御

## VESC ExpressとVESC ToolによるVESC制御

## 課題4

### 4.1

VESC Toolのマニュアルを参照し、以下の操作、動作を確認しなさい。

1. VESCに電源とDCモーター、電源スイッチをつなぎ、さらにWindowsをUSBで接続しなさい。
1. DCモーターの設定を行いなさい。
1. キーボード制御機能を有効化し、キーボードからモータが操作できることを確認しなさい。

### 4.2

VESC Express、VESC Toolのマニュアルを参照し、以下の操作、動作を確認しなさい。

1. 課題4.1の構成を組み立て、動作することを確認しなさい。
1. VESCとWindowsのUSB接続を外し、VESC Expressにつなぎなさい。
1. VESCとVESC ExpressをCANで接続し、VESC ToolでCANが認識されていることを確認しなさい。
1. キーボード制御機能を有効化し、キーボードからモータが操作できることを確認しなさい。

### 4.3

VESC 6 CAN Formatを参照し、以下のプログラムを作り操作、動作を確認しなさい。

1. 以下の処理は10ms毎にFreeRTOSのタスクとして実行しなさい。
1. スロットルの値を1ms毎に読み取り10ms間隔で平均化しなさい。
1. スロットル開度の値をVESC 6 CAN FormatのPWMの値に変換しなさい。
1. PWMの値をVESC 6 Format CAN_PACKET_SET_DUTYの値に変換しなさい。
1. PWMの値をCANで発出しなさい。

https://vesc-project.com/sites/default/files/imce/u15301/VESC6_CAN_CommandsTelemetry.pdf
https://github.com/vedderb/bldc/blob/master/documentation/comm_can.md

### 4.4

CANから受信したPWMの値を7セグLEDで表示するプログラムを作り、動作を確認しなさい。タイムループはFreeRTOSで制御すること。