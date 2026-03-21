# ESP32 Home 项目根目录

当前已经演变为一个多子项目仓库，文档统一收敛到根目录的 [`doc/`](doc/) 管理。

## 子项目

- `esp32_home_server/`：主服务端，负责传感器采集、本地网页、HTTP 接口、自动化和 MQTT 上行。
- `esp32_home_client/`：局域网客户端，负责本地显示与控制输入。
- `esp8266_ir_control/`：红外桥接相关子项目。
- `esp32_ir_Study/`：红外实验与验证子项目。

## 文档入口

- [`doc/README.md`](doc/README.md)：文档总索引
- [`doc/server/功能要求.md`](doc/server/%E5%8A%9F%E8%83%BD%E8%A6%81%E6%B1%82.md)：服务端需求文档
- [`doc/client/需求说明.md`](doc/client/%E9%9C%80%E6%B1%82%E8%AF%B4%E6%98%8E.md)：客户端需求说明
- [`doc/shared/ESP32客户端与MQTT上位机对接文档.md`](doc/shared/ESP32%E5%AE%A2%E6%88%B7%E7%AB%AF%E4%B8%8EMQTT%E4%B8%8A%E4%BD%8D%E6%9C%BA%E5%AF%B9%E6%8E%A5%E6%96%87%E6%A1%A3.md)：共享对接文档
