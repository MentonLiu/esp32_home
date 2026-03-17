/**
 * @file Controllerr.cpp
 * @brief 执行器控制器实现文件
 * 
 * 实现所有执行器设备的控制逻辑：
 * - 风扇PWM调速
 * - 双舵机窗帘控制
 * - 蜂鸣器报警
 * - 红外桥接命令下发(ESP32 -> ESP8266)
 */

#include "Controllerr.h"

// ============ RelayFanController 实现 ============

RelayFanController::RelayFanController(uint8_t pin) : pin_(pin) {}

/**
 * @brief 初始化风扇控制器
 * 配置GPIO为输出模式，设置PWM参数(25kHz, 8位分辨率)
 */
void RelayFanController::begin()
{
    pinMode(pin_, OUTPUT);                  // 设置引脚为输出模式
    ledcSetup(pwmChannel_, 25000, 8);       // 配置PWM: 25kHz频率, 8位分辨率(0-255)
    ledcAttachPin(pin_, pwmChannel_);       // 将引脚绑定到PWM通道
    setMode(FanMode::Off);                  // 初始化为关闭状态
}

/**
 * @brief 设置风扇运行模式
 * 根据不同模式设置对应的转速百分比
 */
void RelayFanController::setMode(FanMode mode)
{
    mode_ = mode;
    switch (mode_)
    {
    case FanMode::Off:
        setSpeedPercent(0);   // 关闭风扇
        break;
    case FanMode::Low:
        setSpeedPercent(30);   // 低速: 30%
        break;
    case FanMode::Medium:
        setSpeedPercent(65);   // 中速: 65%
        break;
    case FanMode::High:
        setSpeedPercent(100);  // 高速: 100%
        break;
    }
}

/**
 * @brief 设置风扇转速百分比
 * 将0-100的百分比转换为PWM占空比(0-255)
 * @param speedPercent 目标转速百分比
 */
void RelayFanController::setSpeedPercent(uint8_t speedPercent)
{
    speedPercent_ = constrain(speedPercent, 0, 100);  // 限制在0-100范围内
    const uint8_t duty = map(speedPercent_, 0, 100, 0, 255);  // 映射到PWM范围
    ledcWrite(pwmChannel_, duty);  // 输出PWM信号

    // 转速为0时更新模式为关闭
    if (speedPercent_ == 0)
    {
        mode_ = FanMode::Off;
    }
}

FanMode RelayFanController::mode() const { return mode_; }

uint8_t RelayFanController::speedPercent() const { return speedPercent_; }


// ============ DualCurtainController 实现 ============

DualCurtainController::DualCurtainController(uint8_t pinA, uint8_t pinB) : pinA_(pinA), pinB_(pinB) {}

/**
 * @brief 初始化双窗帘舵机
 * 配置舵机PWM参数(50Hz)，设置初始角度为0(关闭)
 */
void DualCurtainController::begin()
{
    servoA_.setPeriodHertz(50);  // 舵机标准50Hz
    servoB_.setPeriodHertz(50);
    // 绑定引脚，设置脉宽范围500-2400us对应0-180度
    servoA_.attach(pinA_, 500, 2400);
    servoB_.attach(pinB_, 500, 2400);
    setAngle(0);  // 初始化为关闭状态
}

/**
 * @brief 设置窗帘角度
 * @details 两个舵机反向转动实现双侧窗帘同步开合
 * @param angle 目标角度 (0-180)
 */
void DualCurtainController::setAngle(uint8_t angle)
{
    currentAngle_ = constrain(angle, 0, 180);  // 限制角度范围
    servoA_.write(currentAngle_);               // 左侧舵机正转
    servoB_.write(180 - currentAngle_);        // 右侧舵机反转，实现对称开合
}

/**
 * @brief 设置预设开合等级
 * @param level 预设等级: 0=全关, 1=四分之一, 2=半开, 3=四分之三, 4=全开
 */
void DualCurtainController::setPresetLevel(uint8_t level)
{
    static const uint8_t presets[] = {0, 45, 90, 135, 180};  // 预设角度表
    level = constrain(level, 0, 4);  // 限制在0-4范围内
    setAngle(presets[level]);        // 设置对应角度
}

uint8_t DualCurtainController::angle() const { return currentAngle_; }


