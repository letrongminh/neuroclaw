# 💬 Cấu Hình Ứng Dụng Chat

> Quay lại [README](../project/README.vi.md)

## 💬 Ứng Dụng Chat

Trò chuyện với neuroclaw của bạn qua Telegram, Discord, WhatsApp, Matrix, QQ, DingTalk, LINE, WeCom, Feishu, Slack, IRC, OneBot, MQTT, MaixCam hoặc Pico (giao thức bản địa)

> **Lưu ý**: Các kênh dựa trên HTTP callback dùng chung một máy chủ HTTP Gateway (`gateway.host`:`gateway.port`, mặc định `127.0.0.1:18790`). Các kênh dựa trên socket/stream như Feishu, DingTalk và WeCom không dùng máy chủ webhook chung để nhận tin.

> **Hãy đặt `allow_from` cho mọi kênh.** Nếu `allow_from` để trống, bất kỳ ai tiếp cận được bot đều có thể trò chuyện với nó (gateway chỉ ghi cảnh báo `SECURITY: Channel allows EVERYONE`). Hãy liệt kê ID người dùng của bạn, hoặc dùng `["*"]` để nói rõ rằng cố ý cho truy cập mở. Một số ví dụ bên dưới ghi `"allow_from": []` cho ngắn gọn; đừng để trống trên bot có thể chạy công cụ. Những việc người dùng chat có thể khiến agent làm (shell, lệnh theo lịch, cài skill) được liệt kê cùng nhãn rủi ro tại [Khả năng §2](../reference/capabilities.md).

