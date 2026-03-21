# ESP32 Home Client

基于 ESP32 + PlatformIO + Arduino 的局域网客户端面板，用来对接 `esp32_home_server`。

## 当前功能定位

客户端当前按“本地控制面板”来组织，核心目标包括：

- 自动连接 `iPadmini` 或服务端热点 `esp32-server`
- 通过 HTTP 轮询服务端 `/api/status`
- 通过 HTTP 向服务端 `/api/control` 下发窗帘、风扇、红外命令
- 使用 RGB 灯显示烟雾安全等级
- 使用 4 个按键发送红外字符串命令
- 使用旋钮默认控制窗帘，按下切换到风扇模式
- 使用独立按钮控制风扇电源开关
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

在服务端热点场景下，客户端会直接访问：

- `http://192.168.4.1/api/status`
- `http://192.168.4.1/api/control`

## 输入映射

- `key1`：发送 `ac_power`
- `key2`：发送 `ac_temp_up`
- `key3`：发送 `ac_temp_down`
- `key4`：发送 `ac_mode`
- 旋钮旋转：默认调窗帘角度
- 旋钮按下：切换窗帘/风扇控制模式
- 独立风扇按钮：切换风扇电源开关

说明：

- 当前四个红外命令在 [ClientConfig.h](include/ClientConfig.h) 中是字符串命令，由服务端串口转发给 ESP8266。

## 显示策略

- LCD1602：当前实际启用的文本显示
- RGB：根据 `smokeLevel` 显示 `green / blue / yellow / red`
- 8pin TFT：后续会改用 LVGL，但现在界面没定，因此相关显示逻辑先屏蔽

## 接线配置

当前客户端引脚集中在 [ClientConfig.h](include/ClientConfig.h)：

- TFT SPI：`18/23/5/17/16/4`
- LCD1602 I2C：`21/22`
- RGB：`27/14/15`
- 红外快捷键：`32/33/25/26`
- 编码器：`34/35/13`
- 风扇电源按钮：见 [ClientConfig.h](include/ClientConfig.h)

这些值是可编译的默认方案；如果你的实物接线不同，直接改这个头文件即可。

## 编译

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```
