#!/usr/bin/env python3
"""OutputManager preview server.

This preview is derived from src/OutputManager.cpp (renderTftHtmlPage) and
lets you tweak runtime values via query parameters.
"""

from __future__ import annotations

import contextlib
import datetime as dt
import http.server
import json
import socket
import socketserver
import urllib.parse
import sys
import threading
import webbrowser
from pathlib import Path


def pick_port(host: str, preferred: int = 8765, max_tries: int = 20) -> int:
    """Find an available TCP port near the preferred one."""
    for port in range(preferred, preferred + max_tries):
        with contextlib.closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            if sock.connect_ex((host, port)) != 0:
                return port
    raise RuntimeError("No free port available for UI preview server")


def smoke_dot_color(smoke_level: str) -> str:
        level = smoke_level.strip().lower()
        if level == "green":
                return "#07e0"
        if level == "blue":
                return "#07ff"
        if level == "yellow":
                return "#fd20"
        return "#f800"


def build_badge(wifi_connected: bool, wifi_is_server_ap: bool) -> str:
        if not wifi_connected:
                return "OFF"
        if wifi_is_server_ap:
                return "AP"
        return "STA"


def bool_param(value: str, default: bool = False) -> bool:
        if value is None:
                return default
        return value.strip().lower() in {"1", "true", "yes", "y", "on"}


def build_preview_html(params: dict[str, str]) -> str:
        now = dt.datetime.now()

        wifi_connected = bool_param(params.get("wifi"), True)
        wifi_is_server_ap = bool_param(params.get("ap"), True)
        server_reachable = bool_param(params.get("server"), True)
        flame_detected = bool_param(params.get("flame"), False)

        mode = params.get("mode", "cloud")
        fan_mode = params.get("fan", "medium")
        smoke = params.get("smoke", "green")
        temp = params.get("temp", "25.0")
        hum = params.get("hum", "55")
        msg = params.get("msg", "")

        if not msg:
                msg = "flame_detected" if flame_detected else "screen_ready"
        msg = msg[:24]

        badge = build_badge(wifi_connected, wifi_is_server_ap)
        clock_text = params.get("clock", now.strftime("%H:%M"))
        date_text = params.get("date", f"{now.strftime('%b')} {now.day}")
        weekday_text = params.get("week", now.strftime("%a").upper())
        live_info = f"{temp}C  {hum}%  {fan_mode}"

        wifi_dot = "#07ff" if wifi_connected else "#630c"
        server_dot = "#07e0" if server_reachable else "#630c"
        smoke_dot = smoke_dot_color(smoke)

        state = {
                "mode": mode,
                "fanMode": fan_mode,
                "smokeLevel": smoke,
                "wifiConnected": wifi_connected,
                "wifiIsServerAp": wifi_is_server_ap,
                "serverReachable": server_reachable,
                "flameDetected": flame_detected,
                "footer": msg,
        }
        state_json = json.dumps(state, ensure_ascii=True)

        return f"""<!DOCTYPE html>
<html lang=\"zh-CN\">
<head>
    <meta charset=\"UTF-8\" />
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />
    <title>OutputManager Preview</title>
    <style>
        :root {{
            --bg-top: #000000;
            --bg-bottom: #18e7;
            --card-fill: #4208;
            --card-border: #94b2;
            --panel-fill: #2969;
            --text-primary: #ffffff;
            --text-muted: #c638;
            --dot-off: #630c;
        }}
        * {{ box-sizing: border-box; }}
        body {{
            margin: 0;
            min-height: 100vh;
            display: grid;
            place-items: center;
            background: radial-gradient(circle at 20% 20%, #1c222f, #0b0f16);
            font-family: "SF Pro Display", "Segoe UI", sans-serif;
            color: #d7dde8;
        }}
        .frame {{
            width: 240px;
            height: 320px;
            position: relative;
            overflow: hidden;
            border-radius: 10px;
            background: linear-gradient(to bottom, var(--bg-top), var(--bg-bottom));
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.45);
        }}
        .card {{
            position: absolute;
            left: 10px;
            top: 10px;
            width: 220px;
            height: 300px;
            border-radius: 15px;
            border: 1px solid var(--card-border);
            background: var(--card-fill);
        }}
        .t1 {{ position: absolute; left: 25px; top: 27px; font-size: 15px; font-weight: 700; color: #c6ced8; }}
        .t2 {{ position: absolute; left: 186px; top: 27px; font-size: 28px; font-weight: 700; color: var(--text-primary); }}
        .clock {{ position: absolute; left: 25px; top: 108px; font-size: 42px; font-weight: 700; color: var(--text-primary); letter-spacing: 1px; }}
        .date {{ position: absolute; left: 28px; top: 257px; font-size: 12px; font-weight: 700; color: var(--text-muted); }}
        .week {{ position: absolute; left: 120px; top: 257px; font-size: 12px; font-weight: 700; color: var(--text-muted); }}
        .dot {{ position: absolute; width: 16px; height: 16px; border-radius: 50%; right: 25px; box-shadow: 0 0 8px rgba(255,255,255,.15) inset; }}
        .smoke {{ top: 201px; background: {smoke_dot}; }}
        .server {{ top: 230px; background: {server_dot}; }}
        .wifi {{ top: 259px; background: {wifi_dot}; }}
        .footer-brand {{ position: absolute; left: 67px; top: 292px; font-size: 12px; color: var(--text-primary); font-weight: 700; }}
        .panel-top {{ position: absolute; left: 24px; top: 58px; width: 130px; height: 28px; border-radius: 8px; background: var(--panel-fill); color: var(--text-primary); padding: 6px 8px; font-size: 14px; }}
        .panel-mid {{ position: absolute; left: 24px; top: 186px; width: 166px; height: 50px; border-radius: 10px; background: var(--panel-fill); padding: 8px; }}
        .panel-mid .label {{ color: var(--text-muted); font-size: 12px; }}
        .panel-mid .value {{ color: var(--text-primary); font-size: 14px; margin-top: 6px; white-space: nowrap; }}
        .panel-bottom {{ position: absolute; left: 24px; top: 242px; width: 166px; height: 28px; border-radius: 8px; background: var(--panel-fill); color: var(--text-primary); padding: 6px 8px; font-size: 12px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }}
        .note {{ margin-top: 14px; width: 560px; max-width: calc(100vw - 24px); background: #10151f; border: 1px solid #273248; border-radius: 10px; padding: 10px; font: 12px/1.5 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace; }}
    </style>
</head>
<body>
    <div class=\"frame\">
        <div class=\"card\"></div>
        <div class=\"t1\">ESP32-HOME</div>
        <div class=\"t2\">{badge}</div>
        <div class=\"clock\">{clock_text}</div>
        <div class=\"date\">{date_text}</div>
        <div class=\"week\">{weekday_text}</div>
        <div class=\"dot smoke\"></div>
        <div class=\"dot server\"></div>
        <div class=\"dot wifi\"></div>
        <div class=\"footer-brand\">MADE IN 600</div>
        <div class=\"panel-top\">{mode}</div>
        <div class=\"panel-mid\">
            <div class=\"label\">TEMP / HUM / FAN</div>
            <div class=\"value\">{live_info}</div>
        </div>
        <div class=\"panel-bottom\">{msg}</div>
    </div>
    <div class=\"note\">
        Preview source: src/OutputManager.cpp::renderTftHtmlPage<br />
        State: {state_json}<br />
        Query example: ?wifi=1&ap=1&server=1&mode=cloud&temp=24.8&hum=62&fan=high&smoke=yellow&msg=fan_ok
    </div>
</body>
</html>
"""


