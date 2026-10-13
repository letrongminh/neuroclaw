#!/usr/bin/env python3
"""
NeuroClaw-ESP end to end on Espressif QEMU, against a fake Telegram and a fake
LLM on the host (no network beyond the host, no real bot, no real key).

    python3 esp32/scripts/qemu_e2e.py esp32/examples/qemu/build

Needs the ESP-IDF environment (v5.4) with qemu-xtensa. The chip reaches the host
at 10.0.2.2 through QEMU's OpenCores Ethernet; this script serves both APIs on
127.0.0.1:8090. The scenario:

  1. a stranger writes "mở cửa": dropped on the chip, the LLM never sees it;
  2. the owner writes "bật đèn": the model calls living_light_on, the gate
     allows it, the owner gets the model's reply;
  3. the owner writes "mở cửa": the model calls front_door_unlock, the gate asks
     the owner; the chip (not the model) sends the approve / refuse buttons, and
     the model is told it cannot approve and must not say it is done;
  4. a stranger presses approve: dropped;
  5. the owner presses approve: the gate allows the call;
  6. the owner presses it again: the question is gone, nothing runs.

Throughout, the model is offered exactly the gate's two tools, and the bot
token and LLM key never appear on the UART. Exit 0 only when all of it held.
"""

from __future__ import annotations

import json
import re
import subprocess
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

PORT = 8090
OWNER = 123456789
STRANGER = 555
TOKEN = "7000000001:AAQemuOnlyNotARealTokenxxxxxxxxxxxx"
KEY = "sk-qemu-only-not-a-real-key"
TIMEOUT_S = 120.0


class Cloud:
    """The fake Telegram (updates queue, sent messages) and the scripted LLM."""

    def __init__(self) -> None:
        self.lock = threading.Condition()
        self.updates: list[dict] = []
        self.next_id = 1
        self.sent: list[dict] = []
        self.acks: list[dict] = []
        self.llm_requests: list[dict] = []
        self.bad_auth = 0

    def push(self, update: dict) -> None:
        with self.lock:
            update["update_id"] = self.next_id
            self.next_id += 1
            self.updates.append(update)
            self.lock.notify_all()

    def message(self, user: int, text: str) -> None:
        self.push(
            {
                "message": {
                    "message_id": self.next_id,
                    "from": {"id": user, "is_bot": False},
                    "chat": {"id": user, "type": "private"},
                    "text": text,
                }
            }
        )

    def press(self, user: int, data: str) -> None:
        self.push(
            {
                "callback_query": {
                    "id": f"cb{self.next_id}",
                    "from": {"id": user, "is_bot": False},
                    "data": data,
                    "message": {"message_id": 1, "chat": {"id": user, "type": "private"}},
                }
            }
        )

    def poll(self, offset: int, wait_s: float) -> list[dict]:
        deadline = time.monotonic() + wait_s
        with self.lock:
            while True:
                ready = [u for u in self.updates if u["update_id"] >= offset]
                if ready or time.monotonic() >= deadline:
                    return ready
                self.lock.wait(deadline - time.monotonic())

    def llm(self, body: dict) -> dict:
        """Calls a tool for "đèn" / "cửa"; after a tool result, says what it read."""
        self.llm_requests.append(body)
        last = body["messages"][-1]
        if last["role"] == "user":
            text = last["content"]
            if "đèn" in text:
                return _calls(("living_light_on", {}))
            if "cửa" in text:
                return _calls(("front_door_unlock", {"duration_s": 10}))
            return _text("Tôi có thể bật đèn hoặc xin mở cửa.")
        if last["role"] == "tool":
            content = last["content"]
            if '"status":"ALLOW"' in content:
                return _text("Đã bật đèn.")
            if "did NOT do this yet" in content:
                return _text("Đang chờ bạn duyệt trên Telegram.")
            return _text("Không làm được.")
        return _text("…")


def _text(content: str) -> dict:
    return {"choices": [{"message": {"role": "assistant", "content": content}}]}


