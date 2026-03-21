# ESP32 客户端与 MQTT 上位机对接文档

本文档基于当前项目的新架构编写，面向两类接入方：

1. ESP32 局域网客户端
2. MQTT 云端上位机

当前架构已经把这两条链路拆开：

- `LocalProcessingProgram`：负责局域网网页与 HTTP 接口
- `MqttUpstream`：负责 MQTT 主题与云端配置
- `CentralProcessor`：负责统一装配、状态发布和消息分发

因此，后续开发时请不要再把“ESP32 客户端”和“MQTT 上位机”当成同一条接入方式。

## 1. 新架构下的通信分工

### 1.1 ESP32 局域网客户端

ESP32 客户端通过局域网直接访问服务端 HTTP 接口，适合做：

- 小屏显示终端
- 本地按键/旋钮控制器
- 局域网内的第二块 ESP32 面板

它不依赖 MQTT 才能工作。

### 1.2 MQTT 云端上位机

MQTT 上位机通过 Mosquitto 等 Broker 与服务端通信，适合做：

- 云端控制台
- 远程控制程序
- 数据汇聚与告警平台

它只在服务端处于 `cloud` 模式时有效。

## 2. 服务端运行模式

服务端运行模式由 `ConnectivityManager` 管理。

### 2.1 `cloud` 模式

- 服务端连接到路由器 Wi-Fi
- 本地 HTTP 接口可继续使用
- MQTT 连接可用
- `GET /api/status` 返回 `mode = "cloud"`

当前源码内置的站点 Wi-Fi 配置在 [CentralProcessor.cpp](../../esp32_home_server/src/CentralProcessor.cpp)：

- SSID：`home-WiFi`
- Password：空字符串

### 2.2 `local_ap` 模式

- 服务端无法连上外网时自动进入该模式
- 服务端开启热点，供局域网客户端接入
- MQTT 不可用
- HTTP 接口可用
- `GET /api/status` 返回 `mode = "local_ap"`

当前热点配置同样在 [CentralProcessor.cpp](../../esp32_home_server/src/CentralProcessor.cpp)：

- SSID：`esp32-server`
- Password：`lbl450981`

补充说明：

- 服务端每 30 秒重新检查一次联网状态
- 有网时切回 `cloud`
- 无网时切回 `local_ap`

## 3. ESP32 局域网客户端对接

这部分只描述“第二块 ESP32 客户端如何和服务端通信”。

### 3.1 通信前提

客户端和服务端必须在同一个局域网内。这个局域网可能是：

1. 家用路由器 Wi-Fi
2. 服务端自己开启的 `esp32-server` 热点

推荐流程：

1. 客户端连入与服务端相同的网络
2. 获取服务端当前 IP
3. 通过 HTTP 访问 `/api/status`
4. 通过 HTTP 向 `/api/control` 发送命令

如果是服务端热点场景，通常可以先尝试：

- `http://192.168.4.1/api/status`

### 3.2 本地 HTTP 接口归属

本地接口由 [LocalProcessingProgram.cpp](../../esp32_home_server/src/LocalProcessingProgram.cpp) 提供。

当前已实现的路由有：

- `GET /`
- `GET /project-map`
- `GET /api/status`
- `POST /api/control`

ESP32 客户端真正需要的是后两个接口。

### 3.3 获取状态

请求：

```http
GET /api/status HTTP/1.1
Host: <server-ip>
```

返回示例：

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

字段说明：

- `mode`：服务端运行模式，`cloud` 或 `local_ap`
- `ip`：服务端当前局域网 IP
- `temperatureC`：温度，单位摄氏度
- `humidityPercent`：湿度百分比
- `lightPercent`：光照百分比，范围 `0-100`
- `mq2Percent`：MQ2 百分比，范围 `0-100`
- `smokeLevel`：`green`、`blue`、`yellow`、`red`
- `flameDetected`：是否检测到火焰
- `fanSpeedPercent`：风扇当前速度百分比
- `curtainAngle`：窗帘当前角度，范围 `0-180`
- `error`：采样是否异常
- `errorMessage`：错误信息，常见值为 `dht_read_failed`

这些字段由 [SensorDataProcessor.cpp](../../esp32_home_server/src/SensorDataProcessor.cpp) 中的 `buildStatusJson()` 生成。

### 3.4 发送控制命令

请求：

```http
POST /api/control HTTP/1.1
Host: <server-ip>
Content-Type: application/json
```

返回：

