# ESP32 Home Client

基于 ESP32 + PlatformIO + Arduino 的局域网客户端面板，用来对接 `esp32_home_server`。

## 当前定位

当前客户端的职责是作为服务端的本地控制面板与状态显示终端，通过 HTTP 与 `esp32_home_server` 联动。

联调时请优先以服务端当前实现为准：

- 权威来源：`../esp32_home_server` 代码、`README.md`、`changes.md`
- 当前主链路：本地 HTTP
- MQTT：保留为服务端扩展能力，但不是当前客户端对接前提

更完整的字段与协议说明见：

- `../doc/client/服务端对接.md`

## 当前能力

客户端当前按“本地控制面板”来组织，核心目标包括：

- 自动连接 `iPadmini` 或服务端热点 `esp32-server`
- 通过 HTTP 轮询服务端 `/api/status`
- 通过 HTTP 向服务端 `/api/control` 下发窗帘、风扇命令
- 使用 RGB 灯显示烟雾安全等级
- 使用旋钮默认控制窗帘，按下切换到风扇模式
- 保留独立风扇按钮输入，但其协议语义仍待迁移
- LCD1602 作为当前有效显示器
- 8pin TFT 的 LVGL 界面先屏蔽，等待后续设计

## 当前架构

- `HomeClientApp`
  顶层调度器，串联联网、状态拉取、输入处理和显示输出。
- `ClientWiFiManager`
  负责连接家庭 Wi-Fi 和服务端热点。
- `ServerApiClient`
  负责发现服务端、拉取状态、发送控制命令。
- `InputManager`
  负责按键消抖、编码器解码、风扇电源按钮和模式切换事件。
- `OutputManager`
  负责 RGB 指示灯、LCD1602 文本状态，以及被屏蔽的 TFT 占位逻辑。

## 连接方式

客户端优先连接：

1. `iPadmini`
2. `esp32-server`

服务端访问模式分为：

- `cloud`
  服务端连上家庭 Wi-Fi 后，通过局域网 IP 或 `http://esp32-home-server.local/` 访问
- `local_ap`
  服务端未连上家庭 Wi-Fi 时，客户端直连热点并访问 `http://192.168.4.1`

当前客户端发现逻辑：

- 连接到 `esp32-server` 时，直接探测 `http://192.168.4.1`
- 连接到家庭 Wi-Fi 时，优先通过 mDNS 查询 `esp32-home-server.local`

## 服务端接口口径

当前客户端对接的服务端主接口为：

- `GET /api/status`
- `POST /api/control`

服务端当前真实协议口径：

- 状态接口返回顶层字段加 `sensor`、`controller` 分组字段
- 风扇状态以 `fanMode` 与 `fanSpeedPercent` 表达
- 控制接口支持：
  - `{"device":"fan","mode":"off|low|medium|high"}`
  - `{"device":"fan","speedPercent":80}`
  - `{"device":"curtain","angle":135}`
  - `{"device":"curtain","preset":3}`

注意：

- 当前服务端已不再返回 `fanPowerOn`
- 当前服务端也不再支持 `{"device":"fan","power":"on|off"}`

## 输入映射

- 旋钮旋转：默认调窗帘角度
- 旋钮按下：切换窗帘/风扇控制模式
- 独立风扇按钮：当前代码仍按“风扇电源切换”理解，但该语义与最新服务端协议不一致

## 显示策略

- LCD1602：当前实际启用的文本显示
- RGB：根据 `smokeLevel` 显示 `green / blue / yellow / red`
- 8pin TFT：后续会改用 LVGL，但现在界面没定，因此相关显示逻辑先屏蔽

## 当前兼容性状态

按最新服务端实现看，当前客户端的联调状态如下：

- 窗帘控制可对接
- 风扇速度控制可对接
- 状态轮询主链路可对接
- 风扇独立电源按钮语义与最新服务端不一致，需后续客户端代码迁移

也就是说，当前客户端与最新服务端是“部分联通”，但不是完全协议一致。

## 接线配置

当前客户端引脚集中在 [ClientConfig.h](include/ClientConfig.h)：

- TFT SPI：`18/23/5/17/16/4`
- LCD1602 I2C：`21/22`
- RGB：`27/14/15`
- 编码器：`34/35/13`
- 风扇电源按钮：见 [ClientConfig.h](include/ClientConfig.h)

这些值是可编译的默认方案；如果你的实物接线不同，直接改这个头文件即可。

## 编译

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```
