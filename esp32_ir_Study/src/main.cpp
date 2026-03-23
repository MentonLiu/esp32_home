#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <vector>

#define DECODE_DISTANCE_WIDTH
#define DECODE_HASH
#define RAW_BUFFER_LENGTH 750
#define RECORD_GAP_MICROS 16000
#define USE_16_BIT_TIMING_BUFFER
#include <IRremote.hpp>

namespace {
constexpr uint8_t IR_RECEIVE_PIN = 14;
constexpr unsigned long SERIAL_BAUD = 115200;
constexpr char IR_CODES_FILE[] = "/ir_codes.txt";

std::vector<String> savedKeys;
bool spiffsReady = false;
uint32_t captureIndex = 0;

String formatHex8(uint8_t value) {
  char buffer[5];
  snprintf(buffer, sizeof(buffer), "0x%02X", value);
  return String(buffer);
}

String formatHex32(uint32_t value) {
  char buffer[11];
  snprintf(buffer, sizeof(buffer), "0x%08lX", static_cast<unsigned long>(value));
  return String(buffer);
}

String formatHex64(uint64_t value) {
  char buffer[19];
  snprintf(buffer, sizeof(buffer), "0x%016llX",
           static_cast<unsigned long long>(value));
  return String(buffer);
}

String protocolName() {
  const char *name = IrReceiver.getProtocolString();
  return name == nullptr ? String("UNKNOWN") : String(name);
}

uint32_t computeRawHash() {
  uint32_t hash = 2166136261UL;
  for (IRRawlenType i = 1; i < IrReceiver.decodedIRData.rawlen; ++i) {
    const uint32_t microsValue = IrReceiver.irparams.rawbuf[i] * MICROS_PER_TICK;
    hash ^= microsValue + (i << 16);
    hash *= 16777619UL;
  }
  hash ^= IrReceiver.decodedIRData.rawlen;
  return hash;
}

bool isAlreadySaved(const String &key) {
  for (const String &savedKey : savedKeys) {
    if (savedKey == key) {
      return true;
    }
  }
  return false;
}

void loadSavedKeys() {
  File file = SPIFFS.open(IR_CODES_FILE, FILE_READ);
  if (!file) {
    Serial.println("未发现历史红外码文件，将在首次学习时自动创建。");
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (!line.startsWith("key=")) {
      continue;
    }

    const String key = line.substring(4);
    if (!key.isEmpty()) {
      savedKeys.push_back(key);
    }
  }

  file.close();
  Serial.printf("已加载 %u 条历史学习记录。\n",
                static_cast<unsigned>(savedKeys.size()));
}

String buildStorageKey() {
  if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
    return "UNKNOWN-" + formatHex32(computeRawHash()) + "-" +
           String(IrReceiver.decodedIRData.rawlen);
  }

  return protocolName() + "-" +
         formatHex64(IrReceiver.decodedIRData.decodedRawData) + "-" +
         String(IrReceiver.decodedIRData.numberOfBits);
}

void appendCaptureToFile(const String &key) {
  if (!spiffsReady || isAlreadySaved(key)) {
    return;
  }

  File file = SPIFFS.open(IR_CODES_FILE, FILE_APPEND);
  if (!file) {
    Serial.println("打开红外码文件失败，未能保存到本地。");
    return;
  }

  ++captureIndex;
  file.printf("key=%s\n", key.c_str());
  file.printf("capture=%lu\n", static_cast<unsigned long>(captureIndex));
  file.printf("protocol=%s\n", protocolName().c_str());
  file.printf("flags=0x%02X\n", IrReceiver.decodedIRData.flags);
  file.printf("rawlen=%u\n", static_cast<unsigned>(IrReceiver.decodedIRData.rawlen));
  file.printf("bits=%u\n", IrReceiver.decodedIRData.numberOfBits);

  if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
    file.printf("address=%s\n", formatHex32(IrReceiver.decodedIRData.address).c_str());
    file.printf("command=%s\n", formatHex32(IrReceiver.decodedIRData.command).c_str());
    file.printf("raw_data=%s\n", formatHex64(IrReceiver.decodedIRData.decodedRawData).c_str());
  } else {
    file.printf("hash=%s\n", formatHex32(computeRawHash()).c_str());
  }

  file.print("raw_us={");
  for (IRRawlenType i = 1; i < IrReceiver.decodedIRData.rawlen; ++i) {
    if (i > 1) {
      file.print(',');
    }
    file.print(IrReceiver.irparams.rawbuf[i] * MICROS_PER_TICK);
  }
  file.println("}");
  file.println("---");
  file.close();

  savedKeys.push_back(key);
  Serial.printf("已保存到 %s\n", IR_CODES_FILE);
}

void printUsageTip() {
  Serial.println();
  Serial.println("=== ESP32 空调红外学习器 ===");
  Serial.printf("红外接收引脚: GPIO%u\n", IR_RECEIVE_PIN);
  Serial.println("支持普通遥控器解码，也支持空调长码原始抓取。");
  Serial.println("新学到的信号会保存到 SPIFFS 的 /ir_codes.txt。");
  Serial.printf("RAW_BUFFER_LENGTH=%d, RECORD_GAP_MICROS=%d\n",
                RAW_BUFFER_LENGTH, RECORD_GAP_MICROS);
  Serial.println();
}

void printCaptureSummary() {
  Serial.println();
  IrReceiver.printIRResultShort(&Serial, false);

  if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
    Serial.println("接收缓冲区溢出，当前信号太长，需继续增大 RAW_BUFFER_LENGTH。");
    return;
  }

  if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
    Serial.println("检测到未知或未启用协议，已按原始脉冲学习。");
    Serial.printf("Hash: %s\n", formatHex32(computeRawHash()).c_str());
  } else {
    Serial.printf("协议: %s, 地址: %s, 命令: %s, 原始值: %s\n",
                  protocolName().c_str(),
                  formatHex32(IrReceiver.decodedIRData.address).c_str(),
                  formatHex32(IrReceiver.decodedIRData.command).c_str(),
                  formatHex64(IrReceiver.decodedIRData.decodedRawData).c_str());
  }

  Serial.println("Raw result in microseconds:");
  IrReceiver.printIRResultRawFormatted(&Serial, true);
}
}  // namespace

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);

  printUsageTip();

  spiffsReady = SPIFFS.begin(true);
  if (!spiffsReady) {
    Serial.println("SPIFFS 挂载失败，无法保存本地红外码。");
  } else {
    loadSavedKeys();
  }

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  Serial.print("当前已启用协议: ");
  printActiveIRProtocols(&Serial);
  Serial.println();
  Serial.println("红外接收器已启动，等待按键...");
}

void loop() {
  if (!IrReceiver.decode()) {
    return;
  }

  if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
    Serial.println("[重复帧] 已忽略。");
    IrReceiver.resume();
    return;
  }

  printCaptureSummary();
  appendCaptureToFile(buildStorageKey());
  Serial.println();
  Serial.println("等待下一次学习...");

  IrReceiver.resume();
}