def _calls(*calls: tuple[str, dict]) -> dict:
    return {
        "choices": [
            {
                "message": {
                    "role": "assistant",
                    "content": None,
                    "tool_calls": [
                        {
                            "id": f"call_{i}",
                            "type": "function",
                            "function": {"name": name, "arguments": json.dumps(args)},
                        }
                        for i, (name, args) in enumerate(calls)
                    ],
                }
            }
        ]
    }


def serve(cloud: Cloud) -> ThreadingHTTPServer:
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args) -> None:  # quiet
            pass

        def _reply(self, payload: dict, status: int = 200) -> None:
            data = json.dumps(payload).encode()
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def _body(self) -> dict:
            length = int(self.headers.get("Content-Length", "0"))
            return json.loads(self.rfile.read(length) or b"{}")

        def do_GET(self) -> None:
            url = urlparse(self.path)
            if url.path == f"/bot{TOKEN}/getUpdates":
                query = parse_qs(url.query)
                offset = int(query.get("offset", ["0"])[0])
                wait_s = min(float(query.get("timeout", ["0"])[0]), 2.0)
                self._reply({"ok": True, "result": cloud.poll(offset, wait_s)})
            else:
                self._reply({"ok": False, "description": "Not Found"}, 404)

        def do_POST(self) -> None:
            url = urlparse(self.path)
            body = self._body()
            if url.path == f"/bot{TOKEN}/sendMessage":
                cloud.sent.append(body)
                self._reply({"ok": True, "result": {"message_id": len(cloud.sent)}})
            elif url.path == f"/bot{TOKEN}/answerCallbackQuery":
                cloud.acks.append(body)
                self._reply({"ok": True, "result": True})
            elif url.path == "/v1/chat/completions":
                if self.headers.get("Authorization") != f"Bearer {KEY}":
                    cloud.bad_auth += 1
                    self._reply({"error": {"message": "bad key"}}, 401)
                    return
                self._reply(cloud.llm(body))
            else:
                self._reply({"ok": False, "description": "Not Found"}, 404)

    server = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


class Fail(Exception):
    pass


class Uart:
    def __init__(self, log: Path) -> None:
        self.log = log
        self.pos = 0

    def text(self) -> str:
        return self.log.read_text(errors="replace").replace("\r", "") if self.log.exists() else ""

    def expect(self, pattern: str, timeout: float = TIMEOUT_S) -> str:
        regex = re.compile(pattern, re.MULTILINE)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            match = regex.search(self.text(), self.pos)
            if match:
                self.pos = match.end()
                return match.group(0)
            time.sleep(0.1)
        raise Fail(f"no /{pattern}/ within {timeout:.0f}s; UART tail:\n{self.text()[-1500:]}")


def wait_for(what: str, check, timeout: float = TIMEOUT_S):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = check()
        if value:
            return value
        time.sleep(0.1)
    raise Fail(f"timed out waiting for {what}")


