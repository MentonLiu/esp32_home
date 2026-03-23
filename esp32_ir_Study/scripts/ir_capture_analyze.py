#!/usr/bin/env python3
import argparse
import json
import re
import sys
import time
from dataclasses import dataclass, asdict
from pathlib import Path

import serial
from serial.tools import list_ports


BLOCK_END = "等待下一次学习..."
RAW_TIMINGS_PREFIX = "rawIRTimings["
HASH_PREFIX = "Hash:"
DURATION_PREFIX = "Duration="


@dataclass
class Capture:
    protocol: str = ""
    summary_hash: str = ""
    analyzer_hash: str = ""
    bits: int = 0
    duration_us: int = 0
    raw_count: int = 0
    timings_us: list[int] | None = None
    lines: list[str] | None = None


def parse_protocol_bits(line: str) -> tuple[str, int]:
    match = re.search(r"Protocol=([A-Z_]+).*?(\d+)\s+bits", line)
    if not match:
        return "", 0
    return match.group(1), int(match.group(2))


def parse_duration(line: str) -> int:
    match = re.search(r"Duration=(\d+)us", line)
    return int(match.group(1)) if match else 0


def parse_hash(line: str) -> str:
    match = re.search(r"0x[0-9A-Fa-f]+", line)
    return match.group(0).upper() if match else ""


def parse_timings(lines: list[str]) -> list[int]:
    values: list[int] = []
    for line in lines:
        if line.startswith(" +") or line.startswith(" -") or line.startswith("+") or line.startswith("-"):
            for token in re.findall(r"[+-]\s*\d+", line):
                number = int(token.replace(" ", ""))
                values.append(number)
    return values


def parse_block(lines: list[str]) -> Capture | None:
    if not lines:
      return None

    capture = Capture(lines=list(lines))
    raw_section = False
    raw_lines: list[str] = []

    for line in lines:
        stripped = line.strip()
        if stripped.startswith("Protocol="):
            capture.protocol, capture.bits = parse_protocol_bits(stripped)
            capture.summary_hash = parse_hash(stripped)
        elif stripped.startswith(HASH_PREFIX):
            capture.analyzer_hash = parse_hash(stripped)
        elif stripped.startswith(RAW_TIMINGS_PREFIX):
            raw_section = True
            match = re.search(r"\[(\d+)\]", stripped)
            if match:
                capture.raw_count = int(match.group(1))
        elif raw_section and stripped.startswith(DURATION_PREFIX):
            capture.duration_us = parse_duration(stripped)
            raw_section = False
        elif raw_section:
            raw_lines.append(stripped)

    capture.timings_us = parse_timings(raw_lines)
    if not capture.protocol and not capture.timings_us:
        return None
    return capture


def parse_stream(lines: list[str]) -> list[Capture]:
    captures: list[Capture] = []
    block: list[str] = []

    for line in lines:
        text = line.rstrip("\n")
        if not text:
            continue
        block.append(text)
        if BLOCK_END in text:
            capture = parse_block(block)
            if capture is not None:
                captures.append(capture)
            block = []

    if block:
        capture = parse_block(block)
        if capture is not None:
            captures.append(capture)

    return captures


def summarize(captures: list[Capture]) -> dict:
    summary: dict[str, object] = {
        "capture_count": len(captures),
        "by_protocol": {},
        "durations_us": [capture.duration_us for capture in captures if capture.duration_us],
    }

    by_protocol: dict[str, int] = {}
    by_signature: dict[str, int] = {}
    for capture in captures:
        protocol = capture.protocol or "UNKNOWN"
        by_protocol[protocol] = by_protocol.get(protocol, 0) + 1
        signature = f"{protocol}|{capture.bits}|{capture.duration_us}|{capture.analyzer_hash or capture.summary_hash}"
        by_signature[signature] = by_signature.get(signature, 0) + 1

    summary["by_protocol"] = by_protocol
    summary["unique_signatures"] = by_signature
    return summary


def write_outputs(captures: list[Capture], output_prefix: Path) -> None:
    output_prefix.parent.mkdir(parents=True, exist_ok=True)

    json_path = output_prefix.with_suffix(".json")
    txt_path = output_prefix.with_suffix(".txt")

    payload = {
        "summary": summarize(captures),
        "captures": [asdict(capture) for capture in captures],
    }
    json_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")

    lines: list[str] = []
    lines.append(f"captures={len(captures)}")
    lines.append("")
    for index, capture in enumerate(captures, start=1):
        lines.append(f"[capture {index}]")
        lines.append(f"protocol={capture.protocol}")
        lines.append(f"bits={capture.bits}")
        lines.append(f"duration_us={capture.duration_us}")
        lines.append(f"summary_hash={capture.summary_hash}")
        lines.append(f"analyzer_hash={capture.analyzer_hash}")
        lines.append(f"timing_count={len(capture.timings_us or [])}")
        if capture.timings_us:
            lines.append("timings_us=" + ",".join(str(v) for v in capture.timings_us))
        lines.append("")

    txt_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"分析结果已写入: {json_path}")
    print(f"文本摘要已写入: {txt_path}")


def serial_capture(port: str, baudrate: int, seconds: int) -> list[str]:
    lines: list[str] = []
    deadline = time.time() + seconds if seconds > 0 else None

    with serial.Serial(port, baudrate=baudrate, timeout=0.2) as ser:
        print(f"正在监听串口 {port} @ {baudrate} ...")
        while True:
            if deadline is not None and time.time() >= deadline:
                break
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
            print(line)
            lines.append(line)
    return lines


def list_serial_ports() -> None:
    for port in list_ports.comports():
        print(f"{port.device}  {port.description}")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="抓取并分析 ESP32 红外学习器日志")
    parser.add_argument("--port", help="串口设备，例如 /dev/cu.usbserial-110")
    parser.add_argument("--baud", type=int, default=115200, help="串口波特率，默认 115200")
    parser.add_argument("--seconds", type=int, default=20, help="监听时长，默认 20 秒；0 表示持续监听")
    parser.add_argument("--input", type=Path, help="离线日志文件路径")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("captures/ir_capture_analysis"),
        help="输出前缀，默认 captures/ir_capture_analysis",
    )
    parser.add_argument("--list-ports", action="store_true", help="列出本机串口")
    return parser


def main() -> int:
    parser = build_arg_parser()
    args = parser.parse_args()

    if args.list_ports:
        list_serial_ports()
        return 0

    if args.input is None and not args.port:
        parser.error("请提供 --input 或 --port")

    if args.input is not None:
        lines = args.input.read_text(encoding="utf-8").splitlines()
    else:
        lines = serial_capture(args.port, args.baud, args.seconds)

    captures = parse_stream(lines)
    if not captures:
        print("没有解析到完整学习记录。")
        return 1

    print(f"共解析到 {len(captures)} 条学习记录。")
    print(json.dumps(summarize(captures), indent=2, ensure_ascii=False))
    write_outputs(captures, args.output)
    return 0


if __name__ == "__main__":
    sys.exit(main())

