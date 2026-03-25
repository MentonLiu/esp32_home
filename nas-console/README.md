# NAS 局域网 MQTT 控制台

这个目录提供一个独立网页控制台，用于在 NAS 局域网电脑上直接连接 EMQX，并控制 ESP32。

## 功能

- 通过 WebSocket 连接 EMQX
- 订阅：`esp32/home/sensors`、`esp32/home/status`、`esp32/home/alarm`
- 发布：`esp32/home/control`
- 支持风扇和窗帘控制、实时状态展示、消息日志

## 关键配置

页面默认值：

- WebSocket URL：`ws://192.168.31.204:8083/mqtt`
- 用户名：`test_link`
- 密码：`lbl450981`
- 主题：
  - sensors: `esp32/home/sensors`
  - status: `esp32/home/status`
  - alarm: `esp32/home/alarm`
  - control: `esp32/home/control`

如果你的 EMQX 只开了 TLS WebSocket，请改为 `wss://...`。

## 4. 使用步骤

1. 点击“连接”。
2. 观察日志出现“连接成功”和订阅成功。
3. 点击风扇/窗帘按钮，或发送自定义 JSON。
4. 在“最近状态消息”与“消息日志”查看回执。

## 5. 注意事项

- 浏览器直连方案会在前端保留 MQTT 凭据，建议使用低权限专用账号。
- 不要使用与 ESP32 相同的 Client ID。
- 如果连接失败，优先检查 EMQX WebSocket 监听端口与防火墙策略。