```json
{"ok":true}
```

注意：

- 当前 HTTP 返回值固定为 `{"ok":true}`
- 即使命令 JSON 非法，HTTP 仍然返回 200
- 真正的命令解析结果由 `ControllerCommandProcessor` 决定
- 所以客户端发命令后，应该重新拉一次 `/api/status` 做状态确认

### 3.5 局域网控制命令格式

所有命令都由 [ControllerCommandProcessor.cpp](../../esp32_home_server/src/ControllerCommandProcessor.cpp) 解析。

公共字段：

- `device`

支持的设备：

- `fan`
- `curtain`
- `ir`

#### 风扇

按档位：

```json
{"device":"fan","mode":"off"}
```

```json
{"device":"fan","mode":"low"}
```

```json
{"device":"fan","mode":"medium"}
```

```json
{"device":"fan","mode":"high"}
```

按百分比：

```json
{"device":"fan","speedPercent":80}
```

说明：

- `mode` 允许 `off`、`low`、`medium`、`high`
- `speedPercent` 建议范围 `0-100`
- 如果同时传 `mode` 和 `speedPercent`，最终以 `speedPercent` 覆盖结果为准

#### 窗帘

按角度：

```json
{"device":"curtain","angle":135}
```

按预设：

```json
{"device":"curtain","preset":2}
```

预设映射：

- `0 -> 0°`
- `1 -> 45°`
- `2 -> 90°`
- `3 -> 135°`
- `4 -> 180°`

说明：

- `angle` 建议范围 `0-180`
- 如果同时传 `angle` 和 `preset`，最终以 `preset` 对应角度为准

#### 红外桥接

服务端不会直接发红外，而是通过 UART 转发给外部桥接模块。

协议发送：

```json
{"device":"ir","action":"send","protocol":"NEC","address":1,"command":2,"repeats":0}
```

动作发送：

```json
{"device":"ir","action":"learn","args":{"slot":"tv_power"}}
```

完整 JSON 透传：

```json
{"device":"ir","action":"send_json","commandJson":"{\"device\":\"ir\",\"action\":\"raw_send\",\"args\":{\"raw\":[9000,4500,560,560]}}"}
```

说明：

- `send`：使用 `protocol/address/command/repeats`
- `send_json`：直接透传整段 JSON
- 其他任意 `action`：转成 `args` 结构发给桥接模块

### 3.6 轮询与刷新建议

建议 ESP32 客户端按以下节奏工作：

1. 每 `500ms ~ 1000ms` 拉取一次 `/api/status`
2. 用户下发命令后等待 `200ms ~ 500ms`
3. 再次拉取 `/api/status`
4. 用新状态更新屏幕

原因：

- 传感器本身采样节奏是 `500ms`
- 页面端本身也是按 `500ms` 级别设计的

### 3.7 局域网客户端注意点

1. 客户端不要把 MQTT 当作本地 ESP32 控制链路的必需条件。
2. 模式切换时服务端 IP 可能变化，不要长期写死。
3. `mode=local_ap` 时，HTTP 仍可用，但 MQTT 不可用。
4. `/api/status` 里没有标准时间字段，只有当前状态，不带 RTC 时间文本。

## 4. MQTT 云端上位机对接

这一节单独描述 MQTT 上位机，不再和 ESP32 局域网客户端混写。

### 4.1 模块归属

MQTT 上位机相关配置集中在：

- [MqttUpstream.cpp](../../esp32_home_server/src/MqttUpstream.cpp)
- [ConnectivityManager.cpp](../../esp32_home_server/src/ConnectivityManager.cpp)
- [CentralProcessor.cpp](../../esp32_home_server/src/CentralProcessor.cpp)

职责划分：

- `MqttUpstream`：定义 Broker 配置和主题名
- `ConnectivityManager`：负责 MQTT 建连、重连、订阅
- `CentralProcessor`：负责把传感器、状态、报警、控制命令接到 MQTT 链路

### 4.2 生效条件

MQTT 只在服务端处于 `cloud` 模式时工作。

也就是说：

- 服务端连上路由器 Wi-Fi：MQTT 有效
- 服务端只开 `esp32-server` 热点：MQTT 无效

### 4.3 当前 Broker 配置

当前源码中的占位配置在 [MqttUpstream.cpp](../../esp32_home_server/src/MqttUpstream.cpp)：

```cpp
host = "example.mosquitto.server"
port = 1883
clientId = "esp32-home-server"
user = nullptr
password = nullptr
```

