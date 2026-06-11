#pragma once
#include <Arduino.h>

// =====================================================================
// Pins.h — 全部 GPIO 的唯一定义处
//
// 规则：业务代码中禁止直接写 GPIO 数字，必须引用本文件的常量。
// 修改引脚时请同步更新 docs/pinout.md 和 README.md 的接线表。
//
// ESP32 注意事项：
//  - GPIO 34 / 35 / 36 / 39 为输入专用脚，不能作输出，也没有内部上下拉。
//  - GPIO 0 / 2 / 5 / 12 / 15 是启动敏感（strapping）引脚。
//    本表仅在 GPIO 5 上挂 RC522 的 SPI CS，RC522 不会在上电时拉低它，
//    属于常见安全用法；其余 strapping 脚均未使用。
// =====================================================================

// ---- I2C（OLED SSD1306）----
constexpr uint8_t PIN_I2C_SDA = 21;
constexpr uint8_t PIN_I2C_SCL = 22;

// ---- SPI（RC522 RFID，使用 VSPI 默认引脚）----
constexpr uint8_t PIN_RFID_SCK  = 18;
constexpr uint8_t PIN_RFID_MISO = 19;
constexpr uint8_t PIN_RFID_MOSI = 23;
constexpr uint8_t PIN_RFID_SS   = 5;   // SPI CS（RC522 标注 SDA/SS）
constexpr uint8_t PIN_RFID_RST  = 4;

// ---- 闸机舵机（SG90，外接 5V 供电，与 ESP32 共地）----
constexpr uint8_t PIN_GATE_SERVO = 13;

// ---- 蜂鸣器（数字输出，默认按高电平触发的有源蜂鸣器模块）----
constexpr uint8_t PIN_BUZZER = 14;

// ---- 入口超声波 HC-SR04 ----
constexpr uint8_t PIN_ULTRASONIC_TRIG = 25;
constexpr uint8_t PIN_ULTRASONIC_ECHO = 26;  // Echo 为 5V 输出，必须分压到 3.3V 再接入

// ---- 车位红外传感器（数字输入；34/35 为输入专用脚）----
constexpr uint8_t PIN_SLOT_SENSOR[4] = {32, 33, 34, 35};

// ---- Phase 2 预留（本期不接线，仅占位防止引脚冲突）----
constexpr uint8_t PIN_FLAME_SENSOR = 27;  // 火焰传感器，数字输入
constexpr uint8_t PIN_SMOKE_SENSOR = 36;  // MQ 烟雾模块 AO，模拟输入（ADC1，输入专用）
constexpr uint8_t PIN_FAN_CONTROL  = 16;  // 风扇 MOSFET/继电器控制
