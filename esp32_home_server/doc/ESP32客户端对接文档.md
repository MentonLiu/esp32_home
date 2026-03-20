# ESP32 客户端对接文档

本文档面向“第二块 ESP32 客户端”开发，目标是让客户端可以：

1. 显示服务端当前传感器数据与设备状态
2. 向服务端发送控制指令

本文档以当前工作区源码实际实现为准，不以历史版本说明为准。

## 1. 服务端能力概览

当前服务端由一块 ESP32-S3 运行，提供两类对接方式：

1. HTTP 本地接口
2. MQTT 云端接口

当前对接前提是：

1. ESP32 客户端与服务端始终处于同一个局域网内
2. 这个局域网可能是家用路由器 Wi-Fi，也可能是服务端开启的 `esp32-server` 热点

客户端如果只是局域网显示和控制，优先建议走 HTTP。
客户端如果后面要接入云端或和其他设备联动，再接 MQTT。

## 2. 服务端运行模式

服务端有两种模式：

### 2.1 Cloud 模式

- 服务端连上路由器 Wi-Fi 后进入该模式
- Web 接口仍可用
- MQTT 可用
- `GET /api/status` 返回 `mode = "cloud"`

### 2.2 Local AP 模式

- 服务端连不上外网时进入该模式
- 会启动热点，供客户端接入同一个局域网
- MQTT 不可用
- Web 接口可用
- `GET /api/status` 返回 `mode = "local_ap"`

当前热点配置：

- SSID：`esp32-server`
- Password：`lbl450981`

说明：

- 服务端每 30 秒重新检查一次网络状态
- 有网时会关闭 AP，没网时会重新开启 AP
- 如果你的客户端长期在线，建议加入“断线重连”和“IP 变化重发现”逻辑

## 3. 推荐对接方案

推荐优先使用 HTTP：

1. 让客户端和服务端接入同一个局域网
2. 获取服务端在该局域网中的 IP
3. 轮询 `GET /api/status` 显示数据
4. 通过 `POST /api/control` 发送控制命令

局域网分两种情况：

1. 路由器 Wi-Fi 局域网
   服务端连上家用 Wi-Fi，客户端也连上同一路由器，通过服务端局域网 IP 访问它。
2. 服务端热点局域网
   服务端开启 `esp32-server` 热点，客户端连入该热点，通过 AP 网段 IP 访问它。

在服务端热点局域网下，默认可先尝试 `192.168.4.1`。

## 4. HTTP 接口

服务端内置 WebServer，端口固定为 `80`。

### 4.1 获取状态

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

- `mode`：服务端模式，`cloud` 或 `local_ap`
- `ip`：当前服务端在局域网中的 IP
- `temperatureC`：温度，单位摄氏度
- `humidityPercent`：湿度百分比
- `lightPercent`：光照百分比，范围 `0-100`
- `mq2Percent`：烟雾传感器百分比，范围 `0-100`
- `smokeLevel`：烟雾等级，枚举值为 `green`、`blue`、`yellow`、`red`
- `flameDetected`：是否检测到火焰
- `fanSpeedPercent`：风扇当前速度百分比，范围 `0-100`
- `curtainAngle`：窗帘当前角度，范围 `0-180`
- `error`：本次传感器采样是否有错误
- `errorMessage`：错误说明，当前常见值为 `dht_read_failed`

显示建议：

- `smokeLevel=green`：安全
- `smokeLevel=blue`：正常偏低
- `smokeLevel=yellow`：警告
- `smokeLevel=red`：危险

### 4.2 发送控制指令

请求：

```http
POST /api/control HTTP/1.1
Host: <server-ip>
Content-Type: application/json
```

返回固定为：

```json
{"ok":true}
```

注意：

- 即使 JSON 内容非法或设备类型错误，HTTP 仍然会返回 `200` 和 `{"ok":true}`
- 真正的执行结果目前主要通过 MQTT 状态主题上报，不通过 HTTP 返回
- 所以 HTTP 客户端发送命令后，建议再重新拉取一次 `/api/status` 做结果确认

## 5. 控制命令格式

所有控制命令都使用 JSON，公共字段：

- `device`：目标设备类型

当前支持的 `device`：

- `fan`
- `curtain`
- `ir`

### 5.1 风扇控制

按档位控制：

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

按速度百分比控制：

```json
{"device":"fan","speedPercent":80}
```

说明：

- `mode` 支持 `off`、`low`、`medium`、`high`
- `speedPercent` 范围建议传 `0-100`
- 如果同时传 `mode` 和 `speedPercent`，源码会先按 `mode` 设置，再按 `speedPercent` 覆盖

### 5.2 窗帘控制

按角度控制：

```json
{"device":"curtain","angle":135}
```

按预设档位控制：

```json
{"device":"curtain","preset":2}
```

预设档位映射如下：

- `0` -> `0°`
- `1` -> `45°`
- `2` -> `90°`
- `3` -> `135°`
- `4` -> `180°`

说明：

- `angle` 建议传 `0-180`
- 如果同时传 `angle` 和 `preset`，源码会先设置 `angle`，再设置 `preset`，最终以 `preset` 为准

### 5.3 红外桥接控制

服务端不会直接发红外，而是把命令通过 UART 转发给另一个 ESP8266 红外桥。

通用协议发送：

```json
{"device":"ir","action":"send","protocol":"NEC","address":1,"command":2,"repeats":0}
```

扩展动作发送：

```json
{"device":"ir","action":"learn","args":{"slot":"tv_power"}}
```

完整 JSON 透传：

```json
{"device":"ir","action":"send_json","commandJson":"{\"device\":\"ir\",\"action\":\"raw_send\",\"args\":{\"raw\":[9000,4500,560,560]}}"}
```

