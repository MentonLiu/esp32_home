# 变更说明 0.1.0

日期：2026-03-20

## 0.1.0 更新内容
1. 按 `/doc` 下需求文档、功能流程图和项目架构图，对 `include/` 与 `src/` 进行了整套重建。
2. 删除旧 `HomeService` 方案，改为新的模块化结构：
- `CentralProcessor`
- `Sensor` / `SensorDataProcessor`
- `Controllerr` / `ControllerCommandProcessor`
- `ConnectivityManager`
- `LocalProcessingProgram`
- `AutomationEngine`
3. 重建传感器链路，统一支持 DHT11、光照、MQ2、火焰采样，并按 0.5 秒周期输出标准化状态。
4. 重建设备控制链路，统一支持：
- 风扇开关与百分比调速
- 双舵机窗帘角度控制与 0-4 预设档位
- 蜂鸣器报警
- 红外桥接协议发送、动作发送与 JSON 透传
5. 重建联网逻辑，实现：
- `home-WiFi` 路由器联网
- `esp32-server` / `lbl450981` 本地热点兜底
- 30 秒网络重检
- Cloud / Local AP 双模式切换
6. 重建本地 WebServer 页面与接口：
- `/`
- `/project-map`
- `GET /api/status`
- `POST /api/control`
7. 重写 `web/index.html`，使页面与当前固件接口完全对应，并新增红外桥接控制区。
8. 重写 `web/project-map.html`，让结构图页面与当前模块划分一致。
9. 重写 `README.md`，同步新的目录结构、接口、构建方式、接线说明，并引用 `/doc` 中的图示资源。
10. 保留并整理 MQTT 云接口约定，当前第一版继续使用占位域名，等待后续接入真实 Mosquitto 服务。
11. 编译验证通过：
- `pio run`
- `pio run -t buildfs`

## 版本说明
- 当前版本：0.1.0
- 上一版本记录：0.0.7

---

# 变更说明 0.0.7

日期：2026-03-19

## 0.0.7 更新内容（新增）
1. 将 Web 控制台与固件逻辑解耦：删除 `src/HomeServiceControl.cpp` 中内嵌 HTML，改为通过 LittleFS 加载 `web/index.html`。
2. 新增文件系统访问容错：LittleFS 挂载失败返回 `503`，页面文件缺失返回 `404`，并上报状态主题错误信息。
3. 补全 `platformio.ini` 中库解析与文件系统相关字段：
- 增加 `board_build.filesystem = littlefs`
- 增加 `data_dir = web`
- 增加 `lib_ldf_mode = chain+`
4. 构建与部署流程调整：需要在固件上传后执行一次 `pio run -t uploadfs` 同步网页资源。

## 版本说明
- 当前版本：0.0.7
- 上一版本记录：0.0.6

---

# 变更说明 0.0.6

日期：2026-03-18

## 0.0.6 更新内容（新增）
1. 项目主控平台从 ESP32 切换为 ESP32-S3（`esp32-s3-devkitc-1`）。
2. 更新 PlatformIO 构建环境与上传配置：环境名改为 `esp32s3`，使用 `default_reset/hard_reset` 上传流程。
3. 根据 ESP32-S3 调整关键引脚：
- 模拟输入改为 GPIO4/5/6（LDR/MQ2/火焰占位）
- 红外桥接 UART 改为 GPIO15/14
- RTC I2C 改为 GPIO8/9
4. 移除 LittleFS 依赖路径，Web 首页恢复为固件内置页面方案（不再需要 `uploadfs`）。
5. README 全量同步为 ESP32-S3 硬件说明、接线表与排障信息。
6. 编译验证通过：`platformio run` 成功（目标环境 `esp32s3`）。

## 版本说明
- 当前版本：0.0.6
- 上一版本记录：0.0.5

---

# 变更说明 0.0.5

日期：2026-03-18

## 0.0.5 更新内容（新增）
1. 温湿度链路改为仅支持 DHT11，删除 DHT22 相关实现与可配置类型参数。
2. 精简传感器构造接口：DHT 传感器固定初始化为 DHT11。
3. 更新引脚与说明文档，统一为 DHT11 数据脚 GPIO13。
4. 同步更新 web 控制台温湿度文案，标注 temperature/humidity 来自 DHT11。
5. 重新编译验证通过：platformio run 成功。

## 版本说明
- 当前版本：0.0.5
- 上一版本记录：0.0.4

---

# 变更说明 0.0.4

日期：2026-03-17

## 0.0.4 更新内容（新增）
1. 根据当前代码引脚定义重排并美化 Wokwi 电路图布局：主控居中、传感器左侧、执行器右侧。
2. 将红外拓扑明确为 ESP32 UART2 对接 ESP8266（不再表现为 ESP32 直连红外器件）。
3. 统一连线视觉规范：
- 电源 `red`、地线 `black`
- UART 桥接：TX/RX 分别使用 `purple` / `cyan`
- 数字控制线 `orange`、模拟信号线 `blue`
4. 对走线做主干化与避让优化，减少交叉和遮挡，提升模拟器可读性。
5. 变更文件：`dragram.json`。

## 版本说明
- 当前版本：0.0.4
- 上一版本记录：0.0.3

---

# 变更说明 0.0.3

日期：2026-03-17

## 0.0.3 更新内容（新增）
1. 将项目中全部 ArduinoJson 旧语法迁移到 v7 推荐写法。
2. 在 [src/HomeService.cpp](src/HomeService.cpp) 中移除旧接口：
- `DynamicJsonDocument` -> `JsonDocument`
- `doc.containsKey("...")` -> `!doc["..."].isNull()`
3. 重新构建验证，确保迁移后功能和编译通过。
4. 说明：`web/` 目录为上位机网页联调目录，由你手动维护，本次未修改其内容。

## 版本说明
- 当前版本：0.0.3
- 上一版本记录：0.0.2

---

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