// ============ BuzzerController 实现 ============

BuzzerController::BuzzerController(uint8_t pin) : pin_(pin) {}

/**
 * @brief 初始化蜂鸣器
 * 配置PWM参数，初始状态静音
 */
void BuzzerController::begin()
{
    ledcSetup(pwmChannel_, 2000, 8);   // 配置PWM: 2kHz频率, 8位分辨率
    ledcAttachPin(pin_, pwmChannel_);  // 绑定引脚到PWM通道
    ledcWriteTone(pwmChannel_, 0);      // 初始静音
}

/**
 * @brief 发出指定频率和时长的蜂鸣声
 * @param frequency 声音频率 (Hz)
 * @param durationMs 持续时间 (毫秒)
 */
void BuzzerController::beep(uint16_t frequency, uint16_t durationMs)
{
    ledcWriteTone(pwmChannel_, frequency);  // 设置频率并开始发声
    delay(durationMs);                      // 持续指定时间
    ledcWriteTone(pwmChannel_, 0);          // 停止发声
}

/**
 * @brief 播放火灾警报模式: 短-短-长
 * @details 典型报警音: 两声短促提示后接一声长警报
 */
void BuzzerController::patternShortShortLong()
{
    beep(2400, 120);   // 第一声短促(120ms, 2.4kHz)
    delay(90);         // 间隔90ms
    beep(2400, 120);   // 第二声短促
    delay(90);         // 间隔90ms
    beep(1700, 450);   // 一声长警报(450ms, 1.7kHz)
}


// ============ IRController 实现 ============

IRController::IRController(uint8_t rxPin, uint8_t txPin, uint32_t baudRate)
    : rxPin_(rxPin), txPin_(txPin), baudRate_(baudRate) {}

/**
 * @brief 绑定桥接串口
 * @details
 * 串口对象由HomeService统一初始化(包括波特率与引脚)，本类仅保存指针并通过该串口发送命令。
 */
void IRController::begin(Stream &serial)
{
    serial_ = &serial;
}

/**
 * @brief 发送NEC协议红外命令
 * @details 发送给ESP8266，由ESP8266执行实际红外发射
 */
bool IRController::sendNEC(uint16_t address, uint8_t command, uint8_t repeats)
{
    return sendProtocolCommand("NEC", address, command, repeats);
}

/**
 * @brief 发送通用协议命令
 * @details 命令格式统一为JSON，便于ESP8266端扩展多协议
 */
bool IRController::sendProtocolCommand(const String &protocol, uint32_t address, uint32_t command, uint8_t repeats)
{
    String payload = String("{\"device\":\"ir\",\"action\":\"send\",\"protocol\":\"") + protocol +
                     "\",\"address\":" + String(address) +
                     ",\"command\":" + String(command) +
                     ",\"repeats\":" + String(repeats) + "}";
    return sendJsonCommand(payload);
}

/**
 * @brief 发送扩展动作命令
 * @details args默认是空JSON对象，便于后续扩展如学习模式、停止学习、原始码发送
 */
bool IRController::sendActionCommand(const String &action, const String &args)
{
    String payload = String("{\"device\":\"ir\",\"action\":\"") + action + "\",\"args\":" + args + "}";
    return sendJsonCommand(payload);
}

/**
 * @brief 直接发送完整JSON命令
 * @details 该接口为上层保留最大扩展性，便于快速支持新字段
 */
bool IRController::sendJsonCommand(const String &jsonCommand)
{
    return sendLine(jsonCommand);
}

/**
 * @brief 读取ESP8266返回的一行消息
 * @details 约定ESP8266每条状态消息以换行符结尾
 */
IRDecodedSignal IRController::receive()
{
    IRDecodedSignal result;
    if (serial_ == nullptr || !serial_->available())
    {
        return result;
    }

    result.message = serial_->readStringUntil('\n');
    result.message.trim();
    result.available = result.message.length() > 0;
    return result;
}

String IRController::lastCommand() const { return lastCommand_; }

uint32_t IRController::baudRate() const { return baudRate_; }

bool IRController::sendLine(const String &line)
{
    if (serial_ == nullptr)
    {
        return false;
    }

    // 串口桥接协议采用一行一条命令，方便ESP8266按行解析。
    serial_->println(line);
    lastCommand_ = line;
    return true;
}
