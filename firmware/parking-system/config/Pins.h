#pragma once
#include <Arduino.h>

// =====================================================================
// Pins.h — 全部 GPIO 的唯一定义处
//
// 规则：业务代码中禁止直接写 GPIO 数字，必须引用本文件的常量。
// 修改引脚时请同步更新 docs/pinout.md 和 README.md 的接线表。
//
// 本期方案（简化版）：去掉 RFID 刷卡、SG90 闸机、HC-SR04 入口超声波，
// 仅靠红外车位传感器做车辆识别 + 车位管理 + 按时长计费。
//
// ESP32 注意事项：
//  - GPIO 34 / 35 / 36 / 39 为输入专用脚，不能作输出，也没有内部上下拉。
//  - GPIO 0 / 2 / 5 / 12 / 15 是启动敏感（strapping）引脚，扩展时优先避开。
// =====================================================================

// ---- I2C（OLED SSD1306）----
constexpr uint8_t PIN_I2C_SDA = 21;
constexpr uint8_t PIN_I2C_SCL = 22;

// ---- 蜂鸣器（数字输出，默认按高电平触发的有源蜂鸣器模块）----
constexpr uint8_t PIN_BUZZER = 14;

// ---- 车位红外传感器（数字输入；34/35 为输入专用脚）----
// 检测到车辆即视为该车位被占用，是本期"车辆识别"的唯一传感来源。
constexpr uint8_t PIN_SLOT_SENSOR[4] = {32, 33, 34, 35};

// ---- Phase 2 预留（本期不接线，仅占位防止引脚冲突）----
constexpr uint8_t PIN_FLAME_SENSOR = 27;  // 火焰传感器，数字输入
constexpr uint8_t PIN_SMOKE_SENSOR = 36;  // MQ 烟雾模块 AO，模拟输入（ADC1，输入专用）
constexpr uint8_t PIN_FAN_CONTROL  = 16;  // 风扇 MOSFET/继电器控制
