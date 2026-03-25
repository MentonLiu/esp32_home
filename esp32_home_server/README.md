# ESP32 Home Server

基于 ESP32-S3 + PlatformIO + Arduino 的智能家居服务端固件。

当前版本聚焦本地可控、自动化稳定和网页实时交互：

- 本地 Web 控制面板（LittleFS）
- 非阻塞网络切换（路由器 Wi-Fi 与本地热点自动切换）
- 风扇 PWM 调速、双舵机窗帘、蜂鸣器告警
- 传感器采样标准化与统一状态接口
- 自动化规则（时间、温度、烟雾、光照）

## 1. 项目定位

这是一个“服务端固件”，主要对外提供：

- 页面入口：`/`
- 状态接口：`/api/status`
- 控制接口：`/api/control`

前端页面由设备内置 WebServer 直接提供，不依赖外部云服务即可使用。

## 2. 当前技术栈

- 主控：ESP32-S3 DevKitC-1
- 框架：Arduino（PlatformIO）
- 文件系统：LittleFS（`web/` 作为数据目录）
- 主要依赖：
  - ArduinoJson
  - PubSubClient（当前仅保留占位接入能力）
  - DHT sensor library
  - ESP32Servo
  - RTClib

## 3. 目录说明

- `include/`：模块接口与数据契约
- `src/`：固件实现
- `web/`：LittleFS 页面资源（当前主页面为 `index.html`）
- `platformio.ini`：编译、上传、依赖配置
- `changes.md`：变更记录

## 4. 模块架构

系统由 `CentralProcessor` 统一编排，启动与循环流程如下：

1. 传感器初始化
2. 执行器初始化
3. 网络与 WebServer 初始化
4. 自动化引擎初始化
5. 本地 HTTP 程序初始化
6. 进入主循环（网络 -> 控制器 -> 传感器 -> 自动化）

核心模块：

- `ConnectivityManager`
  - 管理 STA/AP 模式切换
  - 提供 WebServer
  - 预留 MQTT 生命周期管理
- `LocalProcessingProgram`
  - 注册并处理 `/`、`/api/status`、`/api/control`
- `SensorHub` + `SensorDataProcessor`
  - 采样 DHT11、光照、MQ2
  - 输出标准化状态结构与 JSON
- `Controllerr` + `ControllerCommandProcessor`
  - 控制风扇、窗帘、蜂鸣器
  - 解析控制命令 JSON
- `AutomationEngine`
  - 时间调度、温度联动、烟雾联动、光照联动

## 5. 网络模式

系统支持两种运行模式：

- `cloud`
  - 已连接家庭路由器（STA）
  - 通过路由器 IP 访问，也可尝试 mDNS：`http://esp32-home-server.local/`
- `local_ap`
  - 未连上家庭路由器时自动开启 AP 热点
  - 默认热点：
    - SSID: `esp32-server`
    - Password: `lbl450981`

模式评估每 30 秒执行一次；重连 STA 为非阻塞，不会明显阻塞本地控制。

## 6. Wi-Fi 与设备访问

当前固件中家庭 Wi-Fi 在代码内固定（`src/CentralProcessor.cpp`）：

- SSID: `HW-2103`
- Password: `20220715`

建议按现场环境修改后重新编译烧录。

## 7. HTTP API

### 7.1 GET /api/status

返回系统完整状态，包含平铺字段和分组字段（`sensor`、`controller`）。

示例：

```json
{
  "mode": "local_ap",
  "ip": "192.168.4.1",
  "sensorTimestamp": 1234567,
  "temperatureC": 26.3,
  "humidityPercent": 58.0,
  "lightPercent": 72,
  "mq2Percent": 18,
  "smokeLevel": "green",
  "fanMode": "off",
  "fanSpeedPercent": 0,
  "curtainAngle": 90,
  "error": false,
  "errorMessage": "",
  "sensor": {
    "temperatureC": 26.3,
    "humidityPercent": 58.0,
    "lightPercent": 72,
    "mq2Percent": 18,
    "smokeLevel": "green",
    "timestamp": 1234567,
    "error": false,
    "errorMessage": ""
  },
  "controller": {
    "fanMode": "off",
    "fanSpeedPercent": 0,
    "curtainAngle": 90
  }
}
```

### 7.2 POST /api/control

请求体为 JSON，支持 `fan` 与 `curtain` 两类设备控制。

统一返回：

```json
{
  "ok": true,
  "stateChanged": true,
  "type": "local_command",
  "message": "fan_updated"
}
```

控制示例：

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

```json
{"device":"fan","speedPercent":80}
```

