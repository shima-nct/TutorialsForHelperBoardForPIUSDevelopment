# Tutorial4_5 Wheel CAN Bridge To VESC

Wheel CAN Bridge の `0x2010 DRIVER_INPUT` を受信し、VESC の
`CAN_PACKET_SET_DUTY` に変換して送信するサンプルです。

## 受信

```text
CAN ID: 0x2010 extended
DLC: 8
byte 0..1 throttle uint16 BE scale 10000
byte 2..3 brake    uint16 BE scale 10000
byte 4..5 steering int16  BE scale 10000
byte 6    flags
byte 7    counter
```

flags:

```text
bit0 enable
bit1 estop
bit2 reverse
```

## 送信

既存の Tutorial4 と同じ VESC duty 指令を送ります。

```text
CAN ID: (CAN_PACKET_SET_DUTY << 8) | VESC_ID
CAN ID: (0x00 << 8) | 0x07 = 0x00000007
frame: extended
DLC: 4
payload: int32 BE, duty * 100000
period: 20 ms
```

## 安全条件

次の場合は duty `0.0` を送ります。

- Wheel CAN Bridge の受信が 200 ms 以上止まった
- `enable == 0`
- `estop == 1`
- `brake > 0.05`

## 変換

```text
forward: duty = throttle * 0.30
reverse: duty = -throttle * 0.20
```

実機では、モーターを外すかVESC側を安全な状態にしてから確認してください。
