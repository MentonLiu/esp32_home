#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import sys
import zipfile

TARGET_DIRS = [
    "esp32_home_client",
    "esp32_home_server",
    "nas-console",
]

EXCLUDED_DIR_NAMES = {
    ".git",
    ".svn",
    ".hg",
    ".pio",
    ".pio-home",
    ".vscode",
    ".idea",
    "node_modules",
    "__pycache__",
}

EXCLUDED_FILE_NAMES = {
    ".DS_Store",
}
# v1.1.0
# 将emqx服务器从nas迁移到云服务器，并更新了相关文档和配置文件。
# 修复了若干错误
# 优化了代码注释

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Pack project folders into versioned zip files."
    )
    parser.add_argument(
        "version",
        nargs="?",
        help="Version suffix for zip names, e.g. v1.0.0",
    )
    return parser.parse_args()


def should_skip(path: Path) -> bool:
    return any(part in EXCLUDED_DIR_NAMES for part in path.parts)


def make_zip(root: Path, folder_name: str, version: str) -> None:
    source_dir = root / folder_name
    if not source_dir.is_dir():
        print(f"Warning: skip missing directory: {folder_name}")
        return

    archive_path = root / f"{folder_name}_{version}.zip"
    if archive_path.exists():
        archive_path.unlink()

    with zipfile.ZipFile(archive_path, mode="w", compression=zipfile.ZIP_DEFLATED) as zf:
        for path in source_dir.rglob("*"):
            if should_skip(path):
                continue
            if path.is_file() and path.name not in EXCLUDED_FILE_NAMES:
                zf.write(path, arcname=path.relative_to(root))

    print(f"Created: {archive_path.name}")


def main() -> int:
    args = parse_args()
    version = (args.version or "").strip()

    if not version:
        version = input("请输入版本号 (例如 v1.0.0): ").strip()

    if not version:
        print("Error: version cannot be empty")
        return 1

    root = Path(__file__).resolve().parent

    for folder in TARGET_DIRS:
        make_zip(root, folder, version)

    print("Packaging complete")
    return 0


if __name__ == "__main__":
    sys.exit(main())
