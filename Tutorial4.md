# モーター制御

モーターは印加電圧の大きさが回転速度を規定し、電流の大きさがトルクを規定します。

## PWM

モーターに印可する電圧を連続的に変化するのは損失が大きくなるため、一般的には一定の周期でON/OFFする波形のデューティー比で調整することで行われています。

## ブラシレスモーター(BLDC)

# VESCによるモーター制御

## VESCとは

VESCとはBenjamin Vedderにより開発されたOpen Source Hardware、Opne S  Source SoftwareなESC(Electric Speed Controller)。ここで言うESCは車両ののスピードコントローラーのことですが、主にEVのスピードコントローラの意味で用いられる用語です。

https://vesc-project.com/
https://www.youtube.com/@BenjaminsRobotics/featured

## VESC ToolによるVESC制御

https://vesc-project.com/vesc_tool

## VESC ExpressとVESC ToolによるVESC制御

https://trampaboards.com/vesc-express--p-34857.html
https://github.com/vedderb/vesc_express
https://www.youtube.com/watch?v=wPzdzcfRJ38

## モーター制御の主なパラメーター

### PWM

モーターに印可する電圧レベルはPWMにより実効的に調整されます。

### Current Limit

モータに流す電流値の上限を設定します。
この電流値に達すると印加電圧をOFFにし、電流値が下がると再びONにします。
波形変化として見た場合、電流値によりPWMをさらに変調するような変化になります。

### Absolute Current Limit

出力を緊急停止させる電流値です。


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