上线前你需要按真实环境替换这些值。

### 4.4 主题划分

当前主题如下：

- 传感器主题：`esp32/home/sensors`
- 状态主题：`esp32/home/status`
- 报警主题：`esp32/home/alarm`
- 控制主题：`esp32/home/control`

### 4.5 上行传感器数据

主题：

```text
esp32/home/sensors
```

示例：

```json
{
  "sensorType": "home_snapshot",
  "timestamp": 123456,
  "temperatureC": 26.3,
  "humidityPercent": 58.0,
  "lightPercent": 72,
  "mq2Percent": 18,
  "smokeLevel": "green",
  "flameDetected": false,
  "error": false,
  "errorMessage": ""
}
```

说明：

- 由 `SensorDataProcessor::buildSensorJson()` 生成
- 采样与发布节流周期约为 `500ms`
- `timestamp` 是 `millis()`，不是 Unix 时间戳

### 4.6 上行状态消息

主题：

```text
esp32/home/status
```

示例：

```json
{
  "type": "local_command",
  "message": "fan_updated",
  "mode": "cloud",
  "timestamp": 234567
}
```

当前状态消息统一由 `CentralProcessor::publishStatus()` 生成，固定字段有：

- `type`
- `message`
- `mode`
- `timestamp`

常见 `type`：

- `local_command`
- `cloud_command`
- `automation`
- `rtc`
- `error`
- `ir_bridge_rx`

### 4.7 上行报警消息

主题：

```text
esp32/home/alarm
```

当前已实现报警：

```json
{
  "type": "fire",
  "message": "flame_over_5min_fire_service_alert",
  "mode": "cloud",
  "timestamp": 345678
}
```

触发条件：

- 火焰持续超过 5 分钟
- 服务端当前为 `cloud` 模式

### 4.8 MQTT 控制下行

主题：

```text
esp32/home/control
```

控制消息体与 HTTP `/api/control` 完全一致，也由 `ControllerCommandProcessor::processCommandJson()` 统一解析。

示例：

```json
{"device":"fan","mode":"high"}
```

```json
{"device":"curtain","preset":4}
```

```json
{"device":"ir","action":"send","protocol":"NEC","address":1,"command":2,"repeats":0}
```

### 4.9 MQTT 上位机注意点

1. 本地热点模式下，MQTT 不工作。
2. `message` 字段是面向事件的短文本，不是完整状态快照。
3. 如果上位机想拿全量状态，应该结合 `esp32/home/sensors` 和自己的状态缓存。
4. `timestamp` 是设备开机后的毫秒计数，不是标准日期时间。

## 5. 自动化行为

自动化由 [AutomationEngine.cpp](../../esp32_home_server/src/AutomationEngine.cpp) 负责。

当前已实现：

- 每天 `07:00` 自动打开窗帘
- 每天 `22:00` 自动关闭窗帘
- `mq2Percent >= 75` 时自动风扇高速
- `mq2Percent >= 90` 时蜂鸣器短鸣
- 火焰持续 `45s` 后播放“短短长”
- 火焰持续 `5min` 且处于 `cloud` 模式时发布火警报警

不论是本地客户端还是 MQTT 上位机，都要把这些自动动作考虑进去，不要把它们误判成用户手动操作。

## 6. 联调建议

### 6.1 ESP32 局域网客户端联调

建议顺序：

1. 让客户端和服务端进入同一个局域网
2. 浏览器访问 `http://<server-ip>/api/status`
3. 用 HTTP 工具向 `/api/control` 发送风扇和窗帘命令
4. 再开始写 ESP32 客户端显示和控制逻辑

### 6.2 MQTT 上位机联调

建议顺序：

1. 把 Broker 地址改成真实 Mosquitto 服务
2. 确认服务端能进入 `cloud` 模式
3. 订阅 `esp32/home/sensors`
4. 订阅 `esp32/home/status`
5. 向 `esp32/home/control` 发布一条控制命令

## 7. 当前源码下最值得注意的点

1. HTTP 是局域网 ESP32 客户端的主通道。
2. MQTT 是云端上位机的独立通道，不要再混为一谈。
3. HTTP 控制接口目前不会把命令失败直接返回给客户端。
4. MQTT 主题是完整的，但 Broker 配置仍是占位值。
5. 本地页面、ESP32 客户端、MQTT 控制三者底层共用同一个 `ControllerCommandProcessor`，所以命令格式是一致的。
