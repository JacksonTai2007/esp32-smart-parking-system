# 云端 AI Agent 协作工作流

本项目采用"云端 AI 开发 + 本地硬件验证"的协作模式。三方角色：

| 角色 | 位置 | 职责 |
| --- | --- | --- |
| **Claude Code Cloud** | 云端 | 主功能开发、架构设计、跨文件实现、文档 |
| **Codex Cloud** | 云端 | PR 审查、编译验证、可维护性检查、测试建议 |
| **用户（本人）** | 本地 | 真实接线、烧录、串口日志、阈值调试、答辩演示 |
| GitHub Issues / PR | 平台 | 任务管理与协作中心 |

## 标准流程（每个功能迭代）

1. 我创建任务（Issue 或直接给 Claude Code Cloud 发任务描述）
2. Claude Code Cloud 创建分支并实现功能
3. Claude Code Cloud 提交 PR（描述必须含 Summary / Cloud validation /
   Local hardware validation needed / Known limitations 四部分）
4. Codex Cloud 审查 PR（按 AGENTS.md 检查清单）
5. Codex Cloud 运行编译、检查代码、提出 blocking issues
6. Claude Code Cloud 根据审查意见修复
7. 我本地拉取分支、配置 Wi-Fi（`config/WifiCredentials.h`）、烧录 ESP32
8. 我把串口日志、接线照片、硬件问题贴回 PR
9. Claude Code Cloud 修复硬件适配问题（阈值、电平逻辑、地址等）
10. Codex Cloud 二次审查
11. 合并 main

## 云端能做什么 / 不能做什么

**云端 Agent 可以：**
- 写固件代码、文档、接线表、测试说明
- 运行 `./scripts/compile.sh` 做编译检查（若容器内能安装 Arduino CLI）
- 维护 GitHub Actions 编译工作流（云端环境缺失时由 CI 兜底编译验证）

**云端 Agent 不能（也不得假装能）：**
- 连接用户的 ESP32 / 烧录固件（`scripts/upload.sh` 仅限本地）
- 读取真实串口日志
- 验证真实传感器状态、阈值、电平逻辑
- 声称"硬件验证通过"，除非用户提供了真实日志

## 编译验证的三道防线

1. **云端容器**：`./scripts/compile.sh`；若容器网络拦截 Arduino 官方源
   （`downloads.arduino.cc` / `espressif.github.io`）但放行 GitHub，
   先运行 `./scripts/setup-cloud-env.sh` 从 GitHub Releases 搭建工具链
2. **GitHub Actions**：`.github/workflows/arduino-compile.yml`，PR 自动触发，
   是云端环境缺失时的权威编译结论
3. **用户本地**：`./scripts/compile.sh` + `./scripts/upload.sh` 真机验证

## 硬件反馈格式建议（贴回 PR 用）

```
### 硬件实测反馈
- 模块: 红外车位传感器 / OLED / 蜂鸣器 / Web（计费）
- 现象: （描述）
- 串口日志:
  （115200 波特率，粘贴相关片段）
- 接线照片: （如有）
- 期望行为: （描述）
```