def run(build: Path) -> list[str]:
    cloud = Cloud()
    server = serve(cloud)
    flash = build / "e2e-flash.bin"
    log = build / "e2e-uart.log"
    log.unlink(missing_ok=True)
    subprocess.run(
        ["esptool.py", "--chip", "esp32s3", "merge_bin", "--fill-flash-size", "4MB"]
        + ["-o", str(flash), "@flash_args"],
        cwd=build,
        check=True,
        stdout=subprocess.DEVNULL,
    )
    qemu = subprocess.Popen(
        ["qemu-system-xtensa", "-machine", "esp32s3", "-display", "none", "-monitor", "none"]
        + ["-drive", f"file={flash},if=mtd,format=raw", "-nic", "user,model=open_eth"]
        + ["-serial", f"file:{log}"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    uart = Uart(log)
    evidence: list[str] = []
    try:
        evidence.append(uart.expect(r"^NC_READY tools=2$"))

        # 1. A stranger: dropped before the model.
        cloud.message(STRANGER, "mở cửa")
        evidence.append(uart.expect(rf"^NC_IGNORED from={STRANGER}$"))
        if cloud.llm_requests:
            raise Fail("a stranger's message reached the LLM")

        # 2. The light.
        cloud.message(OWNER, "bật đèn")
        evidence.append(uart.expect(r"^NC_TOOL name=living_light_on status=ALLOW reason=-$"))
        wait_for("the light reply", lambda: any(m["text"] == "Đã bật đèn." for m in cloud.sent))

        # 3. The door: the gate asks; the chip sends the buttons.
        cloud.message(OWNER, "mở cửa giúp tôi")
        evidence.append(
            uart.expect(
                r"^NC_TOOL name=front_door_unlock status=BLOCK reason=awaiting_confirmation$"
            )
        )
        evidence.append(uart.expect(r"^NC_ASK id=c-1$"))
        ask = wait_for(
            "the approval buttons", lambda: [m for m in cloud.sent if "reply_markup" in m]
        )[0]
        row = ask["reply_markup"]["inline_keyboard"][0]
        if [b["callback_data"] for b in row] != ["nc:y:c-1", "nc:n:c-1"]:
            raise Fail(f"unexpected buttons: {row}")
        if ask["chat_id"] != OWNER:
            raise Fail("the buttons went to someone other than the owner")
        wait_for(
            "the waiting reply",
            lambda: any(m["text"] == "Đang chờ bạn duyệt trên Telegram." for m in cloud.sent),
        )
        told = [
            m["content"] for r in cloud.llm_requests for m in r["messages"] if m["role"] == "tool"
        ]
        if not any("You cannot approve it" in t for t in told):
            raise Fail("the model was not told it cannot approve")

        # 4. A stranger presses approve.
        cloud.press(STRANGER, "nc:y:c-1")
        evidence.append(uart.expect(rf"^NC_IGNORED from={STRANGER}$"))

        # 5. The owner approves: the gate allows it.
        cloud.press(OWNER, "nc:y:c-1")
        evidence.append(
            uart.expect(r"^NC_BUTTON id=c-1 approve=1 status=ALLOW reason=owner_confirmed$")
        )

        # 6. Pressing again does nothing.
        cloud.press(OWNER, "nc:y:c-1")
        evidence.append(
            uart.expect(r"^NC_BUTTON id=c-1 approve=1 status=BLOCK reason=unknown_confirmation$")
        )
    finally:
        qemu.terminate()
        try:
            qemu.wait(timeout=10)
        except subprocess.TimeoutExpired:
            qemu.kill()
        server.shutdown()

    offered = {
        tuple(sorted(t["function"]["name"] for t in r.get("tools", []))) for r in cloud.llm_requests
    }
    if offered != {("front_door_unlock", "living_light_on")}:
        raise Fail(f"the model was offered other tools: {offered}")
    if cloud.bad_auth:
        raise Fail("the LLM key was not sent as configured")
    text = uart.text()
    for secret in (TOKEN, TOKEN.split(":", 1)[1], KEY):
        if secret in text:
            raise Fail("a secret reached the UART")
    if len(re.findall(r"^NC_BUTTON ", text, re.MULTILINE)) != 2:
        raise Fail("a press other than the owner's two reached the gate")
    if len(cloud.acks) != 2:
        raise Fail(f"expected the owner's two presses acknowledged, got {len(cloud.acks)}")
    evidence.append(f"LLM calls: {len(cloud.llm_requests)}; messages sent: {len(cloud.sent)}")
    return evidence


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: qemu_e2e.py <example build dir>", file=sys.stderr)
        return 2
    try:
        evidence = run(Path(argv[1]).resolve())
    except (Fail, subprocess.CalledProcessError) as error:
        print(f"::error::{error}")
        return 1
    print("NeuroClaw-ESP on QEMU: all steps passed")
    for line in evidence:
        print(f"  {line}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
