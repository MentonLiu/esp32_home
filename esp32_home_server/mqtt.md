# ESP32 + EMQX 上位机接收与控制教程

本文适用于你当前的固件配置：ESP32 已经能连上 EMQX，现在要在上位机完成“收遥测 + 发控制”的完整闭环。

## 1. 先确认通信约定

你的设备侧 MQTT 约定如下：

- Broker 地址：frp-era.com
- Broker 端口：10883
- 用户名：esp32_server
- 密码：lbl450981
- 设备客户端 ID：esp32-home-server

主题约定：

- 上行遥测：esp32/home/sensors
- 上行状态：esp32/home/status
- 上行告警预留：esp32/home/alarm
- 下行控制：esp32/home/control

---

## 2. 在 EMQX 创建上位机客户端

建议在 EMQX Dashboard 或 MQTTX 中创建两个客户端，避免收发混用。

### 2.1 监控客户端（只订阅）

用途：显示传感器数据、状态反馈、自动化事件

连接参数：

- Host：frp-era.com
- Port：10883
- Username：esp32_server
- Password：lbl450981
- Client ID：pc-monitor-01（自定义且唯一）

订阅主题：

- esp32/home/sensors
- esp32/home/status
- esp32/home/alarm（可先订阅，后续扩展可用）

### 2.2 控制客户端（只发布）

用途：手动控制风扇、窗帘

连接参数：

- Host：frp-era.com
- Port：10883
- Username：esp32_server
- Password：lbl450981
- Client ID：pc-control-01（自定义且唯一）

发布主题：

- esp32/home/control

---

## 3. 控制消息格式（上位机发布）

发布到主题：esp32/home/control  
消息体为 JSON。

### 3.1 风扇控制示例

关风扇

    {"device":"fan","mode":"off"}

低速

    {"device":"fan","mode":"low"}

中速

    {"device":"fan","mode":"medium"}

高速

    {"device":"fan","mode":"high"}

按百分比调速

    {"device":"fan","speedPercent":80}

### 3.2 窗帘控制示例

按角度

    {"device":"curtain","angle":135}

按预设档位（0 到 4）

    {"device":"curtain","preset":3}

---

## 4. 上位机接收消息说明

## 4.1 传感器主题 esp32/home/sensors

典型字段：

- sensorType
- timestamp
- temperatureC
- humidityPercent
- lightPercent
- mq2Percent
- rainPercent
- isRaining
- smokeLevel
- error
- errorMessage

示例：

    {"sensorType":"home_snapshot","timestamp":1234567,"temperatureC":26.3,"humidityPercent":58,"lightPercent":72,"mq2Percent":18,"rainPercent":12,"isRaining":false,"smokeLevel":"green","error":false,"errorMessage":""}

## 4.2 状态主题 esp32/home/status

格式固定为：

- type：事件类型
- message：结果或事件描述

示例：

控制成功

    {"type":"cloud_command","message":"fan_updated"}

控制失败

    {"type":"error","message":"fan_mode_invalid"}

自动化事件

    {"type":"automation","message":"curtain_close_by_rain"}

---

## 5. 联调验证流程（建议照做）

1. 监控客户端订阅 esp32/home/sensors 和 esp32/home/status。  
2. 控制客户端向 esp32/home/control 发布：

       {"device":"fan","mode":"high"}

3. 观察状态主题返回：

       {"type":"cloud_command","message":"fan_updated"}

4. 再发送一个错误命令：

       {"device":"fan","mode":"max"}

5. 应返回错误状态：

       {"type":"error","message":"fan_mode_invalid"}

如果以上都正常，说明你的上位机接收和控制链路已经打通。

---

## 6. 上位机落地建议

- 用 sensors 做实时图表（温湿度、MQ2、光照、雨量）
- 用 status 做事件日志（控制结果、自动化动作、异常）
- 控制界面只操作 control 主题，避免误发到其他主题
- Client ID 保证唯一，避免被同名连接踢下线

---

## 7. 常见问题排查

1. 能连接但收不到数据  
   检查订阅主题是否写成了 esp32/home/sensor（少了 s）。

2. 能收数据但控制无效  
   检查是否发布到 esp32/home/control，且 JSON 字段名是 device、mode、speedPercent、angle、preset。

3. 偶发离线  
   检查是否有重复 Client ID；EMQX 同 ID 新连接会顶掉旧连接。

4. 认证失败  
   核对用户名密码是否与设备端一致：esp32_server / lbl450981。

---

如果你需要，我可以继续给你一版“可直接导入 MQTTX 的连接与订阅清单”，以及“Node-RED 上位机面板快速搭建版”。

---

## 8. 已实现：NAS 网页控制台

本仓库已新增可直接运行的页面：

- 页面文件：`nas-console/index.html`
- 使用说明：`nas-console/README.md`

你可以在 NAS 局域网电脑上部署静态网页后直接使用该页面完成：

- 连接 EMQX WebSocket
- 订阅 `esp32/home/sensors`、`esp32/home/status`、`esp32/home/alarm`
- 发布 `esp32/home/control` 控制命令
- 查看实时数据与状态日志

快速启动（本机测试）：

```bash
cd nas-console
python3 -m http.server 8090
```

浏览器访问：

`http://192.168.31.204:8090/index.html`