| Kênh                 | Độ khó             | Mô tả                                                 | Tài liệu                                                                                                         |
| -------------------- | ------------------ | ----------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| **Telegram**         | ⭐ Dễ              | Khuyến nghị, chuyển giọng nói thành văn bản, long polling (không cần IP công khai) | [Tài liệu](../channels/telegram/README.vi.md)                                              |
| **Discord**          | ⭐ Dễ              | Gateway WebSocket (không cần IP công khai), hỗ trợ nhóm/DM     | [Tài liệu](../channels/discord/README.vi.md)                                                                  |
| **WhatsApp**         | ⭐ Dễ              | Bản địa (quét QR) hoặc Bridge URL                     | [Tài liệu](#whatsapp)                                                                                            |
| **Weixin**           | ⭐ Dễ              | Quét QR gốc (API Tencent iLink)                       | [Tài liệu](#weixin)                                                                                              |
| **Slack**            | ⭐ Dễ              | **Socket Mode** (không cần IP công khai), doanh nghiệp | [Tài liệu](../channels/slack/README.vi.md)                                                                      |
| **Matrix**           | ⭐⭐ Trung bình    | Giao thức liên kết, hỗ trợ tự lưu trữ                | [Tài liệu](../channels/matrix/README.vi.md)                                                                     |
| **QQ**               | ⭐⭐ Trung bình    | API bot chính thức, cộng đồng Trung Quốc              | [Tài liệu](../channels/qq/README.vi.md)                                                                         |
| **DingTalk**         | ⭐⭐ Trung bình    | Chế độ Stream (không cần IP công khai), doanh nghiệp  | [Tài liệu](../channels/dingtalk/README.vi.md)                                                                   |
| **LINE**             | ⭐⭐⭐ Nâng cao    | Yêu cầu HTTPS Webhook                                 | [Tài liệu](../channels/line/README.vi.md)                                                                       |
| **WeCom (企业微信)** | ⭐⭐⭐ Nâng cao    | AI Bot chính thức qua WebSocket, streaming + media     | [Hướng dẫn](../channels/wecom/README.vi.md) |
| **Feishu (飞书)**    | ⭐⭐⭐ Nâng cao    | Cộng tác doanh nghiệp, nhiều tính năng                | [Tài liệu](../channels/feishu/README.vi.md)                                                                     |
| **IRC**              | ⭐⭐ Trung bình    | Máy chủ + cấu hình TLS                                | [Tài liệu](#irc) |
| **OneBot**           | ⭐⭐ Trung bình    | Tương thích NapCat/Go-CQHTTP, hệ sinh thái cộng đồng  | [Tài liệu](../channels/onebot/README.vi.md)                                                                     |
| **MQTT**             | ⭐ Dễ              | Bất kỳ client MQTT nào qua broker pub/sub            | [Tài liệu](../channels/mqtt/README.vi.md)                                                                    |
| **MaixCam**          | ⭐ Dễ              | Kênh tích hợp phần cứng cho camera AI Sipeed          | [Tài liệu](../channels/maixcam/README.vi.md)                                                                    |
| **Pico**             | ⭐ Dễ              | Kênh giao thức bản địa NeuroClaw                       |                                                                                                                  |

<a id="telegram"></a>
<details>
<summary><b>Telegram</b> (Khuyến nghị)</summary>

**1. Tạo bot**

* Mở Telegram, tìm `@BotFather`
* Gửi `/newbot`, làm theo hướng dẫn
* Sao chép token

**2. Cấu hình**

```json
{
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"]
    }
  }
}
```

> Lấy user ID của bạn từ `@userinfobot` trên Telegram.

**3. Chạy**

```bash
neuroclaw gateway
```

**4. Menu lệnh Telegram (tự động đăng ký khi khởi động)**

NeuroClaw lưu trữ định nghĩa lệnh trong một registry chung. Khi khởi động, Telegram sẽ tự động đăng ký các lệnh bot được hỗ trợ (ví dụ `/start`, `/help`, `/show`, `/list`, `/use`, `/btw`) để menu lệnh và hành vi runtime luôn đồng bộ.
Đăng ký menu lệnh Telegram vẫn là UX khám phá cục bộ của kênh; thực thi lệnh chung được xử lý tập trung trong vòng lặp agent qua commands executor.

Nếu đăng ký lệnh thất bại (lỗi tạm thời mạng/API), kênh vẫn khởi động và NeuroClaw thử lại đăng ký trong nền.

Bạn cũng có thể xem skill và máy chủ MCP trực tiếp từ Telegram:

- `/list skills`
- `/list mcp`
- `/show mcp <server>`
- `/use <skill> <message>`
- `/use <skill>` rồi gửi yêu cầu thật ở tin nhắn tiếp theo
- `/use clear`
- `/btw <question>` để hỏi nhanh một câu ngoài lề mà không thay đổi lịch sử phiên đang hoạt động; `/btw` được xử lý như một truy vấn không dùng công cụ và không đi vào luồng thực thi công cụ thông thường

**5. Định dạng nâng cao**
Bạn có thể đặt `use_markdown_v2: true` để bật các tùy chọn định dạng nâng cao. Khi đó bot dùng được đầy đủ tính năng Telegram MarkdownV2, gồm kiểu lồng nhau, spoiler và khối độ rộng cố định tùy chỉnh.

</details>

<a id="discord"></a>
<details>
<summary><b>Discord</b></summary>

**1. Tạo bot**

* Truy cập <https://discord.com/developers/applications>
* Tạo ứng dụng → Bot → Add Bot
* Sao chép bot token

**2. Bật intents**

* Trong cài đặt Bot, bật **MESSAGE CONTENT INTENT**
* (Tùy chọn) Bật **SERVER MEMBERS INTENT** nếu bạn muốn sử dụng danh sách cho phép dựa trên dữ liệu thành viên

**3. Lấy User ID**
* Cài đặt Discord → Nâng cao → bật **Developer Mode**
* Nhấp chuột phải vào avatar → **Copy User ID**

**4. Cấu hình**

```json
{
  "channel_list": {
    "discord": {
      "enabled": true,
      "type": "discord",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"]
    }
  }
}
```

**5. Mời bot**

* OAuth2 → URL Generator
* Scopes: `bot`
* Bot Permissions: `Send Messages`, `Read Message History`
* Mở URL mời được tạo và thêm bot vào server của bạn

**Tùy chọn: Chế độ kích hoạt nhóm**

Mặc định bot phản hồi tất cả tin nhắn trong kênh server. Để giới hạn phản hồi chỉ khi @mention, thêm:

```json
{
  "channel_list": {
    "discord": {
      "group_trigger": { "mention_only": true }
    }
  }
}
```

Bạn cũng có thể kích hoạt bằng tiền tố từ khóa (ví dụ: `!bot`):

```json
{
  "channel_list": {
    "discord": {
      "group_trigger": { "prefixes": ["!bot"] }
    }
  }
}
```

**6. Chạy**

```bash
neuroclaw gateway
```

</details>

<a id="whatsapp"></a>
<details>
<summary><b>WhatsApp</b> (native qua whatsmeow)</summary>

NeuroClaw có thể kết nối WhatsApp theo hai cách:

- **Native (khuyến nghị):** In-process sử dụng [whatsmeow](https://github.com/tulir/whatsmeow). Không cần bridge riêng. Đặt `"use_native": true` và để trống `bridge_url`. Lần chạy đầu tiên, quét mã QR bằng WhatsApp (Thiết bị liên kết). Phiên được lưu trong workspace (ví dụ: `workspace/whatsapp/`). Kênh native là **tùy chọn** để giữ binary mặc định nhỏ; build với `-tags whatsapp_native` (ví dụ: `make build-whatsapp-native` hoặc `go build -tags whatsapp_native ./cmd/...`).
- **Bridge:** Kết nối đến bridge WebSocket bên ngoài. Đặt `bridge_url` (ví dụ: `ws://localhost:3001`) và giữ `use_native` là false.

**Cấu hình (native)**

```json
{
  "channel_list": {
    "whatsapp": {
      "enabled": true,
      "type": "whatsapp",
      "use_native": true,
      "session_store_path": "",
      "allow_from": []
    }
  }
}
```

Nếu `session_store_path` trống, phiên được lưu tại `<workspace>/whatsapp/`. Chạy `neuroclaw gateway`; lần chạy đầu tiên, quét mã QR hiển thị trong terminal bằng WhatsApp → Thiết bị liên kết.

</details>

<a id="weixin"></a>
<details>
<summary><b>Weixin</b> (WeChat Cá nhân)</summary>

NeuroClaw hỗ trợ kết nối với tài khoản WeChat cá nhân của bạn thông qua API chính thức Tencent iLink.

**1. Đăng nhập**

Chạy luồng đăng nhập QR tương tác:
```bash
neuroclaw auth weixin
```
Quét mã QR được in ra bằng ứng dụng WeChat trên điện thoại. Sau khi đăng nhập thành công, token sẽ được lưu vào cấu hình.

**2. Cấu hình**

(Tùy chọn) Thêm ID người dùng WeChat vào `allow_from` để giới hạn ai có thể nhắn tin với bot:
```json
{
  "channel_list": {
    "weixin": {
      "enabled": true,
      "type": "weixin",
      "token": "YOUR_TOKEN",
      "allow_from": ["YOUR_USER_ID"]
    }
  }
}
```

**3. Chạy**
```bash
neuroclaw gateway
```

</details>

<a id="qq"></a>
<details>
<summary><b>QQ</b></summary>

**Thiết lập nhanh (khuyến nghị)**

QQ Open Platform cung cấp trang thiết lập một chạm cho bot tương thích OpenClaw:

1. Mở [QQ Bot Quick Start](https://q.qq.com/qqbot/openclaw/index.html) và quét mã QR để đăng nhập
2. Bot được tạo tự động — sao chép **App ID** và **App Secret**
3. Cấu hình NeuroClaw:

```json
{
  "channel_list": {
    "qq": {
      "enabled": true,
      "type": "qq",
      "app_id": "YOUR_APP_ID",
      "app_secret": "YOUR_APP_SECRET",
      "allow_from": []
    }
  }
}
```

4. Chạy `neuroclaw gateway` và mở QQ để trò chuyện với bot của bạn

> App Secret chỉ hiển thị một lần. Lưu ngay lập tức — xem lại sẽ buộc phải đặt lại.
>
> Bot được tạo qua trang thiết lập nhanh ban đầu chỉ dành cho người tạo và không hỗ trợ chat nhóm. Để bật quyền truy cập nhóm, cấu hình chế độ sandbox trên [QQ Open Platform](https://q.qq.com/).

**Thiết lập thủ công**

Nếu bạn muốn tạo bot thủ công:

* Đăng nhập tại [QQ Open Platform](https://q.qq.com/) để đăng ký làm nhà phát triển
* Tạo bot QQ — tùy chỉnh avatar và tên
* Sao chép **App ID** và **App Secret** từ cài đặt bot
* Cấu hình như trên và chạy `neuroclaw gateway`

</details>

<a id="dingtalk"></a>
<details>
<summary><b>DingTalk</b></summary>

**1. Tạo bot**

* Truy cập [Open Platform](https://open.dingtalk.com/)
* Tạo ứng dụng nội bộ
* Sao chép Client ID và Client Secret

**2. Cấu hình**

```json
{
  "channel_list": {
    "dingtalk": {
      "enabled": true,
      "type": "dingtalk",
      "client_id": "YOUR_CLIENT_ID",
      "client_secret": "YOUR_CLIENT_SECRET",
      "allow_from": []
    }
  }
}
```

> Chỉ định DingTalk user ID trong `allow_from` để giới hạn truy cập. Để trống là cho phép tất cả; dùng `["*"]` để nói rõ điều đó.

**3. Chạy**

```bash
neuroclaw gateway
```

</details>

<a id="matrix"></a>
<details>
<summary><b>Matrix</b></summary>

**1. Chuẩn bị tài khoản bot**

* Sử dụng homeserver ưa thích (ví dụ: `https://matrix.org` hoặc tự host)
* Tạo user bot và lấy access token

**2. Cấu hình**

```json
{
  "channel_list": {
    "matrix": {
      "enabled": true,
      "type": "matrix",
      "homeserver": "https://matrix.org",
      "user_id": "@your-bot:matrix.org",
      "access_token": "YOUR_MATRIX_ACCESS_TOKEN",
      "allow_from": []
    }
  }
}
```

**3. Chạy**

```bash
neuroclaw gateway
```

Để xem đầy đủ các tùy chọn (`device_id`, `join_on_invite`, `group_trigger`, `placeholder`, `reasoning_channel_id`), xem [Hướng Dẫn Cấu Hình Kênh Matrix](../channels/matrix/README.md).

</details>

<a id="line"></a>
<details>
<summary><b>LINE</b></summary>

**1. Tạo Tài Khoản LINE Official**

- Truy cập [LINE Developers Console](https://developers.line.biz/)
- Tạo provider → Tạo kênh Messaging API
- Sao chép **Channel Secret** và **Channel Access Token**

**2. Cấu hình**

```json
{
  "channel_list": {
    "line": {
      "enabled": true,
      "type": "line",
      "channel_secret": "YOUR_CHANNEL_SECRET",
      "channel_access_token": "YOUR_CHANNEL_ACCESS_TOKEN",
      "webhook_path": "/webhook/line",
      "allow_from": []
    }
  }
}
```

> Webhook LINE được phục vụ trên máy chủ Gateway chung (`gateway.host`:`gateway.port`, mặc định `127.0.0.1:18790`).

**3. Thiết lập Webhook URL**

LINE yêu cầu HTTPS cho webhook. Sử dụng reverse proxy hoặc tunnel:

```bash
# Ví dụ với ngrok (port mặc định gateway là 18790)
ngrok http 18790
```

Sau đó đặt Webhook URL trong LINE Developers Console thành `https://your-domain/webhook/line` và bật **Use webhook**.

**4. Chạy**

```bash
neuroclaw gateway
```

> Trong chat nhóm, bot chỉ phản hồi khi được @mention. Phản hồi trích dẫn tin nhắn gốc.

</details>

<a id="wecom"></a>
<details>
<summary><b>WeCom (企业微信)</b></summary>

NeuroClaw cung cấp WeCom dưới dạng một kênh AI Bot duy nhất qua WebSocket.
Không cần URL callback webhook công khai.

Xem [Hướng Dẫn Cấu Hình WeCom](../channels/wecom/README.vi.md) để biết đầy đủ tham chiếu cấu hình và ghi chú di chuyển.

**Thiết Lập Nhanh - Khuyến nghị**

**1. Xác thực**

```bash
neuroclaw auth wecom
```

Lệnh này hiển thị mã QR, chờ phê duyệt trong WeCom và ghi `bot_id` + `secret` vào mục `wecom` của `channel_list`.

**2. Cấu hình thủ công nếu cần**

```json
{
  "channel_list": {
    "wecom": {
      "enabled": true,
      "type": "wecom",
      "bot_id": "YOUR_BOT_ID",
      "secret": "YOUR_SECRET",
      "websocket_url": "wss://openws.work.weixin.qq.com",
      "send_thinking_message": true,
      "allow_from": [],
      "reasoning_channel_id": ""
    }
  }
}
```

**3. Chạy**

```bash
neuroclaw gateway
```

> Các mục cũ `wecom_app` và `wecom_aibot` được thay bằng mục `wecom` thống nhất trong `channel_list`.

</details>

<a id="feishu"></a>
<details>
<summary><b>Feishu (Lark)</b></summary>

NeuroClaw kết nối với Feishu qua chế độ WebSocket/SDK — không cần URL webhook công khai hay máy chủ callback.

**1. Tạo ứng dụng**

* Truy cập [Feishu Open Platform](https://open.feishu.cn/) và tạo ứng dụng
* Trong cài đặt ứng dụng, bật khả năng **Bot**
* Tạo phiên bản và xuất bản ứng dụng (ứng dụng phải được xuất bản mới có hiệu lực)
* Sao chép **App ID** (bắt đầu bằng `cli_`) và **App Secret**

**2. Cấu hình**

```json
{
  "channel_list": {
    "feishu": {
      "enabled": true,
      "type": "feishu",
      "app_id": "cli_xxx",
      "app_secret": "YOUR_APP_SECRET",
      "allow_from": []
    }
  }
}
```

Tùy chọn: `encrypt_key` và `verification_token` để mã hóa sự kiện (khuyến nghị cho môi trường production).

**3. Chạy và trò chuyện**

```bash
neuroclaw gateway
```

Mở Feishu, tìm tên bot của bạn và bắt đầu trò chuyện. Bạn cũng có thể thêm bot vào nhóm — sử dụng `group_trigger.mention_only: true` để chỉ phản hồi khi được @mention.

Để xem đầy đủ các tùy chọn, xem [Hướng Dẫn Cấu Hình Kênh Feishu](../channels/feishu/README.vi.md).

</details>

<a id="slack"></a>
<details>
<summary><b>Slack</b></summary>

**1. Tạo ứng dụng Slack**

* Truy cập [Slack API](https://api.slack.com/apps) và tạo ứng dụng mới
* Trong **OAuth & Permissions**, thêm các scope bot: `chat:write`, `app_mentions:read`, `im:history`, `im:read`, `im:write`
* Cài đặt ứng dụng vào workspace của bạn
* Sao chép **Bot Token** (`xoxb-...`) và **App-Level Token** (`xapp-...`, bật Socket Mode để lấy token này)

**2. Cấu hình**

```json
{
  "channel_list": {
    "slack": {
      "enabled": true,
      "type": "slack",
      "bot_token": "xoxb-YOUR-BOT-TOKEN",
      "app_token": "xapp-YOUR-APP-TOKEN",
      "allow_from": []
    }
  }
}
```

**3. Chạy**

```bash
neuroclaw gateway
```

</details>

<a id="irc"></a>
<details>
<summary><b>IRC</b></summary>

**1. Cấu hình**

```json
{
  "channel_list": {
    "irc": {
      "enabled": true,
      "type": "irc",
      "server": "irc.libera.chat:6697",
      "tls": true,
      "nick": "neuroclaw-bot",
      "channels": ["#your-channel"],
      "password": "",
      "allow_from": []
    }
  }
}
```

Tùy chọn: `nickserv_password` để xác thực NickServ, `sasl_user`/`sasl_password` để xác thực SASL.

**2. Chạy**

```bash
neuroclaw gateway
```

Bot sẽ kết nối đến máy chủ IRC và tham gia các kênh đã chỉ định.

</details>

<a id="onebot"></a>
<details>
<summary><b>OneBot (QQ qua giao thức OneBot)</b></summary>

OneBot là giao thức mở cho bot QQ. NeuroClaw kết nối với bất kỳ triển khai tương thích OneBot v11 nào (ví dụ: [Lagrange](https://github.com/LagrangeDev/Lagrange.Core), [NapCat](https://github.com/NapNeko/NapCatQQ)) qua WebSocket.

**1. Thiết lập triển khai OneBot**

Cài đặt và chạy framework bot QQ tương thích OneBot v11. Bật máy chủ WebSocket của nó.

**2. Cấu hình**

```json
{
  "channel_list": {
    "onebot": {
      "enabled": true,
      "type": "onebot",
      "ws_url": "ws://127.0.0.1:8080",
      "access_token": "",
      "allow_from": []
    }
  }
}
```

| Trường | Mô tả |
|--------|-------|
| `ws_url` | URL WebSocket của triển khai OneBot |
| `access_token` | Token truy cập để xác thực (nếu đã cấu hình trong OneBot) |
| `reconnect_interval` | Khoảng thời gian kết nối lại tính bằng giây (mặc định: 5) |

**3. Chạy**

```bash
neuroclaw gateway
```

</details>

<a id="mqtt"></a>
<details>
<summary><b>MQTT</b></summary>

Bất kỳ client MQTT nào đều có thể giao tiếp với NeuroClaw qua broker. Thiết bị hoặc dịch vụ publish yêu cầu lên broker; NeuroClaw subscribe, xử lý và publish phản hồi trở lại.

**1. Cấu hình**

```json
{
  "channel_list": {
    "mqtt": {
      "enabled": true,
      "type": "mqtt",
      "settings": {
        "broker": "ssl://your-broker:8883",
        "agent_id": "assistant",
        "topic_prefix": "/neuroclaw",
        "keep_alive": 60,
        "qos": 0
      }
    }
  }
}
```

Tên người dùng và mật khẩu trong `~/.neuroclaw/.security.yml`:

```yaml
channel_list:
  mqtt:
    settings:
      username: ten_nguoi_dung
      password: mat_khau
```

**Định dạng topic**

```
{prefix}/{agent_id}/{client_id}/request    # Client → NeuroClaw
{prefix}/{agent_id}/{client_id}/response   # NeuroClaw → Client
```

`client_id` do ứng dụng client đặt để phân biệt thiết bị hoặc phiên.

**2. Khởi động**

```bash
neuroclaw gateway
```

**3. Kiểm tra**

```bash
mosquitto_pub -t "/neuroclaw/assistant/device1/request" \
  -m '{"text": "Xin chào"}'

mosquitto_sub -t "/neuroclaw/assistant/device1/response"
```

Xem đầy đủ tùy chọn cấu hình tại [Tài liệu Kênh MQTT](../channels/mqtt/README.vi.md).

</details>
