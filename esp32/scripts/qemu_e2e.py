#!/usr/bin/env python3
"""
NeuroClaw-ESP end to end on Espressif QEMU, against a fake Telegram and a fake
LLM on the host (no network beyond the host, no real bot, no real key).

    python3 esp32/scripts/qemu_e2e.py esp32/examples/qemu/build

Needs the ESP-IDF environment (v5.4) with qemu-xtensa. The chip reaches the host
at 10.0.2.2 through QEMU's OpenCores Ethernet; this script serves both APIs on
127.0.0.1:8090. Two boots on one flash image:

  boot 1  a stranger is dropped before the model; the clock is set from
          Telegram's Date header; "bật đèn" → the gate allows the light; "mở
          cửa" → the gate asks, the chip (not the model) sends the owner the
          buttons, a stranger's press is dropped, the owner's opens it once;
          "nhớ giúp…" → memory/MEMORY.md, and the next answer reads it from the
          prompt; writing sessions/ is refused; "nhắc tôi… sau 5 giây" → a cron
          job that, five seconds later, sends the owner the reminder;
          "gửi tệp ghi nhớ" → send_file streams memory/MEMORY.md to the owner
          and the turn ends there; a missing file and a message to a stranger
          are refused; an image goes as a photo, and as a document when
          Telegram refuses the photo; a reminder that the model delivers with
          `message` reaches the owner once, its final reply not sent too;
  boot 2  the conversation and the memory survived the reboot, and no old
          message is handled again; what outgrew the kept history was folded
          by the LLM into a CONTEXT_SUMMARY that the next answers still use;
          the web; the configured LLM URL answers 503, the fallback URL (its
          own key and model) answers, and while the first cools down it is
          not called again; a task in HEARTBEAT.md is checked within a
          minute, and HEARTBEAT_OK is not sent to the owner.

Throughout, the model is offered exactly the brain's tools and the gate's two,
and the bot token and LLM keys never appear on the UART. Exit 0 only when all
of it held.
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
FALLBACK_KEY = "sk-qemu-fallback-not-a-real-key"
TIMEOUT_S = 120.0
STACK_FLOOR = 1024  # bytes the brain task must never use, after the deepest turn


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
        self.bad_bodies = 0
        self.summaries: list[str] = []
        self.searches: list[dict] = []
        self.files: list[dict] = []  # sendPhoto / sendDocument
        self.primary_down = False
        self.llm_urls: list[str] = []  # "v1" or "v2", per LLM call that got an answer or a 503

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
                    "message": {
                        "message_id": 1,
                        "chat": {"id": user, "type": "private"},
                    },
                }
            }
        )

    def poll(self, offset: int, wait_s: float) -> list[dict]:
        deadline = time.monotonic() + wait_s
        with self.lock:
            # As Telegram: asking from `offset` confirms, and forgets, everything before it.
            self.updates = [u for u in self.updates if u["update_id"] >= offset]
            while True:
                ready = [u for u in self.updates if u["update_id"] >= offset]
                if ready or time.monotonic() >= deadline:
                    return ready
                self.lock.wait(deadline - time.monotonic())

    def llm(self, body: dict) -> dict:
        """A scripted model: what it does for each owner message, and what it says
        after a tool result. It reads its memory from the system prompt only."""
        self.llm_requests.append(body)
        messages = body["messages"]
        system = messages[0]["content"]
        last = messages[-1]
        if system.startswith("Summarize this conversation"):
            self.summaries.append(last["content"])
            # Like a real model told to keep the previous summary's points.
            if "nhớ giúp" in last["content"] or "thích đèn vàng" in last["content"]:
                return _text("Lúc đầu chủ nhà nhờ nhớ là họ thích đèn vàng; đã bật đèn, mở cửa.")
            return _text("Chủ nhà đã trò chuyện với trợ lý.")
        if last["role"] == "user":
            text = last["content"]
            if text.startswith("# Heartbeat Check"):
                return _text("HEARTBEAT_OK")
            if text.startswith("[Scheduled task] Gửi bằng tin nhắn: "):
                said = text.removeprefix("[Scheduled task] Gửi bằng tin nhắn: ")
                return _calls(("message", {"content": "💧 " + said}))
            if text.startswith("[Scheduled task] "):
                return _text("⏰ " + text.removeprefix("[Scheduled task] "))
            if text.startswith("gửi tệp ghi nhớ"):
                return _calls(("send_file", {"path": "memory/MEMORY.md"}))
            if text.startswith("gửi tệp không có"):
                return _calls(("send_file", {"path": "khong-co.md"}))
            if text.startswith("gửi ảnh"):
                name = "hong.png" if "hỏng" in text else "cua.png"
                return _calls(
                    ("write_file", {"path": f"photos/{name}", "content": "PNG giả"}),
                    ("send_file", {"path": f"photos/{name}"}),
                )
            if text.startswith("nhắn cho người lạ"):
                return _calls(("message", {"content": "chào", "chat_id": str(STRANGER)}))
            if text.startswith("hẹn gửi tin"):
                return _calls(
                    (
                        "cron",
                        {
                            "action": "add",
                            "message": "Gửi bằng tin nhắn: Uống nước",
                            "at_seconds": 5,
                        },
                    )
                )
            if text.startswith("nhớ giúp"):
                return _calls(
                    (
                        "append_file",
                        {
                            "path": "memory/MEMORY.md",
                            "content": "- Chủ nhà thích đèn vàng\n",
                        },
                    )
                )
            if text == "tôi thích đèn gì?":
                return _text("Bạn thích đèn vàng." if "đèn vàng" in system else "Tôi không biết.")
            if text == "lúc đầu tôi nhờ gì?":
                known = "CONTEXT_SUMMARY" in system and "thích đèn vàng; đã bật đèn" in system
                return _text("Lúc đầu bạn nhờ tôi nhớ đèn vàng." if known else "Tôi không nhớ.")
            if text == "tôi vừa nhờ gì?":
                asked = [m["content"] for m in messages if m["role"] == "user"]
                said = any(a.startswith("hẹn gửi tin") for a in asked)
                return _text("Bạn nhờ tôi nhắn uống nước." if said else "Tôi không nhớ.")
            if text.startswith("nhắc tôi"):
                return _calls(
                    (
                        "cron",
                        {
                            "action": "add",
                            "message": "Nhắc chủ nhà uống thuốc",
                            "at_seconds": 5,
                        },
                    )
                )
            if text.startswith("kiểm tra định kỳ"):
                return _calls(
                    (
                        "write_file",
                        {
                            "path": "HEARTBEAT.md",
                            "content": "# Việc\n- Cửa trước đã khoá chưa\n",
                        },
                    )
                )
            if text.startswith("tìm thời tiết"):
                return _calls(("web_search", {"query": "thời tiết Hà Nội", "count": 2}))
            if text.startswith("đọc tin"):
                return _calls(("web_fetch", {"url": "http://10.0.2.2:8090/news"}))
            if text.startswith("mở trang router"):
                return _calls(("web_fetch", {"url": "http://10.0.2.2:8090/redirect"}))
            if text.startswith("đọc 192"):
                return _calls(("web_fetch", {"url": "http://192.168.1.1/"}))
            if text.startswith("sửa lịch sử"):
                return _calls(("write_file", {"path": "sessions/telegram.json", "content": "[]"}))
            if "đèn" in text:
                return _calls(("living_light_on", {}))
            if "cửa" in text:
                return _calls(("front_door_unlock", {"duration_s": 10}))
            return _text("Tôi có thể bật đèn hoặc xin mở cửa.")
        if last["role"] == "tool":
            content = last["content"]
            if content.startswith("Results for:"):
                return _text("Đã tìm: " + content.splitlines()[1].split(". ", 1)[1])
            if "Bản tin" in content and "<" not in content:
                return _text("Trang viết: " + content[:40])
            if content.startswith("Message sent to telegram:"):
                return _text("Đã gửi tin.")  # must not reach the owner: the message did
            if content.startswith("File ") and content.endswith("sent to user"):
                return _text("Đã gửi ảnh.")
            if content.startswith("ok:"):
                return _text("Xong.")
            if content.startswith("error:"):
                return _text("Không được.")
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
            try:
                self.wfile.write(data)
            except (BrokenPipeError, ConnectionResetError):
                pass  # QEMU stopped mid long-poll

        def _upload(self, method: str) -> None:
            """sendPhoto / sendDocument: multipart/form-data, read as Telegram would."""
            ctype = self.headers.get("Content-Type", "")
            boundary = ctype.split("boundary=", 1)[1].encode() if "boundary=" in ctype else b""
            data = self.rfile.read(int(self.headers.get("Content-Length", "0")))
            if not boundary or not data.endswith(b"--" + boundary + b"--\r\n"):
                cloud.bad_bodies += 1
                self._reply({"ok": False, "description": "bad multipart"}, 400)
                return
            fields: dict[str, dict] = {}
            for part in data.split(b"--" + boundary)[1:-1]:
                head, _, value = part.removeprefix(b"\r\n").partition(b"\r\n\r\n")
                headers = head.decode()
                name = re.search(r'name="([^"]+)"', headers).group(1)
                filename = re.search(r'filename="([^"]*)"', headers)
                fields[name] = {
                    "value": value.removesuffix(b"\r\n"),
                    "filename": filename.group(1) if filename else None,
                    "headers": headers,
                }
            key = "photo" if method == "sendPhoto" else "document"
            if "chat_id" not in fields or key not in fields:
                self._reply({"ok": False, "description": "missing field"}, 400)
                return
            if method == "sendPhoto" and fields[key]["filename"].startswith("hong"):
                self._reply({"ok": False, "description": "IMAGE_PROCESS_FAILED"}, 400)
                return
            cloud.files.append(
                {
                    "method": method,
                    "chat_id": int(fields["chat_id"]["value"]),
                    "filename": fields[key]["filename"],
                    "data": fields[key]["value"],
                    "headers": fields[key]["headers"],
                }
            )
            self._reply({"ok": True, "result": {"message_id": 1000 + len(cloud.files)}})

        def _body(self) -> dict | None:
            length = int(self.headers.get("Content-Length", "0"))
            try:
                return json.loads((self.rfile.read(length) or b"{}").decode("utf-8"))
            except (UnicodeDecodeError, json.JSONDecodeError):
                cloud.bad_bodies += 1  # a real API rejects it too
                return None

        def do_GET(self) -> None:
            url = urlparse(self.path)
            if url.path == "/search":
                cloud.searches.append(parse_qs(url.query))
                self._reply(
                    {
                        "results": [
                            {
                                "title": "Thời tiết <b>Hà Nội</b> hôm nay",
                                "url": "https://weather.example/hn",
                                "content": "Mưa rào &amp; dông",
                            },
                            {
                                "title": "Dự báo 3 ngày",
                                "url": "https://weather.example/3d",
                            },
                        ]
                    }
                )
                return
            if url.path == "/news":
                page = (
                    "<html><head><script>steal()</script></head><body>"
                    "<h1>Bản tin</h1><p>Hà Nội &amp; mưa.</p></body></html>"
                ).encode()
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Content-Length", str(len(page)))
                self.end_headers()
                self.wfile.write(page)
                return
            if url.path == "/redirect":
                self.send_response(302)
                self.send_header("Location", "http://192.168.1.10/admin")
                self.send_header("Content-Length", "0")
                self.end_headers()
                return
            if url.path == f"/bot{TOKEN}/getUpdates":
                query = parse_qs(url.query)
                offset = int(query.get("offset", ["0"])[0])
                wait_s = min(float(query.get("timeout", ["0"])[0]), 2.0)
                self._reply({"ok": True, "result": cloud.poll(offset, wait_s)})
            else:
                self._reply({"ok": False, "description": "Not Found"}, 404)

        def do_POST(self) -> None:
            url = urlparse(self.path)
            for method in ("sendPhoto", "sendDocument"):
                if url.path == f"/bot{TOKEN}/{method}":
                    self._upload(method)
                    return
            body = self._body()
            if body is None:
                self._reply({"ok": False, "description": "bad request body"}, 400)
                return
            if url.path == f"/bot{TOKEN}/sendMessage":
                cloud.sent.append(body)
                self._reply({"ok": True, "result": {"message_id": len(cloud.sent)}})
            elif url.path == f"/bot{TOKEN}/answerCallbackQuery":
                cloud.acks.append(body)
                self._reply({"ok": True, "result": True})
            elif url.path in ("/v1/chat/completions", "/v2/chat/completions"):
                primary = url.path.startswith("/v1/")
                key, model = (KEY, "fake") if primary else (FALLBACK_KEY, "fake-backup")
                if self.headers.get("Authorization") != f"Bearer {key}" or body["model"] != model:
                    cloud.bad_auth += 1
                    self._reply({"error": {"message": "bad key or model"}}, 401)
                    return
                cloud.llm_urls.append("v1" if primary else "v2")
                if primary and cloud.primary_down:
                    self._reply({"error": {"message": "overloaded"}}, 503)
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


def boot(flash: Path, log: Path) -> subprocess.Popen:
    log.unlink(missing_ok=True)
    return subprocess.Popen(
        [
            "qemu-system-xtensa",
            "-machine",
            "esp32s3",
            "-display",
            "none",
            "-monitor",
            "none",
        ]
        + ["-drive", f"file={flash},if=mtd,format=raw", "-nic", "user,model=open_eth"]
        + ["-serial", f"file:{log}"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def stop(qemu: subprocess.Popen) -> None:
    """SIGTERM: QEMU writes the flash file back on a clean stop."""
    if qemu.poll() is None:
        qemu.terminate()
        try:
            qemu.wait(timeout=10)
        except subprocess.TimeoutExpired:
            qemu.kill()
            qemu.wait()


def sent(cloud: Cloud, text: str):
    return lambda: any(m["text"] == text for m in cloud.sent)


def run(build: Path) -> list[str]:
    cloud = Cloud()
    server = serve(cloud)
    flash = build / "e2e-flash.bin"
    logs = [build / "e2e-uart-1.log", build / "e2e-uart-2.log"]
    subprocess.run(
        ["esptool.py", "--chip", "esp32s3", "merge_bin", "--fill-flash-size", "4MB"]
        + ["-o", str(flash), "@flash_args"],
        cwd=build,
        check=True,
        stdout=subprocess.DEVNULL,
    )
    evidence: list[str] = []
    qemu = boot(flash, logs[0])
    uart = Uart(logs[0])
    try:
        # --- boot 1 ---------------------------------------------------------------------------
        evidence.append(uart.expect(r"^NC_SESSION new jobs=0$"))
        # workspace 5, cron, web 2, message, send_file, gate 2; one fallback LLM URL
        evidence.append(uart.expect(r"^NC_READY tools=12 fallback=1$"))
        evidence.append(uart.expect(r"^NC_CLOCK set_from=telegram$"))  # the first poll's Date

        # A stranger: dropped before the model.
        cloud.message(STRANGER, "mở cửa")
        evidence.append(uart.expect(rf"^NC_IGNORED from={STRANGER}$"))
        if cloud.llm_requests:
            raise Fail("a stranger's message reached the LLM")

        # The light.
        cloud.message(OWNER, "bật đèn")
        evidence.append(uart.expect(r"^NC_TOOL name=living_light_on status=ALLOW reason=-$"))
        wait_for("the light reply", sent(cloud, "Đã bật đèn."))

        # The door: the gate asks; the chip sends the buttons.
        cloud.message(OWNER, "mở cửa giúp tôi")
        evidence.append(
            uart.expect(
                r"^NC_TOOL name=front_door_unlock status=BLOCK reason=awaiting_confirmation$"
            )
        )
        evidence.append(uart.expect(r"^NC_ASK id=c-1$"))
        ask = wait_for(
            "the approval buttons",
            lambda: [m for m in cloud.sent if "reply_markup" in m],
        )[0]
        row = ask["reply_markup"]["inline_keyboard"][0]
        if [b["callback_data"] for b in row] != ["nc:y:c-1", "nc:n:c-1"]:
            raise Fail(f"unexpected buttons: {row}")
        if ask["chat_id"] != OWNER:
            raise Fail("the buttons went to someone other than the owner")
        wait_for("the waiting reply", sent(cloud, "Đang chờ bạn duyệt trên Telegram."))
        told = [
            m["content"] for r in cloud.llm_requests for m in r["messages"] if m["role"] == "tool"
        ]
        if not any("You cannot approve it" in t for t in told):
            raise Fail("the model was not told it cannot approve")
        cloud.press(STRANGER, "nc:y:c-1")
        evidence.append(uart.expect(rf"^NC_IGNORED from={STRANGER}$"))
        cloud.press(OWNER, "nc:y:c-1")
        evidence.append(
            uart.expect(r"^NC_BUTTON id=c-1 approve=1 status=ALLOW reason=owner_confirmed$")
        )
        cloud.press(OWNER, "nc:y:c-1")
        evidence.append(
            uart.expect(r"^NC_BUTTON id=c-1 approve=1 status=BLOCK reason=unknown_confirmation$")
        )

        # Memory: written to memory/MEMORY.md, read back through the prompt.
        cloud.message(OWNER, "nhớ giúp tôi là tôi thích đèn vàng")
        evidence.append(uart.expect(r"^NC_TOOL name=append_file result=ok$"))
        cloud.message(OWNER, "tôi thích đèn gì?")
        wait_for("the remembered answer", sent(cloud, "Bạn thích đèn vàng."))

        # The brain's own files are not the model's to write.
        cloud.message(OWNER, "sửa lịch sử chat")
        evidence.append(uart.expect(r"^NC_TOOL name=write_file result=error$"))

        # A reminder: scheduled, and five seconds later the owner gets it.
        cloud.message(OWNER, "nhắc tôi uống thuốc sau 5 giây")
        evidence.append(uart.expect(r"^NC_TOOL name=cron result=ok$"))
        evidence.append(uart.expect(r"^NC_CRON_RUN id=j1$", timeout=40))
        evidence.append(uart.expect(r"^NC_CRON result=sent$"))
        wait_for("the reminder", sent(cloud, "⏰ Nhắc chủ nhà uống thuốc"))
        reminder = next(m for m in cloud.sent if m["text"] == "⏰ Nhắc chủ nhà uống thuốc")
        if reminder["chat_id"] != OWNER:
            raise Fail("the reminder went to someone other than the owner")

        # send_file: the memory file, streamed to the owner; the file was the answer, so the
        # turn ends without another LLM call and without a text reply.
        calls, texts = len(cloud.llm_requests), len(cloud.sent)
        cloud.message(OWNER, "gửi tệp ghi nhớ")
        evidence.append(uart.expect(r"^NC_SEND_FILE kind=document bytes=\d+ http=200$"))
        evidence.append(uart.expect(r"^NC_REPLY skipped=sent_by_tool$"))
        sent_file = cloud.files[-1]
        if (sent_file["method"], sent_file["chat_id"], sent_file["filename"]) != (
            "sendDocument",
            OWNER,
            "MEMORY.md",
        ) or "thích đèn vàng" not in sent_file["data"].decode():
            raise Fail(f"unexpected file sent: {sent_file}")
        turns = [r for r in cloud.llm_requests[calls:] if r.get("tools")]  # not summaries
        if len(turns) != 1 or len(cloud.sent) != texts:
            raise Fail("after send_file delivered, the LLM was called again or a reply was sent")
        cloud.message(OWNER, "gửi tệp không có")
        evidence.append(uart.expect(r"^NC_TOOL name=send_file result=error$"))
        # An image: a photo; a photo Telegram refuses goes again as a document.
        cloud.message(OWNER, "gửi ảnh cửa")
        evidence.append(uart.expect(r"^NC_SEND_FILE kind=photo bytes=\d+ http=200$"))
        wait_for("the photo reply", sent(cloud, "Đã gửi ảnh."))  # write_file ran too: not quiet
        if (
            cloud.files[-1]["method"] != "sendPhoto"
            or "image/png" not in cloud.files[-1]["headers"]
        ):
            raise Fail(f"the image did not go as a photo: {cloud.files[-1]}")
        cloud.message(OWNER, "gửi ảnh hỏng")
        evidence.append(uart.expect(r"^NC_SEND_FILE kind=document bytes=\d+ http=200$"))
        if (cloud.files[-1]["method"], cloud.files[-1]["filename"]) != (
            "sendDocument",
            "hong.png",
        ):
            raise Fail(f"the refused photo did not go as a document: {cloud.files[-1]}")
        # message: never to anyone but the owner.
        cloud.message(OWNER, "nhắn cho người lạ")
        evidence.append(uart.expect(r"^NC_TOOL name=message result=error$"))
        if any(m["chat_id"] != OWNER for m in cloud.sent) or any(
            f["chat_id"] != OWNER for f in cloud.files
        ):
            raise Fail("something was sent to someone other than the owner")
        # A reminder the model delivers itself with `message`: the owner gets it once, and
        # the model's final words after it are not sent too.
        cloud.message(OWNER, "hẹn gửi tin uống nước sau 5 giây")
        evidence.append(uart.expect(r"^NC_TOOL name=cron result=ok$"))
        evidence.append(uart.expect(r"^NC_CRON_RUN id=j\d+$", timeout=40))
        evidence.append(uart.expect(r"^NC_MESSAGE chars=\d+ http=200$"))
        evidence.append(uart.expect(r"^NC_CRON result=sent_by_tool$"))
        wait_for("the messaged reminder", sent(cloud, "💧 Uống nước"))
        if sum(m["text"] == "💧 Uống nước" for m in cloud.sent) != 1 or any(
            m["text"] == "Đã gửi tin." for m in cloud.sent
        ):
            raise Fail("the messaged reminder was sent twice, or the final reply too")
        stop(qemu)

        # --- boot 2: the same flash ---------------------------------------------------------
        before = len(cloud.llm_requests)
        qemu = boot(flash, logs[1])
        uart = Uart(logs[1])
        restored = uart.expect(r"^NC_SESSION restored=(\d+) jobs=0$")
        evidence.append(restored)
        if int(re.search(r"restored=(\d+)", restored).group(1)) < 10:
            raise Fail("the conversation did not survive the reboot")
        uart.expect(r"^NC_READY tools=12 fallback=1$")
        cloud.message(OWNER, "tôi thích đèn gì?")
        wait_for(
            "the remembered answer after the reboot",
            lambda: sum(m["text"] == "Bạn thích đèn vàng." for m in cloud.sent) == 2,
        )
        cloud.message(OWNER, "tôi vừa nhờ gì?")
        wait_for(
            "the conversation after the reboot",
            sent(cloud, "Bạn nhờ tôi nhắn uống nước."),
        )
        cloud.message(OWNER, "lúc đầu tôi nhờ gì?")
        wait_for(
            "the summary after the reboot",
            sent(cloud, "Lúc đầu bạn nhờ tôi nhớ đèn vàng."),
        )
        handled = [r for r in cloud.llm_requests[before:] if r.get("tools")]
        if len(handled) != 3:
            raise Fail(
                f"expected 3 turns after the reboot, got {len(handled)}: old messages again?"
            )

        # The web: a search through the configured SearXNG, a public page as text, and
        # the home's own network refused — directly, and behind a redirect.
        cloud.message(OWNER, "tìm thời tiết hôm nay")
        evidence.append(uart.expect(r"^NC_TOOL name=web_search result=ok$"))
        wait_for("the search answer", sent(cloud, "Đã tìm: Thời tiết Hà Nội hôm nay"))
        if cloud.searches[-1].get("q") != ["thời tiết Hà Nội"]:
            raise Fail(f"unexpected search query: {cloud.searches[-1]}")
        cloud.message(OWNER, "đọc tin")
        evidence.append(uart.expect(r"^NC_FETCH ok chars=\d+$"))
        wait_for(
            "the page answer",
            lambda: any(m["text"].startswith("Trang viết: Bản tin") for m in cloud.sent),
        )
        page = next(m["text"] for m in cloud.sent if m["text"].startswith("Trang viết:"))
        if "steal" in page or "<" in page or "Hà Nội & mưa." not in page:
            raise Fail(f"the page was not read as text: {page!r}")
        cloud.message(OWNER, "mở trang router")
        evidence.append(uart.expect(r"^NC_FETCH refused=private host_len=12$"))
        evidence.append(uart.expect(r"^NC_TOOL name=web_fetch result=error$"))
        cloud.message(OWNER, "đọc 192.168.1.1")
        evidence.append(uart.expect(r"^NC_FETCH refused=private host_len=11$"))
        evidence.append(uart.expect(r"^NC_TOOL name=web_fetch result=error$"))

        # The fallback LLM: the configured URL is down (503); the fallback answers, and
        # while the first cools down (60 s after one failure) it is not called again.
        cloud.primary_down = True
        mark = len(cloud.llm_urls)
        lights = sum(m["text"] == "Đã bật đèn." for m in cloud.sent)
        cloud.message(OWNER, "bật đèn lần nữa")
        evidence.append(
            uart.expect(r"^NC_LLM_FALLBACK from=primary reason=transient http=503 cooldown_s=60$")
        )
        evidence.append(uart.expect(r"^NC_TOOL name=living_light_on status=ALLOW reason=-$"))
        wait_for(
            "the light reply through the fallback",
            lambda: sum(m["text"] == "Đã bật đèn." for m in cloud.sent) == lights + 1,
        )
        cloud.message(OWNER, "bật đèn thêm lần nữa")
        wait_for(
            "the second light reply",
            lambda: sum(m["text"] == "Đã bật đèn." for m in cloud.sent) == lights + 2,
        )
        urls = cloud.llm_urls[mark:]  # two turns of two rounds, and maybe a summary
        if urls[0] != "v1" or "v1" in urls[1:] or len(urls) < 5:
            raise Fail(f"unexpected LLM URLs while the first was down: {urls}")
        if "NC_LLM_FALLBACK" in uart.text()[uart.pos :]:
            raise Fail("the cooling URL was tried again")
        cloud.primary_down = False

        # The heartbeat: HEARTBEAT.md gets a task; within a minute it is checked, and
        # HEARTBEAT_OK is not sent to the owner.
        cloud.message(OWNER, "kiểm tra định kỳ cửa trước")
        evidence.append(uart.expect(r"^NC_TOOL name=write_file result=ok$"))
        count = len(cloud.sent)
        evidence.append(uart.expect(r"^NC_HEARTBEAT result=ok$", timeout=150))
        if any("HEARTBEAT_OK" in m["text"] for m in cloud.sent[count:]):
            raise Fail("HEARTBEAT_OK was sent to the owner")
    finally:
        stop(qemu)
        server.shutdown()

    summary_requests = [
        r for r in cloud.llm_requests if r["messages"][0]["content"].startswith("Summarize")
    ]
    if not summary_requests or any(r.get("tools") for r in summary_requests):
        raise Fail("no summary request, or one that offered tools")
    offered = {
        tuple(sorted(t["function"]["name"] for t in r["tools"]))
        for r in cloud.llm_requests
        if r not in summary_requests
    }
    expected = (
        "append_file",
        "cron",
        "edit_file",
        "front_door_unlock",
        "list_dir",
        "living_light_on",
        "message",
        "read_file",
        "send_file",
        "web_fetch",
        "web_search",
        "write_file",
    )
    if offered != {expected}:
        raise Fail(f"the model was offered other tools: {offered}")
    if cloud.bad_bodies:
        raise Fail(f"{cloud.bad_bodies} request(s) were not valid UTF-8 JSON")
    if cloud.bad_auth:
        raise Fail("an LLM key or model was not sent as configured")
    text = "".join(Uart(log).text() for log in logs)
    for secret in (TOKEN, TOKEN.split(":", 1)[1], KEY, FALLBACK_KEY):
        if secret in text:
            raise Fail("a secret reached the UART")
    folded = re.findall(r"^NC_SUMMARY folded=(\d+) result=ok$", text, re.MULTILINE)
    if not folded:
        raise Fail("no NC_SUMMARY result=ok on the UART")
    evidence.append(f"summaries: {len(folded)}, messages folded: {sum(map(int, folded))}")
    stack = [int(v) for v in re.findall(r"^NC_STACK free_min=(\d+)$", text, re.MULTILINE)]
    if not stack or min(stack) < STACK_FLOOR:
        raise Fail(
            f"the brain task's free stack fell to {min(stack, default=0)} B (< {STACK_FLOOR})"
        )
    evidence.append(f"brain task stack: at least {min(stack)} B never used")
    if len(re.findall(r"^NC_BUTTON ", text, re.MULTILINE)) != 2:
        raise Fail("a press other than the owner's two reached the gate")
    if len(cloud.acks) != 2:
        raise Fail(f"expected the owner's two presses acknowledged, got {len(cloud.acks)}")
    evidence.append(
        f"LLM calls: {len(cloud.llm_requests)}; messages sent: {len(cloud.sent)}; "
        f"files sent: {len(cloud.files)}"
    )
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
