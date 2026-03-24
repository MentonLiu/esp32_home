# ESP32 Home Server

基于 ESP32-S3 + PlatformIO + Arduino 的智能家居服务端。当前实现按项目根目录文档重建，开发重点收敛到“服务端 HTTP + 客户端面板”。

本轮已经重点处理本地网页控制延迟、窗帘无极调节手感不明显、直流电机动作时舵机易受干扰，以及告警期间程序卡顿等问题。

## 当前重点

- 主家庭 Wi-Fi 已固定为 `iPadmini / lbl450981`
- MQTT 目前不作为当前交付主功能
- 红外功能已下线，不在当前交付范围
- 服务端网页不再承担项目结构页展示

## 当前目录结构

- `include/`
  头文件，包含引脚定义、数据契约、传感器/执行器接口、联网与自动化模块接口。
- `src/`
  固件实现，按“采集、控制、联网、页面、自动化、中央调度”拆分。
- `web/`
  LittleFS 页面资源，目前只保留主控制台。
- `../doc/`
  项目根目录统一文档中心。
- `../project-map.html`
  仓库级结构说明页。
- `platformio.ini`
  PlatformIO 构建配置。
- `changes.md`
  本子项目变更记录。

## 主要模块

- `CentralProcessor`
  系统总控，负责统一初始化与主循环调度。
- `Sensor` + `SensorDataProcessor`
  负责 DHT11、光照、MQ2、火焰采样，并标准化为统一状态数据。
- `Controllerr` + `ControllerCommandProcessor`
  负责风扇、窗帘、蜂鸣器控制；当前窗帘和蜂鸣器都已改为非阻塞推进模型。
- `ConnectivityManager`
  负责家用 Wi-Fi、`esp32-server` 热点、WebServer 和占位 MQTT 连接管理。
- `LocalProcessingProgram`
  暴露 `/`、`/api/status`、`/api/control`。
- `AutomationEngine`
  负责定时窗帘、烟雾联动、火焰联动。

## 网络与运行模式

系统有两种模式：

- `cloud`
  连接 `iPadmini` 后进入该模式，本地网页继续可用；MQTT 代码可保留，但当前不作为主流程。
- `local_ap`
  无法联网时自动开启热点：
  - SSID: `esp32-server`
  - Password: `lbl450981`

每 30 秒会重新检查联网状态，有网时回到 `cloud`，没网时保持 `local_ap`。

说明：

- 现在重连家庭 Wi-Fi 已改为非阻塞方式，不会再因为重连等待而把本地网页控制卡住几秒。

## HTTP 接口

### `GET /api/status`

返回当前模式、IP、传感器状态、风扇档位、风扇速度、窗帘角度和错误信息。

示例：

```json
{
  "mode": "local_ap",
  "ip": "192.168.4.1",
  "temperatureC": 26.3,
  "humidityPercent": 58.0,
  "lightPercent": 72,
  "mq2Percent": 18,
  "smokeLevel": "green",
  "flameDetected": false,
  "fanMode": "off",
  "fanSpeedPercent": 0,
  "curtainAngle": 90,
  "error": false,
  "errorMessage": ""
}
```

### `POST /api/control`

用于发送风扇、窗帘控制命令，会返回真实执行结果：

```json
{
  "ok": true,
  "stateChanged": true,
  "type": "local_command",
  "message": "fan_updated"
}
```

控制命令示例：

```json
{"device":"fan","mode":"off"}
```

```json
{"device":"fan","mode":"high"}
```

```json
{"device":"fan","speedPercent":80}
```

```json
{"device":"curtain","angle":135}
```

## 风扇 PWM 调速（ULN2003A）

当前风扇控制已按“直流电机 PWM 调速”实现，适配 ULN2003A 这类低边驱动方案。

### 推荐接线

- ESP32 `GPIO18`（`pins::FAN_PWM`） -> ULN2003A `IN1`
- ULN2003A `OUT1` -> 直流电机负极
- 直流电机正极 -> `+5V`
- ULN2003A `GND` -> 电源地
- ESP32 `GND` 与电机电源地共地
- ULN2003A `COM` -> `+5V`（使用芯片内部续流二极管）

### 功能能力

- 支持：模式控制、单方向 PWM 调速、自动化调速
- 不支持：反转（需要 H 桥驱动）

### 重要说明

- 不要把电机直接接到 ESP32 GPIO。
- 如果直流电机和舵机共用一组供电，仍可能出现舵机抖动、误动作或 ESP32 复位，推荐分开供电并共地。
- 建议在电机驱动侧保留续流回路，并在电机/舵机电源附近增加足够的去耦和储能电容。

## 窗帘与舵机说明

当前窗帘采用双舵机反向联动，软件侧已做两项稳定性处理：

- 舵机只有在收到新角度时才重新挂接 PWM。
- 到位约 450ms 后会自动释放 PWM，减少持续抖动和供电占用。

这能缓解“控制直流电机时舵机跟着动”的现象，但如果现场仍有联动，通常优先排查供电和布线，而不是应用层控制逻辑。

## MQTT 说明

当前版本里 MQTT 只保留占位接口，暂不作为本轮重点。等真实 Broker、主题和权限方案定下来，再恢复完整实现。

## 页面说明

LittleFS 页面资源在 `web/` 目录，目前只保留 [`web/index.html`](web/index.html) 作为服务端控制页。

本地控制页当前包含这些优化：

- 窗帘和风扇滑条采用轻节流发送，拖动时更顺。
- 松手后会立即发送最终值，保证终点准确。
- 滑动期间不再被状态轮询覆盖当前手势。
- 状态轮询默认 1 秒一次，控制完成后会短延迟补拉一次状态。

## 构建与烧录

在项目根目录执行：

```bash
pio run
pio run -t buildfs
pio run -t upload
pio run -t uploadfs
pio device monitor -b 115200
```

## 当前验证状态

- `pio run` 成功
- `pio run -t buildfs` 成功