class PreviewRequestHandler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, *args, project_root: Path, **kwargs):
                self._project_root = project_root
                super().__init__(*args, directory=str(project_root), **kwargs)

        def do_GET(self) -> None:  # noqa: N802
                parsed = urllib.parse.urlparse(self.path)
                if parsed.path in {"/", "/preview"}:
                        params = {k: v[-1] for k, v in urllib.parse.parse_qs(parsed.query).items()}
                        content = build_preview_html(params).encode("utf-8")
                        self.send_response(200)
                        self.send_header("Content-Type", "text/html; charset=utf-8")
                        self.send_header("Content-Length", str(len(content)))
                        self.end_headers()
                        self.wfile.write(content)
                        return
                super().do_GET()


def main() -> int:
    project_root = Path(__file__).resolve().parents[1]
    host = "127.0.0.1"
    port = pick_port(host)

    handler = lambda *args, **kwargs: PreviewRequestHandler(  # noqa: E731
        *args,
        project_root=project_root,
        **kwargs,
    )

    with socketserver.TCPServer((host, port), handler) as httpd:
        url = f"http://{host}:{port}/preview"

        print("=" * 52)
        print("OutputManager preview server started")
        print(f"Project root: {project_root}")
        print(f"Preview URL: {url}")
        print("Legacy static page: http://127.0.0.1:%d/html/Index.html" % port)
        print("Press Ctrl+C to stop")
        print("=" * 52)

        # Delay a bit to make sure server is ready before browser tries to load it.
        threading.Timer(0.4, lambda: webbrowser.open(url)).start()

        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nUI preview server stopped")
            return 0

    # Keep explicit return for static analyzers.
    return 0


if __name__ == "__main__":
    sys.exit(main())
