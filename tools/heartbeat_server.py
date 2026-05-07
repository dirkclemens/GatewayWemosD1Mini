#!/usr/bin/env python3
from __future__ import annotations

import os
from collections import deque
from datetime import datetime, timezone
from typing import Any

from flask import Flask, jsonify, request


app = Flask(__name__)

# Keep memory usage predictable.
_history: deque[dict[str, Any]] = deque(maxlen=500)
_latest_by_host: dict[str, dict[str, Any]] = {}


def _parse_int(value: Any, default: int = 0) -> int:
    try:
        return int(str(value).strip())
    except (TypeError, ValueError):
        return default


@app.get("/healthz")
def healthz():
    return jsonify(ok=True, service="mysensors-heartbeat", hosts=len(_latest_by_host))


@app.route("/mysensors/heartbeat", methods=["GET", "POST"])
def heartbeat():
    if request.method == "GET":
        return jsonify(
            ok=True,
            endpoint="mysensors-heartbeat",
            method="POST",
            message="Use POST for heartbeats. GET is for quick browser checks.",
            latest_hosts=len(_latest_by_host),
        )

    payload = request.form.to_dict(flat=True) if request.form else (request.get_json(silent=True) or {})
    if not isinstance(payload, dict):
        payload = {}

    host = str(payload.get("host", "unknown")).strip() or "unknown"
    boot = _parse_int(payload.get("boot"))
    uptime = _parse_int(payload.get("uptime"))
    heap = _parse_int(payload.get("heap"))
    rssi = _parse_int(payload.get("rssi"))
    wifi = _parse_int(payload.get("wifi"))
    rst = _parse_int(payload.get("rst"))
    ntp = _parse_int(payload.get("ntp"))
    boot_epoch = _parse_int(payload.get("boot_epoch"))

    now = datetime.now(timezone.utc).isoformat()
    previous = _latest_by_host.get(host)
    reboot_detected = bool(previous and boot > 0 and previous.get("boot", 0) != boot)

    row = {
        "server_time_utc": now,
        "host": host,
        "boot": boot,
        "uptime": uptime,
        "heap": heap,
        "rssi": rssi,
        "wifi": wifi,
        "rst": rst,
        "ntp": ntp,
        "boot_epoch": boot_epoch,
        "reboot_detected": reboot_detected,
        "remote_addr": request.headers.get("X-Forwarded-For", request.remote_addr),
    }

    _latest_by_host[host] = row
    _history.append(row)

    print(
        f"[HB] host={host} boot={boot} uptime={uptime}s heap={heap} rssi={rssi} "
        f"wifi={wifi} rst={rst} ntp={ntp} reboot={reboot_detected}",
        flush=True,
    )

    return jsonify(ok=True, reboot_detected=reboot_detected, latest=row)


@app.get("/mysensors/heartbeat/latest")
def latest():
    host = (request.args.get("host") or "").strip()
    if host:
        row = _latest_by_host.get(host)
        if row is None:
            return jsonify(ok=False, error="host not found", host=host), 404
        return jsonify(ok=True, latest=row)
    return jsonify(ok=True, latest=list(_latest_by_host.values()))


@app.get("/mysensors/heartbeat/history")
def history():
    limit = max(1, min(_parse_int(request.args.get("limit"), 50), 500))
    items = list(_history)[-limit:]
    return jsonify(ok=True, count=len(items), items=items)


if __name__ == "__main__":
    host = os.environ.get("HB_BIND_HOST", "0.0.0.0")
    port = _parse_int(os.environ.get("HB_BIND_PORT", "18080"), 18080)
    app.run(host=host, port=port, debug=False)
