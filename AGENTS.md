# AGENTS.md — Codex Cloud 审查守则

本文件写给 Codex Cloud（本项目的 PR 审查 Agent）。
项目背景与开发方协作规范见 CLAUDE.md，完整流程见 docs/cloud-workflow.md。

## 角色与职责

Codex Cloud 负责：**PR 审查、编译验证、可维护性检查、测试建议**。
不负责主功能开发；不要大规模推倒 Claude Code 已有架构，除非发现严重问题
（内存破坏、引脚冲突、死循环、安全隐患等），此时以 blocking issue 提出。

## 每个 PR 的检查清单

### 编译与环境
- [ ] 运行 `./scripts/compile.sh`（必要时 `--install-deps`）确认可编译；
      若退出码 2（环境缺失）且容器网络拦截 Arduino 官方源但放行 GitHub，
      先运行 `./scripts/setup-cloud-env.sh` 再编译（Claude Code Cloud 已实测可行）；
      仍不行则至少确认 GitHub Actions `Arduino Compile` 工作流是绿的，
      并检查 scripts/、CI、依赖库说明三者是否完整一致
- [ ] 新增第三方库时，`.github/workflows/arduino-compile.yml`、
      `scripts/compile.sh`、README 的库清单是否同步更新

### 引脚与硬件安全
- [ ] 所有 GPIO 是否都来自 `config/Pins.h`，无散落的引脚数字
- [ ] 引脚是否冲突（同一 GPIO 被两个功能使用）
- [ ] 是否误用 ESP32 启动敏感引脚（GPIO 0/2/5/12/15；当前仅 GPIO5 作
      RC522 CS，属已知安全用法）；GPIO 34/35/36/39 输入专用，禁止当输出
- [ ] README、docs/pinout.md、config/Pins.h 三处引脚表是否一致

### 代码质量
- [ ] 主循环是否存在阻塞式长 `delay()`（微秒级脉冲与 setup 内一次性等待除外）
- [ ] 状态机时间比较是否用 `now - since >= X` 减法写法（耐受 millis 溢出）
- [ ] 是否存在内存风险：无界 String 拼接、缓冲区溢出、循环里反复 new、
      snprintf/strlcpy 的长度参数是否正确
- [ ] 关闭各 `ENABLE_*` 开关后是否仍可编译（条件编译完整性）

### 安全与信息泄露
- [ ] 是否泄露 Wi-Fi 密码、Token、个人信息；`config/WifiCredentials.h`
      绝不能出现在提交中（只允许 .example 模板）
      （注：`Settings.h` 的 `WIFI_AP_PASSWORD` 是 ESP32 演示热点本机密码，
      属设备配置，不算泄露，不必报问题）
- [ ] 是否引入了 RFID 破解/复制/绕过类代码（本项目禁止）

### Web 与文档
- [ ] Web API 是否清晰：路由、方法、JSON 字段与 README / docs 描述一致
- [ ] `web/dashboard.html` 与 `WebDashboard.cpp` 内嵌 PROGMEM 页面是否同步
- [ ] PR 描述是否包含 Cloud validation / Local hardware validation needed /
      Known limitations 三个诚实声明部分

## 审查产出格式

- 必须修复才能合并的问题 → **blocking**，逐条给出文件:行号和理由
- 建议性改进 → non-blocking，简要说明
- 无法在云端验证的硬件行为 → 不要凭空猜测结论，标注"需本地硬件验证"
