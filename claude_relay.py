#!/usr/bin/env python3
"""
Claude Usage Relay (Clawdmeter-style)
Reads the OAuth access token from ~/.claude/.credentials.json (written by Claude Code).
POSTs a 1-token Haiku request to api.anthropic.com/v1/messages and returns the
rate-limit headers as compact JSON for the ESP32.
"""

import json
import re
import time
import urllib.request
import urllib.error
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path

PORT = 8765
API_URL = "https://api.anthropic.com/v1/messages"
CREDS_PATH = Path.home() / ".claude" / ".credentials.json"
SCRIPT_DIR = Path(__file__).parent


def get_access_token():
    try:
        text = CREDS_PATH.read_text()
        data = json.loads(text)
        if "accessToken" in data:
            return data["accessToken"]
        if "claudeAiOauth" in data and "accessToken" in data["claudeAiOauth"]:
            return data["claudeAiOauth"]["accessToken"]
        m = re.search(r'"accessToken"\s*:\s*"([A-Za-z0-9_\-.~+/=]{20,})"', text)
        if m:
            return m.group(1)
    except Exception as e:
        print(f"Credentials error: {e}")
    return None


def fetch_usage():
    token = get_access_token()
    if not token:
        print("ERR: no access token found in", CREDS_PATH)
        return None

    body = json.dumps({
        "model": "claude-haiku-4-5-20251001",
        "max_tokens": 1,
        "messages": [{"role": "user", "content": "hi"}],
    }).encode()

    req = urllib.request.Request(
        API_URL,
        data=body,
        headers={
            "Authorization": f"Bearer {token}",
            "anthropic-version": "2023-06-01",
            "anthropic-beta": "oauth-2025-04-20",
            "Content-Type": "application/json",
            "User-Agent": "claude-code/2.1.5",
        },
        method="POST",
    )

    headers = None
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            headers = resp.headers
    except urllib.error.HTTPError as e:
        headers = e.headers
        print(f"API HTTP {e.code}")
    except Exception as e:
        print(f"API error: {e}")
        return None

    def h(name):
        return headers.get(name, "") if headers else ""

    def pct(val):
        try:
            return int(float(val) * 100)
        except Exception:
            return 0

    def mins(ts):
        try:
            return max(0, int((float(ts) - time.time()) / 60))
        except Exception:
            return 0

    s  = pct(h("anthropic-ratelimit-unified-5h-utilization"))
    sr = mins(h("anthropic-ratelimit-unified-5h-reset"))
    w  = pct(h("anthropic-ratelimit-unified-7d-utilization"))
    wr = mins(h("anthropic-ratelimit-unified-7d-reset"))
    st = h("anthropic-ratelimit-unified-5h-status") or "unknown"
    ok = bool(h("anthropic-ratelimit-unified-5h-utilization"))

    return json.dumps({"s": s, "sr": sr, "w": w, "wr": wr, "st": st, "ok": ok}).encode()


def get_alert_config():
    try:
        content = (SCRIPT_DIR / "src" / "userConfig.h").read_text()
        token_m = re.search(r'#define\s+ALERT_API_TOKEN\s+"([^"]+)"', content)
        oblast_m = re.search(r'#define\s+ALERT_OBLAST_UID\s+"([^"]+)"', content)
        if token_m and oblast_m:
            return token_m.group(1), oblast_m.group(1)
    except Exception as e:
        print(f"Alert config error: {e}")
    return None, None


def fetch_alert():
    token, oblast = get_alert_config()
    if not token:
        return "N"
    url = f"https://api.alerts.in.ua/v1/iot/active_air_raid_alerts/{oblast}.json?token={token}"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "NerdMiner/2"})
        with urllib.request.urlopen(req, timeout=10) as resp:
            body = resp.read().decode().strip()
            for c in ["A", "P", "N"]:
                if c in body:
                    return c
    except Exception as e:
        print(f"Alert fetch error: {e}")
    return "N"


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith("/alert"):
            status = fetch_alert()
            print(f"Alert: {status}")
            body = status.encode()
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return

        data = fetch_usage()
        if data:
            print(f"OK  {data.decode()}")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        else:
            print("ERR: fetch failed")
            self.send_response(500)
            self.end_headers()

    def log_message(self, fmt, *args):
        pass


if __name__ == "__main__":
    print(f"Claude relay on 0.0.0.0:{PORT}")
    print(f"Credentials: {CREDS_PATH}")
    HTTPServer(("0.0.0.0", PORT), Handler).serve_forever()
