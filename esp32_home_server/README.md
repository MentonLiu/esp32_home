# ESP32 Home Server 使用说明书

## 1. 项目简介
本项目基于 ESP32 + PlatformIO（Arduino 框架），实现家庭环境监测与设备控制的服务端。

核心能力：
- 传感器采集：温湿度、光照、烟雾（MQ2）、火焰状态
- 设备控制：风扇继电器（含速度档位）、双舵机窗帘、蜂鸣器、红外收发
- 通信模式：
  - 可联网时：连接家用 Wi-Fi，使用 MQTT 对接云上位机
  - 无网络时：自动切换本地 AP 热点并提供本地网页控制台
- 自动化：按时开关窗帘、烟雾/火焰联动报警处理

## 2. 目录结构
- include/: 头文件（引脚、传感器、控制器、网络管理、服务总控）
- src/: 业务实现
- doc/: 需求文档与流程图
- platformio.ini: 编译配置与库依赖

## 3. 硬件与引脚说明
当前引脚定义见 include/pins.h。

主要映射：
- DHT22 数据脚: GPIO13（保留 DHT11 兼容接口）
- 光照传感器: GPIO34
- MQ2 烟雾传感器: GPIO36
- 火焰传感器占位: GPIO39
- 继电器风扇: GPIO18
- 舵机: GPIO19 / GPIO21（窗帘主控）
- 蜂鸣器: GPIO12
- 红外接收: GPIO5
- 红外发射: GPIO27

注意：dragram.json 中没有独立火焰模块，当前使用 GPIO39 作为火焰模拟输入占位。

## 4. 软件依赖
已在 platformio.ini 中配置：
- ArduinoJson
- PubSubClient
- DHT sensor library
- Adafruit Unified Sensor
- ESP32Servo
- NTPClient
- IRremote

## 5. 环境准备
1. 安装 VS Code 与 PlatformIO 插件。
2. 打开项目根目录。
3. 连接 ESP32 开发板（esp32dev）。

## 6. 关键配置
请根据实际环境修改 src/HomeService.cpp 中常量：

- Wi-Fi（路由器）
  - kStaSsid
  - kStaPassword
- AP 热点
  - kApSsid（默认 esp32-server）
  - kApPassword（默认 lbl450981）
- MQTT 云端
  - kMqttHost（默认 example.mqtt.server，需要改成实际地址）
  - kMqttPort（默认 1883）
  - kMqttClientId

## 7. 编译与烧录
在项目根目录执行：

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## 8. 启动行为说明
系统启动后流程：
1. 尝试连接家用 Wi-Fi。
2. 若可联网：进入云模式，启用 MQTT 与局域网 Web 服务。
3. 若不可联网：启用 AP 热点（esp32-server）并启动本地网页。
4. 每 30 秒重新检测网络并自动切换模式。

## 9. 传感器与数据广播
- 采样周期：0.5 秒
- MQTT 主题：
  - 传感器数据: esp32/home/sensors
  - 状态信息: esp32/home/status
  - 报警信息: esp32/home/alarm

传感器 JSON 字段示例：
- sensorType
- timestamp
- temperatureC
- humidityPercent
- lightPercent
- mq2Percent
- smokeLevel（green/blue/yellow/red）
- flameDetected
- error / errorMessage

## 10. 控制接口说明
### 10.1 MQTT 控制主题
- esp32/home/control

### 10.2 本地 Web API
- GET /api/status: 获取当前状态
- POST /api/control: 发送控制命令（JSON）

### 10.3 控制命令示例
风扇控制：

```json
{"device":"fan","mode":"high"}
```

或指定速度：

```json
{"device":"fan","speedPercent":80}
```

窗帘控制：

```json
{"device":"curtain","angle":120}
```

或预设档位（0-4）：

```json
{"device":"curtain","preset":2}
```

红外 NEC 发送：

```json
{"device":"ir","action":"send_nec","address":1,"command":2,"repeats":0}
```

## 11. 自动化与报警逻辑
- 定时窗帘：
  - 07:00 自动打开
  - 22:00 自动关闭
- 烟雾联动：
  - MQ2 达到高浓度自动风扇高速
  - 超高浓度触发短促蜂鸣
- 火焰联动：
  - 持续 45 秒触发“短短长”报警音
  - 持续 5 分钟且处于云模式时，发送火警报警消息

## 12. 本地网页控制台
连接设备 AP 或同局域网后，浏览器访问设备 IP（串口可查看），可实现：
- 传感器数据实时查看（0.5 秒刷新）
- 风扇模式控制（关闭/低/中/高）
- 窗帘滑块控制与预设控制

## 13. 常见问题
1. MQTT 无法连接
- 检查 kMqttHost、端口与网络可达性。

2. 读取 DHT 失败
- 检查传感器供电、数据脚、上拉电阻。

3. 本地网页打不开
- 确认已连接 ESP32 AP，或确认设备与电脑在同一局域网。

4. 红外功能异常
- 检查红外接收/发射引脚、电源与管脚定义是否一致。

## 14. 后续建议
- 将敏感配置（Wi-Fi、MQTT）提取到独立配置文件或 NVS。
- 增加设备状态持久化与断电恢复。
- 按需扩展云端协议与设备类型。
