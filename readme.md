# ESP32 Home 项目根目录

这是一个多子项目仓库，当前围绕“服务端主控 + 局域网客户端面板 + 根目录统一文档”来维护。

## 当前开发约定

- 家庭 Wi-Fi 已固定为：
  - SSID：`iPadmini`
  - Password：`lbl450981`
- 服务端离线兜底热点仍为：
  - SSID：`esp32-server`
  - Password：`lbl450981`
- 客户端当前主链路只走 HTTP
- MQTT 先保留占位接口，不作为当前交付主功能
- 8pin TFT 未来计划改为 LVGL 界面，但目前界面未设计，因此相关显示逻辑先屏蔽
- 红外链路固定为“服务端串口发字符串 -> ESP8266 解释并执行”

## 仓库结构

- `doc/`
  统一文档中心，需求、协议、图示都优先放这里。
- `esp32_home_server/`
  主服务端，负责传感器采样、自动化、网页控制、风扇/窗帘执行和串口红外桥接。
- `esp32_home_client/`
  局域网客户端，负责本地按钮、旋钮、RGB 状态灯、LCD1602 和 HTTP 控制。
- `esp8266_ir_control/`
  红外桥接子项目，由它实际接收字符串命令并驱动红外。
- `esp32_ir_Study/`
  红外实验项目，当前不参与主链路。

## 阅读顺序

1. [`project-map.html`](project-map.html)
   仓库整体结构说明页。
2. [`doc/README.md`](doc/README.md)
   文档索引。
3. [`doc/shared/ESP32客户端与MQTT上位机对接文档.md`](doc/shared/ESP32%E5%AE%A2%E6%88%B7%E7%AB%AF%E4%B8%8EMQTT%E4%B8%8A%E4%BD%8D%E6%9C%BA%E5%AF%B9%E6%8E%A5%E6%96%87%E6%A1%A3.md)
   当前双端对接协议。
4. [`esp32_home_server/README.md`](esp32_home_server/README.md)
   服务端说明。
5. [`esp32_home_client/README.md`](esp32_home_client/README.md)
   客户端说明。

## 当前推荐联调方式

1. 先烧录服务端，确认 `/api/status` 和 `/api/control` 可用。
2. 再烧录客户端，让它接入 `iPadmini` 或 `esp32-server`。
3. 用客户端按钮与旋钮验证风扇开关、风扇调速、窗帘角度与红外字符串命令。
4. MQTT 功能目前不作为联调阻塞项。
