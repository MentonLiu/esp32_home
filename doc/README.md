# 项目文档中心

本目录是整个项目的统一文档入口，后续新增文档也建议直接放在这里，不再分散维护在各子项目内部的 `doc/` 目录。

## 文档结构

- `server/`
  服务端相关文档与图示。
- `client/`
  客户端需求和交互说明。
- `shared/`
  跨子项目共享的协议、对接和联调文档。

## 当前文档清单

- [`server/功能要求.md`](server/%E5%8A%9F%E8%83%BD%E8%A6%81%E6%B1%82.md)
- [`server/功能流程.dio`](server/%E5%8A%9F%E8%83%BD%E6%B5%81%E7%A8%8B.dio)
- [`server/功能流程.png`](server/%E5%8A%9F%E8%83%BD%E6%B5%81%E7%A8%8B.png)
- [`server/项目架构.dio`](server/%E9%A1%B9%E7%9B%AE%E6%9E%B6%E6%9E%84.dio)
- [`server/项目架构.png`](server/%E9%A1%B9%E7%9B%AE%E6%9E%B6%E6%9E%84.png)
- [`client/需求说明.md`](client/%E9%9C%80%E6%B1%82%E8%AF%B4%E6%98%8E.md)
- [`shared/ESP32客户端与MQTT上位机对接文档.md`](shared/ESP32%E5%AE%A2%E6%88%B7%E7%AB%AF%E4%B8%8EMQTT%E4%B8%8A%E4%BD%8D%E6%9C%BA%E5%AF%B9%E6%8E%A5%E6%96%87%E6%A1%A3.md)

## 维护约定

1. 文档统一优先放在根目录 `doc/`。
2. 子项目目录下尽量不再维护独立文档副本，避免一份内容多处漂移。
3. 协议类文档放在 `shared/`，需求和结构图放在对应子项目目录下。
