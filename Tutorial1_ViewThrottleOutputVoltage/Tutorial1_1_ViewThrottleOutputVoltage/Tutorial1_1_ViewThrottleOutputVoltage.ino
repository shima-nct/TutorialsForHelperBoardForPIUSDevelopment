/*
  M5Stamp C3U Mate: ホールセンサー出力の ADC 測定スケッチ（GPIO_NUM_3 使用版）
  VCC → 5V, GND → GND, OUT → G3（GPIO3）
*/

#include <Arduino.h>
#include <driver/gpio.h>
#include <esp32-hal-adc.h>   // ADC attenuation 用

const int hallPin = GPIO_NUM_3;  // GPIO3 を指定

void setup() {
  Serial.begin(115200);
  // 測定範囲を約0〜2.5Vに拡張（ADC_11db で最大約2.5Vまで測定可）  
  analogSetPinAttenuation(hallPin, ADC_11db);  // ESP32-C3 でも利用可能 :contentReference[oaicite:1]{index=1}
  // ※ analogSetWidth() は ESP32 専用なので呼ばない
}

void loop() {
  int raw = analogRead(hallPin);  
  // 12bit 解像度：0–4095 → 0–2.5V
  float voltage = raw * (2.5f / 4095.0f);

  Serial.printf("RAW: %4d    Vout: %.3f V\n", raw, voltage);
  delay(500);
}
