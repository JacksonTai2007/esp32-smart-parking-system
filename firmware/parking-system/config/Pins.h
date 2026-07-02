#pragma once
#include <Arduino.h>

// =====================================================================
// Pins.h — 全部 GPIO 的唯一定义处
//
// 规则：业务代码中禁止直接写 GPIO 数字，必须引用本文件的常量。
// 修改引脚时请同步更新 docs/pinout.md 和 README.md 的接线表。
//
// 本期方案：红外车位识别 + 按时长计费 + 触摸感应智能入场（自动分配车位引导）
// + 火灾报警联动风扇。不做 RFID 刷卡 / 闸机 / 超声波。
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

// ---- 火灾报警（火焰传感器 DO，数字输入；用内部上/下拉防未接线浮空误报）----
constexpr uint8_t PIN_FLAME_SENSOR = 27;

// ---- 风扇联动（继电器/MOSFET 控制，火灾报警时自动启动）----
constexpr uint8_t PIN_FAN_CONTROL  = 16;

// ---- 入口感应（TTP223 触摸模块 DO，数字输入；智能入场检测）----
// 车辆到达入口 = 触摸片被拍一下：自动分配空闲车位并给出引导。
// 选 GPIO 25：普通 IO，带内部上/下拉，未接模块时不会误触发。
constexpr uint8_t PIN_ENTRY_TOUCH = 25;

// ---- 降雨提示（雨滴传感器 DO，数字输入；露天车位提醒）----
constexpr uint8_t PIN_RAIN_SENSOR = 17;

// ---- 碰撞告警（震动传感器 DO，数字输入；车辆刮蹭/撞击检测）----
constexpr uint8_t PIN_VIBRATION_SENSOR = 18;

// ---- Phase 2 预留（本期不接线，仅占位防止引脚冲突）----
constexpr uint8_t PIN_SMOKE_SENSOR = 36;  // MQ 烟雾模块 AO，模拟输入（ADC1，输入专用）
