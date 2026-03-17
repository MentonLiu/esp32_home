# 变更说明 0.0.2

日期：2026-03-17

## 0.0.2 更新内容（新增）
1. 删除原有红外兼容控制逻辑，仅保留 ESP8266 串口桥接方案。
2. 删除旧命令兼容路径：移除 action=send_nec 分支。
3. 删除旧接口兼容实现：移除 IRController::sendNEC。
4. 删除旧直连红外兼容引脚：移除 IR_RX / IR_TX 保留字段。
5. 保留并继续使用 ESP8266 桥接能力：
- send（通用协议命令）
- send_json（完整 JSON 透传）
- 扩展 action + args（由 ESP8266 定义语义）

## 版本说明
- 当前版本：0.0.2
- 上一版本记录：0.0.1

---

# 变更说明 dev0.0.1

日期：2026-03-17-10:45

## 本次更新目标
将红外控制逻辑从 ESP32 本地红外收发，改为 ESP32 通过串口向外接 ESP8266 红外模块发送命令，并保留可扩展命令接口。

## 代码变更摘要

1. 红外控制重构为桥接模式
- 修改文件：include/Controllerr.h, src/Controllerr.cpp
- 主要变化：
  - IRController 改为 UART 桥接发送器，不再依赖本地红外库收发。
  - 新增可扩展接口：
    - sendProtocolCommand(protocol, address, command, repeats)
    - sendActionCommand(action, args)
    - sendJsonCommand(jsonCommand)
  - 保留兼容接口 sendNEC(address, command, repeats)。
  - 增加 receive() 读取 ESP8266 回传消息能力。

2. HomeService 接入 ESP8266 串口桥接
- 修改文件：src/HomeService.cpp
- 主要变化：
  - 新增 Serial2 桥接初始化，使用 GPIO16/GPIO17，波特率 115200。
  - 红外命令解析支持：
    - send_nec（兼容旧接口）
    - send（通用协议）
    - send_json（完整JSON透传）
    - 其他 action（扩展动作 + args）
  - 将 ESP8266 上行消息转发为状态消息，便于调试。

3. 引脚定义更新
- 修改文件：include/pins.h
- 新增：
  - IR_BRIDGE_UART_RX = 16
  - IR_BRIDGE_UART_TX = 17
  - IR_BRIDGE_UART_BAUD = 115200
- 保留原 IR_RX/IR_TX 作为兼容字段（默认不启用）。

4. 依赖调整
- 修改文件：platformio.ini
- 移除 IRremote 依赖（ESP32 不再直接驱动红外收发）。

5. 文档更新
- 修改文件：README.md
- 主要变化：
  - 更新为“ESP8266 红外桥接”架构说明。
  - 新增通用协议、扩展动作、完整 JSON 透传示例。
  - 更新排障说明为 UART 桥接相关检查项。

## 兼容性说明
- 原有红外命令 action=send_nec 继续可用。
- 新增通用与扩展命令不会影响风扇、窗帘、传感器等既有功能。

## 使用建议
- 确保 ESP8266 固件按“单行 JSON 命令”协议解析串口输入。
- 建议 ESP8266 每次执行后返回状态行，便于 ESP32 上报运行状态。
