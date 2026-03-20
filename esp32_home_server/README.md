# ESP32 Home Server

基于 ESP32-S3 + PlatformIO + Arduino 的智能家居服务端。当前实现已经按 [`doc/功能要求.md`](doc/%E5%8A%9F%E8%83%BD%E8%A6%81%E6%B1%82.md) 和 `/doc` 下的流程图、架构图重建，目标是同时提供：

- 0.5 秒周期的环境传感器采集
- 本地网页控制台与 HTTP 接口
- 云端 MQTT 上行与控制接口预留
- 风扇、窗帘、蜂鸣器、红外桥接控制
- 断网自动切换热点、本地运行与自动化告警

## 项目图示

### 架构图

![项目架构图](doc/%E9%A1%B9%E7%9B%AE%E6%9E%B6%E6%9E%84.png)

### 功能流程图

![功能流程图](doc/%E5%8A%9F%E8%83%BD%E6%B5%81%E7%A8%8B.png)

## 当前目录结构

- `include/`
  头文件，包含引脚定义、统一数据契约、传感器/执行器接口、联网与自动化模块接口。
- `src/`
  固件实现，按“采集、控制、联网、页面、自动化、中央调度”拆分。
- `web/`
  LittleFS 页面资源，包含主控制台和项目结构图页面。
- `doc/`
  需求文档、客户端对接文档、流程图和架构图。
- `platformio.ini`
  PlatformIO 构建配置。
- `changes.md`
  版本变更记录。

## 主要模块

- `CentralProcessor`
  系统总控，负责统一初始化与主循环调度。
- `Sensor` + `SensorDataProcessor`
  负责 DHT11、光照、MQ2、火焰采样，并标准化为统一状态数据。
- `Controllerr` + `ControllerCommandProcessor`
  负责风扇 PWM 调速、双舵机窗帘、蜂鸣器和红外桥接命令处理。
- `ConnectivityManager`
  负责家用 Wi-Fi、`esp32-server` 热点、WebServer 和 MQTT 连接管理。
- `LocalProcessingProgram`
  暴露 `/`、`/project-map`、`/api/status`、`/api/control`。
- `AutomationEngine`
  负责定时窗帘、烟雾联动、火焰联动与本地/联网时间保持。

## 硬件接线

当前引脚定义见 [`include/pins.h`](include/pins.h)。

| 设备 | ESP32-S3 引脚 | 说明 |
| --- | --- | --- |
| DHT11 温湿度 | `GPIO13` | 保留 3 针 DHT11 方案 |
| 光照传感器 | `GPIO4` | 模拟量输入 |
| MQ2 烟雾传感器 | `GPIO5` | 模拟量输入 |
| 火焰传感器 | `GPIO6` | 模拟量输入 |
| 风扇继电器 / PWM | `GPIO18` | 排气扇控制 |
| SG90 舵机 A | `GPIO16` | 窗帘控制 |
| SG90 舵机 B | `GPIO17` | 窗帘控制 |
| 蜂鸣器 | `GPIO12` | 报警声输出 |
| 红外桥接 UART RX | `GPIO15` | 接外部桥接模块 TX |
| 红外桥接 UART TX | `GPIO14` | 接外部桥接模块 RX |
| RTC SDA | `GPIO8` | DS3231 I2C |
| RTC SCL | `GPIO9` | DS3231 I2C |

说明：

- 文档中提到的 `dragram.json` 当前不在仓库内，因此引脚表按现有项目与文档约束重建。
- 风扇、舵机、蜂鸣器建议使用稳定供电，避免高负载时复位。

## 网络与运行模式

系统有两种模式：

- `cloud`
  连接 `home-WiFi` 后进入该模式，本地网页继续可用，同时启用 MQTT。
- `local_ap`
  无法联网时自动开启热点：
  - SSID: `esp32-server`
  - Password: `lbl450981`

每 30 秒会重新检查联网状态，有网时回到 `cloud`，没网时保持 `local_ap`。

## 状态采集与自动化

- 传感器采样周期：`0.5s`
- 页面轮询周期：`0.5s`
- 自动化规则：
  - 每天 `07:00` 自动打开窗帘
  - 每天 `22:00` 自动关闭窗帘
  - MQ2 高浓度时自动把风扇拉到高速
  - MQ2 超高浓度时蜂鸣器短促报警
  - 火焰持续超过 `45s` 时触发“短短长”报警音
  - 火焰持续超过 `5min` 且处于云模式时，发布火警报警消息

## HTTP 接口

### `GET /api/status`

返回当前模式、IP、传感器状态、风扇速度、窗帘角度和错误信息。

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
  "fanSpeedPercent": 0,
  "curtainAngle": 90,
  "error": false,
  "errorMessage": ""
}
```

### `POST /api/control`

用于发送风扇、窗帘、红外控制命令。当前实现保持与客户端对接文档一致，HTTP 返回固定：

```json
{"ok":true}
```

控制命令示例：

```json
{"device":"fan","mode":"high"}
```

```json
{"device":"fan","speedPercent":80}
```

```json
{"device":"curtain","angle":135}
```

```json
{"device":"curtain","preset":2}
```

```json
{"device":"ir","action":"send","protocol":"NEC","address":1,"command":2,"repeats":0}
```

```json
{"device":"ir","action":"learn","args":{"slot":"tv_power"}}
```

```json
{"device":"ir","action":"send_json","commandJson":"{\"device\":\"ir\",\"action\":\"raw_send\",\"args\":{\"raw\":[9000,4500,560,560]}}"}
```

## MQTT 接口

当前第一版已经实现接口和主题约定，但云端服务地址仍是占位值，需要按实际环境填写。

- 状态主题：`esp32/home/status`
- 传感器主题：`esp32/home/sensors`
- 报警主题：`esp32/home/alarm`
- 控制主题：`esp32/home/control`

MQTT 配置位置：

- [`src/MqttUpstream.cpp`](src/MqttUpstream.cpp)
- [`src/CentralProcessor.cpp`](src/CentralProcessor.cpp)

## 页面说明

LittleFS 页面资源在 `web/` 目录：

- [`web/index.html`](web/index.html)
  实时展示传感器数据，并支持风扇、窗帘、红外桥接控制。
- [`web/project-map.html`](web/project-map.html)
  展示当前项目模块关系和运行流程。

## 构建与烧录

在项目根目录执行：

```bash
pio run
pio run -t buildfs
pio run -t upload
pio run -t uploadfs
pio device monitor -b 115200
```

说明：

- `pio run` 编译固件
- `pio run -t buildfs` 打包 `web/` 为 LittleFS 镜像
- `pio run -t uploadfs` 把页面资源写入设备

## 当前验证状态

已完成本地验证：

- `pio run` 成功
- `pio run -t buildfs` 成功

## 后续建议

- 将 `home-WiFi`、MQTT 域名、账号密码移到独立配置或 NVS。
- 如果后续拿到真实接线图，可以继续微调 [`include/pins.h`](include/pins.h)。
- 如果要对接第二块 ESP32 客户端，建议同时参考 [`doc/ESP32客户端对接文档.md`](doc/ESP32%E5%AE%A2%E6%88%B7%E7%AB%AF%E5%AF%B9%E6%8E%A5%E6%96%87%E6%A1%A3.md)。