说明：

- `send`：固定字段为 `protocol`、`address`、`command`、`repeats`
- `send_json`：直接透传完整 JSON 给 ESP8266
- 其他任意 `action`：会被转为 `{"device":"ir","action":"xxx","args":...}` 发送给 ESP8266
- 当前源码已经不再支持旧的 `send_nec` 分支，客户端不要再用这个旧命令

## 6. MQTT 接口

仅在 `cloud` 模式下有效。

### 6.1 订阅主题

- 传感器数据：`esp32/home/sensors`
- 状态消息：`esp32/home/status`
- 报警消息：`esp32/home/alarm`

### 6.2 控制主题

- 下发控制：`esp32/home/control`

MQTT 控制消息体与 HTTP `POST /api/control` 的 JSON 完全一致。

### 6.3 传感器数据格式

主题：`esp32/home/sensors`

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
  "error": false
}
```

说明：

- 发布周期约 `500ms`
- `timestamp` 是服务端运行后的 `millis()`，不是 Unix 时间戳
- 如果采样失败，会额外包含 `errorMessage`

### 6.4 状态消息格式

主题：`esp32/home/status`

示例：

```json
{
  "type": "automation",
  "message": "curtain_open_07_00",
  "timestamp": 234567
}
```

常见 `type`：

- `command`
- `error`
- `automation`
- `rtc`
- `ir_command`
- `ir`

补充说明：

- `timestamp` 同样是 `millis()`
- `message` 有时是普通字符串，有时会被塞入一段 JSON 字符串，客户端建议按文本展示

### 6.5 报警消息格式

主题：`esp32/home/alarm`

当前已实现的报警：

```json
{
  "type": "fire",
  "message": "flame_over_5min_fire_service_alert",
  "timestamp": 345678
}
```

触发条件：

- 火焰持续检测超过 5 分钟
- 且服务端当前处于 `cloud` 模式

## 7. 服务端自动行为

客户端显示状态时，建议考虑这些自动控制逻辑，避免误判为“有人手动改了状态”。

### 7.1 定时窗帘

- 每天 `07:00` 自动全开
- 每天 `22:00` 自动全关

### 7.2 烟雾联动

- `mq2Percent >= 75`：风扇自动切到高速
- `mq2Percent >= 90`：蜂鸣器每约 1 秒短鸣一次

### 7.3 火焰联动

- 检测火焰超过 `45 秒`：蜂鸣器循环“短短长”
- 检测火焰超过 `5 分钟` 且在云模式：发送火警 MQTT 报警

## 8. 客户端开发建议

### 8.1 轮询周期

建议 HTTP 客户端每 `500ms` 到 `1000ms` 拉一次 `/api/status`。

原因：

- 传感器更新周期本身就是 `500ms`
- 轮询过快收益不大
- 轮询过慢会影响界面实时性

### 8.2 命令确认方式

推荐流程：

1. 发送 `POST /api/control`
2. 延时 `200ms ~ 500ms`
3. 再次 `GET /api/status`
4. 用返回状态更新屏幕

### 8.3 容错建议

- HTTP 失败时自动重试
- 如果当前是路由器局域网，连不上服务端 IP 时，先确认客户端和服务端是否还在同一路由器下
- 如果当前是服务端热点局域网，连不上时尝试重新连接 `esp32-server`
- 如果客户端已经知道上次 IP，也不要长期写死，模式切换时 IP 可能变化
- 解析 JSON 时，为缺失字段准备默认值

## 9. 最小联调清单

在你开始写 ESP32 客户端前，建议先完成下面 4 项联调：

1. 确认客户端和服务端已经在同一个局域网内
2. 确认服务端 IP
3. 浏览器访问 `http://<server-ip>/api/status` 能看到 JSON
4. 用 Postman 或串口工具向 `/api/control` 发一条风扇命令并观察状态变化
5. 确认你的客户端屏幕字段和 `/api/status` 字段一一对应

## 10. 建议的客户端显示字段

如果你要做一个小屏幕客户端，建议优先显示这些字段：

- 网络模式：`mode`
- IP：`ip`
- 温度：`temperatureC`
- 湿度：`humidityPercent`
- 光照：`lightPercent`
- 烟雾等级：`smokeLevel`
- 烟雾百分比：`mq2Percent`
- 火焰状态：`flameDetected`
- 风扇速度：`fanSpeedPercent`
- 窗帘角度：`curtainAngle`
- 采样错误：`error` / `errorMessage`

## 11. 当前源码里的几个注意点

1. `/api/status` 没有返回独立时间字段，如果客户端需要显示“数据更新时间”，需要自己记录本地刷新时间。
2. HTTP 控制接口不会把执行失败直接返回给客户端，所以不要只看 `{"ok":true}`。
3. MQTT 只有在 `cloud` 模式才有效，本地 AP 模式下不要依赖 MQTT。
4. 传感器消息和状态消息里的 `timestamp` 都是设备启动后的毫秒数，不是标准时间戳。
5. 红外控制是“ESP32 服务端 -> UART -> ESP8266 红外桥”的两级架构，是否真正发射成功还取决于 ESP8266 固件。

## 12. 一组可直接测试的命令

关闭风扇：

```json
{"device":"fan","mode":"off"}
```

风扇 80%：

```json
{"device":"fan","speedPercent":80}
```

窗帘全开：

```json
{"device":"curtain","preset":4}
```

窗帘设置到 120 度：

```json
{"device":"curtain","angle":120}
```

红外发送 NEC：

```json
{"device":"ir","action":"send","protocol":"NEC","address":1,"command":2,"repeats":0}
```