```json
{"device":"curtain","angle":135}
```

```json
{"device":"curtain","preset":3}
```

常见失败消息：

- `invalid_control_json:*`
- `device_missing`
- `unknown_device`
- `fan_mode_invalid`
- `fan_command_missing`
- `curtain_command_missing`

## 8. 自动化规则（当前实现）

### 8.1 时间窗帘

- 每日 07:00：窗帘打开到预设 4（全开）
- 每日 22:00：窗帘关闭到预设 0（全关）
- 同一天内去重触发

### 8.2 温度联动风扇

- 温度 >= 32°C：触发风扇高档（带 10 分钟动作冷却）
- 温度 <= 29°C：退出高温增强状态

### 8.3 烟雾联动

- MQ2 >= 75%：风扇自动高档
- MQ2 >= 90%：蜂鸣器约每 1.2 秒短鸣一次

### 8.4 光照联动窗帘（仅白天 08:00-18:00）

- 强光进入阈值 >= 85%：切防眩（预设 1）
- 弱光进入阈值 <= 30%：切补光（预设 3）
- 防抖与滞回阈值：
  - 防眩退出阈值 <= 70%
  - 补光退出阈值 >= 45%
- 冷却时间：10 分钟
- 手动窗帘控制后 30 分钟内，自动化不接管

### 8.5 时间源优先级

1. NTP（联网后）
2. DS3231 RTC（若可用且时间有效）
3. 编译时刻 + 开机运行时长（回退方案）

## 9. 传感器与执行器

### 9.1 传感器

- DHT11：温度、湿度
- 光照模拟量：百分比（反向映射）
- MQ2 模拟量：百分比

烟雾分级（由 MQ2 百分比映射）：

- `<25`：`green`
- `<50`：`blue`
- `<75`：`yellow`
- `>=75`：`red`

### 9.2 风扇（PWM 调速）

- 控制引脚：GPIO18
- LEDC：通道 5、2kHz、8bit
- 档位映射：
  - `off` -> 0%
  - `low` -> 30%
  - `medium` -> 65%
  - `high` -> 100%

### 9.3 双舵机窗帘

- 控制引脚：GPIO16、GPIO17
- 双舵机反向联动
- 仅动作时 attach，约 450ms 后自动 detach
- 预设角度：`[0, 45, 90, 135, 180]`

### 9.4 蜂鸣器

- 控制引脚：GPIO12
- LEDC：通道 6
- 非阻塞队列播放，避免阻塞主循环

## 10. 引脚定义

- DHT11 数据：GPIO13
- 光照模拟：GPIO4
- MQ2 模拟：GPIO5
- 风扇 PWM：GPIO18
- 窗帘舵机 A：GPIO16
- 窗帘舵机 B：GPIO17
- 蜂鸣器：GPIO12
- RTC I2C SDA：GPIO8
- RTC I2C SCL：GPIO9

## 11. 页面说明

页面文件位于 `web/index.html`，当前页面特性：

- 风扇与窗帘滑条轻节流发送（约 90ms）
- 滑动结束立即发送最终值
- 控制请求进行中时，状态轮询避免覆盖手势
- 默认每 1 秒轮询 `/api/status`

## 12. 构建与烧录

在项目根目录执行：

```bash
pio run
pio run -t buildfs
pio run -t upload
pio run -t uploadfs
pio device monitor -b 115200
```

说明：

- `data_dir` 已在 `platformio.ini` 中设置为 `web`，构建文件系统时会打包该目录。

## 13. 日志与调试

串口波特率：115200

日志格式：

```text
[HH:MM:SS] [LEVEL] [TAG] message
```

常见标签：`SYSTEM`、`INIT`、`NET`、`SENSOR`、`HTTP`、`DHT`

## 14. 硬件注意事项

- 风扇电机不要直接连接 ESP32 GPIO，必须使用驱动器（如 ULN2003A/MOSFET 模块）。
- 电机与舵机建议独立供电，并与 ESP32 共地。
- 电源侧建议增加去耦与储能电容，降低舵机抖动和复位风险。
- LEDC 通道建议保持当前分配：
  - 舵机：自动分配低通道
  - 风扇：通道 5
  - 蜂鸣器：通道 6

## 15. MQTT 状态

当前 MQTT 为占位能力：

- 主题和云端参数已集中定义
- 主流程未启用真实 Broker 连接

待 Broker、认证和主题规范确认后再启用完整云链路。

## 16. 已知边界

- 红外控制不在当前交付范围
- 云端控制链路暂未作为主流程验证
- 家庭 Wi-Fi 凭据暂存于代码常量，尚未做运行时配置化
