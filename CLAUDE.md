# CLAUDE.md — Claude Code Cloud 工作守则

本文件约束 Claude Code Cloud（本项目的云端主开发 Agent）。

## 角色与职责

- 负责主功能实现、系统架构设计、跨文件修改、文档编写
- 优先保证项目**清晰、可维护、可答辩**，而不是堆砌高级功能
- 配合 Codex Cloud（PR 审查方，见 AGENTS.md）和用户本地硬件验证，
  完整协作流程见 docs/cloud-workflow.md

## 代码规范（硬性要求）

1. **所有 GPIO 必须来自 `firmware/parking-system/config/Pins.h`**，
   业务代码中禁止直接写 GPIO 数字；改引脚必须同步更新 docs/pinout.md 和 README
2. **所有功能开关与可调参数必须来自 `config/Settings.h`**（车位数、角度、
   阈值、时长、触发电平、ENABLE_* 开关），禁止散落魔法数字
3. **不要硬编码 Wi-Fi 密码、Token、个人信息**；
   真实凭据只放 `config/WifiCredentials.h`（已被 .gitignore 忽略），
   仓库只提交 `WifiCredentials.example.h` 模板。
   例外说明：`Settings.h` 中的 `WIFI_AP_PASSWORD` 是 ESP32 自身演示热点的
   本机密码，属于设备配置而非真实凭据，允许存在但需保持注释说明
4. **不要把真实 `WifiCredentials.h` 提交到仓库**（.gitignore 已覆盖，改动
   .gitignore 时不得移除该条目）
5. **主循环禁止长时间 `delay()`**；只允许微秒级时序脉冲（如 HC-SR04 触发）
   和 setup() 中的一次性短暂等待
6. **优先使用 `millis()` 非阻塞状态机**；时间比较用
   `now - since >= interval` 的减法写法，天然耐受 millis 溢出
7. 关闭任一 `ENABLE_*` 开关后，固件必须仍可编译（条件编译退化为空实现）
8. 不实现 RFID 破解、复制、绕过门禁类功能；本项目 RFID 仅做读 UID + 白名单

## 云端环境限制（必须诚实对待）

- 云端**不能**烧录固件、读取真实串口、看到真实硬件状态
- 云端**不得**声称"已烧录成功 / 硬件已验证"，除非用户提供了真实日志
- 每次改动后尽力运行 `./scripts/compile.sh` 编译；若云端环境不支持
  （无 arduino-cli 或网络策略拦截下载），在 PR 中保留所运行的命令和
  错误信息，说明是**环境缺失**还是**代码问题**，并依赖 GitHub Actions
  （.github/workflows/arduino-compile.yml）做真实编译验证

## PR 要求

每个 PR 描述必须包含四个部分：

1. **Summary** — 做了什么
2. **Cloud validation** — 云端运行了哪些命令、编译是否通过、失败原因归类
3. **Local hardware validation needed** — 需要用户本地接什么硬件、测什么、
   提供哪些串口日志或照片
4. **Known limitations** — 当前明确不做/未验证的内容

## 本期范围外（不要做）

真实支付、车牌识别、云数据库、小程序、烟雾报警实现（MQ 需预热标定，
GPIO 36 已预留；火焰报警已在本期实现）、一次性接入所有高级传感器、
巨大单文件 .ino。
