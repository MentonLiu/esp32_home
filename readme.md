# ESP32 Home 项目总览

这是一个围绕“家庭环境感知 + 本地控制 + 局域网联调 + 红外桥接”组织的多子项目仓库。

当前主链路已经收敛为：

`浏览器 / ESP32 客户端 -> esp32_home_server -> 风扇 / 窗帘 / 蜂鸣器 / ESP8266 红外桥`

## 项目图示

### 服务端架构图

![服务端架构图](doc/server/%E9%A1%B9%E7%9B%AE%E6%9E%B6%E6%9E%84.png)

### 服务端功能流程图

![服务端功能流程图](doc/server/%E5%8A%9F%E8%83%BD%E6%B5%81%E7%A8%8B.png)

### 客户端架构图

![客户端架构图](doc/client/%E9%A1%B9%E7%9B%AE%E6%9E%B6%E6%9E%84.png)

## 仓库一句话定位

- `esp32_home_server/`：主控工程，当前最完整、最优先理解。
- `esp32_home_client/`：局域网控制面板，走 HTTP 调服务端。
- `esp8266_ir_control/`：红外执行端占位工程，目录已建，逻辑未完工。
- `doc/`：仓库统一文档入口。

## 基于 Git 的近期演进（2026-03-15 ~ 2026-03-23）

根据提交历史，项目演进可概括为 5 个阶段：

1. 启动与需求沉淀
  完成基础结构、功能流程和需求文档。
2. 服务端首版与红外桥接方向确定
  服务端首版落地，红外路径改为“ESP32 串口下发字符串给 ESP8266”。
3. 核心架构重构
  包括 ArduinoJson v7 迁移、模块拆分、ESP32-S3 迁移、Web 控制台改为 LittleFS。
4. 文档和仓库结构统一
  补充中文注释、结构图、接线说明，清理占位目录与缓存。
5. 现场可用性优化
  自动化规则增强，风扇调速模型收敛，本地控制响应与执行器稳定性优化。

详细里程碑见：[`doc/版本演进与里程碑.md`](doc/%E7%89%88%E6%9C%AC%E6%BC%94%E8%BF%9B%E4%B8%8E%E9%87%8C%E7%A8%8B%E7%A2%91.md)

## 当前固定约定

- 家庭 Wi-Fi：`iPadmini / lbl450981`
- 服务端离线热点：`esp32-server / lbl450981`
- 服务端主接口：`GET /api/status`、`POST /api/control`
- 红外链路：服务端通过 UART 向 ESP8266 发送字符串命令
- MQTT：当前保留为预留能力，不是主链路
- 8pin TFT / LVGL：保留为后续方向，不作为当前交付前提

## 文档导航（建议按顺序）

1. [`project-map.html`](project-map.html)
  快速建立仓库结构全景。
2. [`doc/快速上手.md`](doc/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.md)
  新人最短上手路径。
3. [`doc/shared/系统总览与联调指南.md`](doc/shared/%E7%B3%BB%E7%BB%9F%E6%80%BB%E8%A7%88%E4%B8%8E%E8%81%94%E8%B0%83%E6%8C%87%E5%8D%97.md)
  统一理解设备角色和运行链路。
4. [`esp32_home_server/README.md`](esp32_home_server/README.md)
  深入当前主控实现。
5. [`doc/shared/项目接线说明.md`](doc/shared/%E9%A1%B9%E7%9B%AE%E6%8E%A5%E7%BA%BF%E8%AF%B4%E6%98%8E.md)
  确认供电、共地与驱动接线。
6. [`doc/shared/ESP32客户端与MQTT上位机对接文档.md`](doc/shared/ESP32%E5%AE%A2%E6%88%B7%E7%AB%AF%E4%B8%8EMQTT%E4%B8%8A%E4%BD%8D%E6%9C%BA%E5%AF%B9%E6%8E%A5%E6%96%87%E6%A1%A3.md)
  查看当前 HTTP 协议与 MQTT 预留位说明。

## 10 分钟上手路线

1. 只看 `esp32_home_server`，先跑通服务端网页和 HTTP。
2. 验证风扇、窗帘、传感器链路稳定性。
3. 再接入 `esp32_home_client` 作为第二控制入口。
4. 最后推进 `esp8266_ir_control` 的真实红外执行逻辑。

## 当前易误解点

- 文档中能看到 MQTT，不代表当前依赖 MQTT 才能运行。
- `esp8266_ir_control/` 已存在，不代表已经可直接烧录联调。
- 个别历史文档描述与当前实现存在时间差，冲突时以服务端现状和 `doc/shared/` 为准。

## 相关入口

- 仓库级变更记录：[`changes.md`](changes.md)
- 文档总入口：[`doc/README.md`](doc/README.md)
