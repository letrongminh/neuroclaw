# ⚙️ Hướng Dẫn Cấu Hình

> Quay lại [README](../project/README.vi.md)

## ⚙️ Cấu Hình

File cấu hình: `~/.neuroclaw/config.json`

> **Cấu hình bảo mật:** Để lưu khóa API, token và dữ liệu nhạy cảm khác, xem [Hướng dẫn cấu hình bảo mật](../security/security_configuration.md).

### Biến Môi Trường

Bạn có thể ghi đè các đường dẫn mặc định bằng biến môi trường. Điều này hữu ích cho cài đặt portable, triển khai container, hoặc chạy neuroclaw như dịch vụ hệ thống. Các biến này độc lập và kiểm soát các đường dẫn khác nhau.

| Biến              | Mô tả                                                                                                                             | Đường Dẫn Mặc Định       |
|-------------------|-----------------------------------------------------------------------------------------------------------------------------------------|---------------------------|
| `NEUROCLAW_CONFIG` | Ghi đè đường dẫn đến file cấu hình. Chỉ định trực tiếp cho neuroclaw file `config.json` nào cần tải, bỏ qua tất cả vị trí khác. | `~/.neuroclaw/config.json` |
| `NEUROCLAW_HOME`   | Ghi đè thư mục gốc cho dữ liệu neuroclaw. Thay đổi vị trí mặc định của `workspace` và các thư mục dữ liệu khác.          | `~/.neuroclaw`             |

**Ví dụ:**

```bash
# Chạy neuroclaw với file cấu hình cụ thể
# Đường dẫn workspace sẽ được đọc từ trong file cấu hình đó
NEUROCLAW_CONFIG=/etc/neuroclaw/production.json neuroclaw gateway

# Chạy neuroclaw với tất cả dữ liệu lưu tại /opt/neuroclaw
# Cấu hình sẽ được tải từ mặc định ~/.neuroclaw/config.json
# Workspace sẽ được tạo tại /opt/neuroclaw/workspace
NEUROCLAW_HOME=/opt/neuroclaw neuroclaw agent

# Sử dụng cả hai cho thiết lập tùy chỉnh hoàn toàn
NEUROCLAW_HOME=/srv/neuroclaw NEUROCLAW_CONFIG=/srv/neuroclaw/main.json neuroclaw gateway
```

### Mức Log của Gateway

`gateway.log_level` kiểm soát mức độ chi tiết của log Gateway, có thể cấu hình trong `config.json`:

```json
{
  "gateway": {
    "log_level": "warn"
  }
}
```

Giá trị mặc định là `warn`. Các giá trị được hỗ trợ: `debug`, `info`, `warn`, `error`, `fatal`.

Cũng có thể ghi đè bằng biến môi trường: `NEUROCLAW_LOG_LEVEL=info`

### Bố Cục Workspace

NeuroClaw lưu trữ dữ liệu trong workspace đã cấu hình (mặc định: `~/.neuroclaw/workspace`):

```
~/.neuroclaw/workspace/
├── sessions/          # Phiên hội thoại và lịch sử
├── memory/           # Bộ nhớ dài hạn (MEMORY.md)
├── state/            # Trạng thái bền vững (kênh cuối, v.v.)
├── cron/             # Cơ sở dữ liệu tác vụ lên lịch
├── skills/           # Skill tùy chỉnh
├── AGENT.md          # Hướng dẫn hành vi agent
├── HEARTBEAT.md      # Prompt tác vụ định kỳ (kiểm tra mỗi 30 phút)
├── IDENTITY.md       # Danh tính agent
├── SOUL.md           # Linh hồn agent
└── USER.md           # Tùy chọn người dùng
```

> **Lưu ý:** Các thay đổi đối với `AGENT.md`, `SOUL.md`, `USER.md` và `memory/MEMORY.md` được tự động phát hiện trong thời gian chạy thông qua theo dõi thời gian sửa đổi file (mtime). **Không cần khởi động lại gateway** sau khi chỉnh sửa các file này — agent sẽ tải nội dung mới vào yêu cầu tiếp theo.

### Tự Tiến Hóa Của Agent

Khối `evolution` điều khiển runtime tự tiến hóa của NeuroClaw. Khi bật, agent ghi lại các turn đã hoàn thành thành bản ghi học tập. Ở các chế độ cao hơn, nó có thể gom các mẫu thành công lặp lại, tạo bản nháp skill và tùy chọn áp dụng các bản nháp đã chấp nhận vào skill của workspace.

```json
{
  "evolution": {
    "enabled": false,
    "mode": "observe",
    "state_dir": "",
    "min_task_count": 2,
    "min_success_ratio": 0.7,
    "cold_path_trigger": "after_turn",
    "cold_path_times": []
  }
}
```

| Trường | Mặc định | Mô tả |
|--------|----------|-------|
| `enabled` | `false` | Bật việc ghi bản ghi học tập cho các turn agent đã hoàn thành. Turn heartbeat bị bỏ qua. |
| `mode` | `observe` | `observe` chỉ ghi dữ liệu. `draft` có thể tạo bản nháp skill ứng viên. `apply` có thể áp dụng bản nháp đã chấp nhận vào skill của workspace. |
| `state_dir` | `""` | Thư mục tùy chọn cho trạng thái evolution. Để trống để dùng mặc định trong workspace. |
| `min_task_count` | `2` | Số bản ghi tác vụ liên quan tối thiểu để một mẫu đủ điều kiện tạo bản nháp. |
| `min_success_ratio` | `0.7` | Tỷ lệ thành công tối thiểu của một cụm tác vụ. Dùng giá trị lớn hơn `0` và tối đa `1`. |
| `cold_path_trigger` | `after_turn` | Chạy việc tạo bản nháp `after_turn`, theo lịch `scheduled`, hoặc tắt các lần chạy cold-path tự động khi đặt `manual`. Hiện chưa có cách kích hoạt thủ công dành cho người dùng. Chỉ áp dụng ở chế độ `draft` và `apply`. |
| `cold_path_times` | `[]` | Giờ chạy theo lịch khi `cold_path_trigger` là `scheduled`, viết dạng chuỗi `HH:MM`. |

Hãy dùng `observe` trước nếu muốn xem các bản ghi học tập mà không tạo thay đổi skill. Dùng `draft` khi muốn NeuroClaw chuẩn bị các cải tiến để bạn xem xét. Chỉ dùng `apply` khi bạn thoải mái để các bản nháp đã chấp nhận cập nhật skill của workspace.

### Chính sách ngữ cảnh request

`turn_profile` là chính sách tùy chọn trong `agents.defaults.turn_profile` để kiểm soát ngữ cảnh mỗi turn mới mang theo: lịch sử, system prompt, prompt skills và các tool được phép gọi. Nếu không cấu hình, hoặc đặt `"enabled": false`, NeuroClaw giữ nguyên hành vi mặc định. Khi đặt `"enabled": true`, chính sách bên dưới áp dụng cho mỗi turn mới.

Mỗi block dùng chung các giá trị `mode`:

| Mode | Ý nghĩa |
| --- | --- |
| `default` | Giữ hành vi bình thường của NeuroClaw. Block bị thiếu hoặc thiếu `mode` đều được xem là `default`. |
| `off` | Tắt block đó cho turn. |
| `custom` | Dùng danh sách cho phép. Phiên bản này chỉ hỗ trợ `custom` cho `skills` và `tools`; dùng cho `history` hoặc `system_prompt` sẽ lỗi validate. |

Các block:

| Block | Nội dung kiểm soát |
| --- | --- |
| `history` | Đọc lịch sử và tóm tắt, ghi tin nhắn user/assistant/tool, nạp context, compact và summarize. |
| `system_prompt` | Chèn identity mặc định của NeuroClaw, chỉ dẫn workspace, memory, runtime context và summary. System prompt từ request bên ngoài vẫn được dùng khi block này `off`. |
| `skills` | Catalog skills và nội dung active skill trong prompt. `custom.allow` chỉ giữ các tên skill được liệt kê. |
| `tools` | Công cụ hiển thị cho model và được phép thực thi. `custom.allow` chỉ giữ các tool đã đăng ký và được liệt kê. |

Khi `system_prompt.mode` là `off`, tools vẫn hiển thị và không có system prompt bên ngoài, NeuroClaw dùng lại quy tắc dùng tool hiện có làm prompt fallback tối thiểu. Nếu `tools.mode` là `off`, fallback này không được thêm.

Ví dụ ngữ cảnh sạch chỉ giữ tool web:

```json
{
  "agents": {
    "defaults": {
      "turn_profile": {
        "enabled": true,
        "history": { "mode": "off" },
        "system_prompt": { "mode": "off" },
        "skills": { "mode": "off" },
        "tools": {
          "mode": "custom",
          "allow": ["web_search", "web_fetch"]
        }
      }
    }
  }
}
```

### Web launcher dashboard

**neuroclaw-launcher** phục vụ một giao diện trình duyệt yêu cầu đăng nhập bằng mật khẩu trước. Ở lần chạy đầu, mở `/launcher-setup` để tạo mật khẩu dashboard. Các lần đăng nhập thủ công sau đó dùng `/launcher-login`.

- **File cấu hình**: Cùng thư mục với `config.json` (hoặc file mà `NEUROCLAW_CONFIG` trỏ tới). File riêng của launcher là `launcher-config.json`.
- **Lưu mật khẩu**: Trên các nền tảng được hỗ trợ, mật khẩu được lưu dưới dạng hash bcrypt trong `launcher-auth.db`. Trên nền tảng không có kho mật khẩu SQLite, hash bcrypt được lưu trong `launcher-config.json`.
- **Di chuyển cũ**: Các giá trị `launcher_token` cũ được chuyển một lần sang đăng nhập bằng mật khẩu và bị xóa khỏi cấu hình launcher đã lưu.
- **Tự đăng nhập cục bộ**: Khi launcher tự mở trình duyệt cục bộ sau khi khởi động, nó dùng một endpoint bootstrap dùng một lần chỉ qua loopback để tự đặt cookie phiên.
- **Cách xác thực không còn hỗ trợ**: Đăng nhập bằng token trên URL (`?token=...`), `NEUROCLAW_LAUNCHER_TOKEN` và xác thực dashboard bằng `Authorization: Bearer` không còn được hỗ trợ.
- **Đăng xuất**: Dùng **`POST /api/auth/logout`** với **`Content-Type: application/json`** (body có thể là `{}`). Đừng dựa vào URL GET để đăng xuất (mẫu an toàn trước CSRF).
- **Chống dò mật khẩu**: **`POST /api/auth/login`** bị **giới hạn tốc độ theo IP client mỗi phút** (HTTP 429 khi vượt).
- **Thời hạn phiên**: Cookie phiên HttpOnly mặc định kéo dài khoảng **31 ngày**, nhưng phiên bị vô hiệu khi tiến trình launcher khởi động lại.

### Nguồn Skill

Mặc định, skill được tải từ:

1. `~/.neuroclaw/workspace/skills` (workspace)
2. `~/.neuroclaw/skills` (global)
3. `<đường-dẫn-nhúng-khi-build>/skills` (tích hợp)

Cho thiết lập nâng cao/test, bạn có thể ghi đè thư mục gốc skill builtin với:

```bash
export NEUROCLAW_BUILTIN_SKILLS=/path/to/skills
```

### Dùng Skill Từ Các Kênh Chat

Sau khi cài skill và cấu hình máy chủ MCP, bạn có thể xem và ép dùng chúng trực tiếp từ một kênh chat:

- `/list skills` hiện tên các skill đã cài mà agent hiện tại có thể dùng.
- `/list mcp` hiện các máy chủ MCP đã cấu hình cùng trạng thái enabled/deferred/connected.
- `/show mcp <server>` hiện các công cụ đang được một máy chủ MCP đã kết nối cung cấp.
- `/use <skill> <message>` ép dùng một skill cho duy nhất một yêu cầu.
- `/use <skill>` đặt sẵn skill đó cho tin nhắn tiếp theo trong cùng phiên chat.
- `/use clear` hủy skill override đang chờ được tạo bởi `/use <skill>`.
- `/btw <question>` đặt câu hỏi phụ ngay lập tức mà không thay đổi lịch sử phiên hiện tại. `/btw` được xử lý như một truy vấn không dùng công cụ và không đi vào luồng thực thi công cụ thông thường.

Ví dụ:

```text
/list skills
/list mcp
/show mcp github
/use git giải thích cách squash 3 commit cuối
/btw nhắc lại giúp tôi chúng ta đã chốt gì cho kế hoạch deploy
/use italiapersonalfinance
dammi le ultime news
```

### Chính Sách Thực Thi Lệnh Thống Nhất

- Lệnh slash chung được thực thi qua một đường dẫn duy nhất trong `pkg/agent/loop.go` qua `commands.Executor`.
- Adapter kênh không còn xử lý lệnh chung cục bộ; chúng chuyển tiếp văn bản đầu vào đến đường dẫn bus/agent. Telegram vẫn tự động đăng ký khi khởi động các lệnh được hỗ trợ như `/start`, `/help`, `/show`, `/list`, `/use` và `/btw`.
- Lệnh slash không xác định (ví dụ `/foo`) được chuyển sang xử lý LLM bình thường.
- Lệnh đã đăng ký nhưng không được hỗ trợ trên kênh hiện tại (ví dụ `/show` trên WhatsApp) trả về lỗi rõ ràng cho người dùng và dừng xử lý tiếp.

### Cô Lập Phiên

Phạm vi phiên kiểm soát lượng bộ nhớ được chia sẻ giữa các chat, người dùng, thread và không gian.

- Dùng `session.dimensions` cho mặc định toàn cục.
- Dùng `session_dimensions` trên một quy tắc dispatch cho một ngoại lệ được định tuyến.

Để xem các công thức từng bước và mẫu cô lập, xem [Hướng dẫn phiên](session-guide.md).

### Định Tuyến

Định tuyến được cấu hình qua `agents.dispatch.rules`.

Mỗi quy tắc khớp với ngữ cảnh đầu vào đã chuẩn hóa do các kênh tạo ra.
Các quy tắc được đánh giá từ trên xuống dưới. Quy tắc khớp đầu tiên thắng. Nếu không
quy tắc nào khớp, NeuroClaw quay về agent mặc định đã cấu hình.

Các trường khớp được hỗ trợ:

* `channel`
* `account`
* `space`
* `chat`
* `topic`
* `sender`
* `mentioned`

Giá trị khớp dùng cùng bộ từ vựng phạm vi với hệ thống phiên:

* `space`: `workspace:t001`, `guild:123456`
* `chat`: `direct:user123`, `group:-100123`, `channel:c123`
* `topic`: `topic:42`
* `sender`: mã định danh người gửi đã chuẩn hóa của nền tảng

Quy tắc có thể tùy chọn ghi đè giá trị `session.dimensions` toàn cục qua
`session_dimensions`. Nhờ đó định tuyến và phân bổ phiên luôn khớp nhau
mà không cần đưa lại các định dạng `bindings` hay `dm_scope` cũ.

Ví dụ:

```json
{
  "agents": {
    "list": [
      { "id": "main", "default": true },
      { "id": "support" },
      { "id": "sales" }
    ],
    "dispatch": {
      "rules": [
        {
          "name": "vip in support group",
          "agent": "sales",
          "when": {
            "channel": "telegram",
            "chat": "group:-1001234567890",
            "sender": "12345"
          },
          "session_dimensions": ["chat", "sender"]
        },
        {
          "name": "telegram support group",
          "agent": "support",
          "when": {
            "channel": "telegram",
            "chat": "group:-1001234567890"
          },
          "session_dimensions": ["chat"]
        }
      ]
    }
  },
  "session": {
    "dimensions": ["chat"]
  }
}
```

Trong ví dụ trên, quy tắc VIP phải xuất hiện trước quy tắc nhóm rộng hơn.
Vì định tuyến được sắp thứ tự nghiêm ngặt, các quy tắc cụ thể hơn nên đặt
trước và các quy tắc dự phòng rộng hơn đặt sau.

Để xem thêm ví dụ định tuyến và tầng mô hình đầy đủ hơn, xem [Hướng dẫn định tuyến](routing-guide.md).

### Allowlist Công Cụ Của Agent

Khai báo công cụ cho từng agent nằm trong frontmatter của `AGENT.md`, không nằm trong `config.json`.

Nếu frontmatter bỏ qua `tools`, agent nhận bộ công cụ được bật toàn cục như bình thường. Nếu có `tools`, NeuroClaw chỉ đăng ký các công cụ runtime được liệt kê cho agent đó.

```md
---
name: Research Agent
description: Specialist for web research and in-depth analysis.
tools: [read_file, write_file, web_search, web_fetch, message]
skills: [deep-research]
mcpServers: [web-index]
---

You are the research agent.
```

Ghi chú:

- Đây là allowlist, không phải gợi ý ưu tiên.
- Tên công cụ được so khớp 1:1 với tên công cụ runtime.
- Dùng tên công cụ runtime như `web_search`, `web_fetch`, `spawn`, `subagent`, `send_file`.
- Khai báo công cụ trong `AGENT.md` được runtime/tooling dùng, nhưng không được chèn vào prompt khám phá.

### Khám Phá Agent (Tự Động)

Khi một agent có các peer có thể spawn và gọi được `spawn`, NeuroClaw chèn một registry agent có cấu trúc vào system prompt của agent đó ở mỗi turn. Không cần gọi thêm công cụ `list_agents`.

Registry này giúp việc ủy quyền cụ thể và đáng tin cậy, đặc biệt khi dùng `spawn` với `agent_id` đích.

Mỗi mục gồm:

| Trường | Ý nghĩa |
|--------|---------|
| `id` | Id agent ổn định |
| `name` | Tên danh tính agent từ frontmatter `AGENT.md` |
| `description` | Mô tả danh tính agent từ frontmatter `AGENT.md` |

Hành vi quan trọng:

- Phần khám phá chỉ xuất hiện khi agent hiện tại có công cụ `spawn` và chỉ gồm các agent peer mà nó được phép spawn qua `subagents.allow_agents`.
- Agent hiện tại và các peer không spawn được bị bỏ qua, nên mô hình không lập kế hoạch dựa trên agent không dùng được.
- Việc khám phá cố ý gọn nhẹ. Nó chỉ cho mô hình danh tính cần để chọn peer: `id`, `name` và `description`.
- `config.json` vẫn là tầng hạ tầng: workspace, chọn agent mặc định, định tuyến và quyền của subagent. Các quyền đó cũng quyết định khả năng hiển thị khi khám phá.
- `AGENT.md` vẫn là tầng danh tính. Mã runtime/công cụ vẫn có thể dùng các trường `tools`, `skills`, `mcpServers` và `model` khi ủy quyền diễn ra.

Ví dụ dạng được chèn:

```json
{
  "agents": [
    {
      "id": "research",
      "name": "Research Agent",
      "description": "Specialist for long-form investigation and web work."
    }
  ]
}
```

Trên thực tế, một agent đa năng có thể chọn peer dựa trên mô tả vai trò, rồi gọi `spawn` với `agent_id` của peer. Runtime xử lý phần còn lại.

### 🔒 Sandbox Bảo Mật

NeuroClaw chạy trong môi trường sandbox mặc định. Agent chỉ có thể truy cập file và thực thi lệnh trong workspace đã cấu hình.

#### Cấu Hình Mặc Định

```json
{
  "agents": {
    "defaults": {
      "workspace": "~/.neuroclaw/workspace",
      "restrict_to_workspace": true
    }
  }
}
```

| Tùy chọn                | Mặc định                | Mô tả                                    |
| ----------------------- | ----------------------- | ----------------------------------------- |
| `workspace`             | `~/.neuroclaw/workspace` | Thư mục làm việc của agent               |
| `restrict_to_workspace` | `true`                  | Giới hạn truy cập file/lệnh trong workspace |

#### Công Cụ Được Bảo Vệ

Khi `restrict_to_workspace: true`, các công cụ sau được sandbox:

| Công cụ       | Chức năng        | Giới hạn                               |
| ------------- | ---------------- | -------------------------------------- |
| `read_file`   | Đọc file         | Chỉ file trong workspace              |
| `write_file`  | Ghi file         | Chỉ file trong workspace              |
| `list_dir`    | Liệt kê thư mục | Chỉ thư mục trong workspace           |
| `edit_file`   | Sửa file         | Chỉ file trong workspace              |
| `append_file` | Nối vào file     | Chỉ file trong workspace              |
| `exec`        | Thực thi lệnh   | Đường dẫn lệnh phải trong workspace   |

#### Bảo Vệ Exec Bổ Sung

Ngay cả khi `restrict_to_workspace: false`, công cụ `exec` chặn các lệnh nguy hiểm sau:

* `rm -rf`, `del /f`, `rmdir /s` — Xóa hàng loạt
* `format`, `mkfs`, `diskpart` — Định dạng đĩa
* `dd if=` — Tạo ảnh đĩa
* Ghi vào `/dev/sd[a-z]` — Ghi trực tiếp đĩa
* `shutdown`, `reboot`, `poweroff` — Tắt hệ thống
* Fork bomb `:(){ :|:& };:`
* Ngoài ra: `sudo`, `chmod`/`chown`, `kill`/`pkill`, `curl ... | sh`, `git push`, `docker run`, cài đặt gói và command substitution (`$(...)`, dấu backtick). Danh sách đầy đủ là `defaultDenyPatterns` trong `pkg/tools/shell.go`.

### Kiểm Soát Truy Cập File

| Config Key | Type | Default | Description |
|------------|------|---------|-------------|
| `tools.allow_read_paths` | string[] | `[]` | Additional paths allowed for reading outside workspace |
| `tools.allow_write_paths` | string[] | `[]` | Additional paths allowed for writing outside workspace |
| `tools.message.media_enabled` | bool | `false` | Cho phép công cụ `message` đính kèm tệp media cục bộ theo đường dẫn. Tách biệt với `tools.send_file.enabled`; chỉ bật khi muốn gửi thống nhất văn bản/media/chú thích. |

### Chế Độ Read File

`read_file` có hai cách triển khai loại trừ nhau, chọn bằng cấu hình. NeuroClaw chỉ đăng ký đúng một trong hai khi khởi động:

| Config Key | Type | Default | Description |
|------------|------|---------|-------------|
| `tools.read_file.enabled` | bool | `true` | Bật công cụ `read_file` |
| `tools.read_file.mode` | string | `bytes` | Chọn cách triển khai `read_file`: `bytes` hoặc `lines` |
| `tools.read_file.max_read_file_size` | int | `65536` | Số byte tối đa `read_file` trả về |

#### Chế độ: `bytes`

Tối ưu cho file bất kỳ và phân trang an toàn với dữ liệu nhị phân.

Tham số:

* `path` (bắt buộc): Đường dẫn file
* `offset` (tùy chọn): Offset byte bắt đầu, mặc định `0`
* `length` (tùy chọn): Số byte tối đa cần đọc, mặc định `max_read_file_size`

Dùng `bytes` khi:

* Bạn có thể đọc file nhị phân
* Bạn muốn phân trang theo khoảng byte mang tính xác định

#### Chế độ: `lines`

Hành vi hướng văn bản, tối ưu cho mã nguồn, markdown, log và file cấu hình. Công cụ đọc tuần tự theo dòng và dừng khi đạt ngân sách byte đã cấu hình.

Tham số:

* `path` (bắt buộc): Đường dẫn file
* `start_line` (tùy chọn): Số dòng bắt đầu, đánh số từ 1 và bao gồm dòng đó, mặc định `1`
* `max_lines` (tùy chọn): Số dòng tối đa cần đọc, mặc định = tất cả dòng còn lại đến hết file hoặc hết ngân sách byte

Ghi chú hành vi:

* File trông như nhị phân bị từ chối kèm hướng dẫn chuyển `read_file` sang `mode = bytes`
* Dòng đơn cực dài bị cắt ngắn thay vì bị bỏ qua

Dùng `mode = lines` khi:

* Agent chủ yếu đọc file văn bản
* Bạn muốn phân trang theo dòng trong prompt và lệnh gọi công cụ
* Bạn muốn các đoạn sạch hơn cho review mã, log và tài liệu

#### Ví dụ

```json
{
  "tools": {
    "read_file": {
      "enabled": true,
      "mode": "lines",
      "max_read_file_size": 65536
    }
  }
}
```

### Bảo Mật Exec

| Config Key | Type | Default | Description |
|------------|------|---------|-------------|
| `tools.exec.allow_remote` | bool | `true` | Cho phép công cụ exec từ các kênh từ xa (Telegram/Discord, v.v.). Đặt `false` để giới hạn ở các kênh nội bộ |
| `tools.exec.enable_deny_patterns` | bool | `true` | Enable dangerous command interception |
| `tools.exec.custom_deny_patterns` | string[] | `[]` | Custom regex patterns to block |
| `tools.exec.custom_allow_patterns` | string[] | `[]` | Custom regex patterns to allow |

> **Nhãn rủi ro:** `exec` và lệnh shell của cron được bật mặc định và mỗi thứ được nêu tên khi khởi động (`NeuroClaw risk=shell`, `NeuroClaw risk=cron_shell`). Cài đặt skill và các công cụ bus phần cứng cũng có nhãn. Xem [Khả năng §2](../reference/capabilities.md) để biết danh sách, mỗi nhãn cho phép mô hình làm gì và cách tắt. Hãy dùng `exec` cùng `allow_from` không rỗng trên mọi kênh ([Ứng dụng chat](chat-apps.vi.md)).

> **Lưu ý Bảo Mật:** Bảo vệ symlink được bật mặc định — tất cả đường dẫn file được giải quyết qua `filepath.EvalSymlinks` trước khi so khớp whitelist, ngăn chặn tấn công thoát qua symlink.

#### Hạn Chế Đã Biết: Tiến Trình Con Từ Công Cụ Build

Guard bảo mật exec chỉ kiểm tra dòng lệnh mà NeuroClaw khởi chạy trực tiếp. Nó không kiểm tra đệ quy các tiến trình con được tạo bởi công cụ phát triển được phép như `make`, `go run`, `cargo`, `npm run`, hoặc script build tùy chỉnh.

Điều này có nghĩa là lệnh cấp cao nhất vẫn có thể biên dịch hoặc khởi chạy binary khác sau khi vượt qua kiểm tra guard ban đầu. Trong thực tế, hãy coi script build, Makefile, script package, và binary được tạo như mã thực thi cần cùng mức độ review như lệnh shell trực tiếp.

Cho môi trường rủi ro cao hơn:

* Review script build trước khi thực thi.
* Ưu tiên phê duyệt/review thủ công cho quy trình biên dịch và chạy.
* Chạy NeuroClaw trong container hoặc VM nếu bạn cần cách ly mạnh hơn guard tích hợp.

#### Ví Dụ Lỗi

```
[ERROR] tool: Tool execution failed
{tool=exec, error=Command blocked by safety guard (path outside working dir)}
```

```
[ERROR] tool: Tool execution failed
{tool=exec, error=Command blocked by safety guard (dangerous pattern detected)}
```

#### Tắt Giới Hạn (Rủi Ro Bảo Mật)

Nếu bạn cần agent truy cập đường dẫn ngoài workspace:

**Phương pháp 1: File cấu hình**

```json
{
  "agents": {
    "defaults": {
      "restrict_to_workspace": false
    }
  }
}
```

**Phương pháp 2: Biến môi trường**

```bash
export NEUROCLAW_AGENTS_DEFAULTS_RESTRICT_TO_WORKSPACE=false
```

> ⚠️ **Cảnh báo**: Tắt giới hạn này cho phép agent truy cập bất kỳ đường dẫn nào trên hệ thống. Chỉ sử dụng cẩn thận trong môi trường được kiểm soát.

#### Tính Nhất Quán Ranh Giới Bảo Mật

Cài đặt `restrict_to_workspace` áp dụng nhất quán trên tất cả đường dẫn thực thi:

| Đường Dẫn Thực Thi | Ranh Giới Bảo Mật          |
| -------------------- | ---------------------------- |
| Main Agent           | `restrict_to_workspace` ✅   |
| Subagent / Spawn     | Kế thừa cùng giới hạn ✅    |
| Heartbeat tasks      | Kế thừa cùng giới hạn ✅    |

Tất cả đường dẫn chia sẻ cùng giới hạn workspace — không có cách nào vượt qua ranh giới bảo mật qua subagent hoặc tác vụ lên lịch.

### Heartbeat (Tác Vụ Định Kỳ)

NeuroClaw có thể thực hiện tác vụ định kỳ tự động. Tạo file `HEARTBEAT.md` trong workspace:

```markdown
# Tác Vụ Định Kỳ

- Kiểm tra email cho tin nhắn quan trọng
- Xem lịch cho sự kiện sắp tới
- Kiểm tra dự báo thời tiết
```

Agent sẽ đọc file này mỗi 30 phút (có thể cấu hình) và thực thi các tác vụ sử dụng công cụ có sẵn.

#### Tác Vụ Bất Đồng Bộ Với Spawn

Cho tác vụ chạy lâu (tìm kiếm web, gọi API), sử dụng công cụ `spawn` để tạo **subagent**:

```markdown
# Tác Vụ Định Kỳ

## Tác Vụ Nhanh (trả lời trực tiếp)

- Báo giờ hiện tại

## Tác Vụ Dài (dùng spawn cho bất đồng bộ)

- Tìm kiếm tin tức AI trên web và tóm tắt
- Kiểm tra email và báo cáo tin nhắn quan trọng
```

**Hành vi chính:**

| Tính năng        | Mô tả                                                              |
| ---------------- | ------------------------------------------------------------------ |
| **spawn**        | Tạo subagent bất đồng bộ, không chặn heartbeat                    |
| **Ngữ cảnh độc lập** | Subagent có ngữ cảnh riêng, không có lịch sử phiên             |
| **message tool** | Subagent giao tiếp trực tiếp với người dùng qua message tool      |
| **Không chặn**   | Sau khi spawn, heartbeat tiếp tục tác vụ tiếp theo                |

#### Luồng Giao Tiếp Của Subagent

```
Heartbeat kích hoạt
    ↓
Agent đọc HEARTBEAT.md
    ↓
Tác vụ dài: spawn subagent
    ↓                           ↓
Tiếp tục tác vụ tiếp theo  Subagent hoạt động độc lập
    ↓                           ↓
Hoàn thành tất cả tác vụ   Subagent dùng công cụ "message"
    ↓                           ↓
Trả lời HEARTBEAT_OK        Người dùng nhận kết quả trực tiếp
```

**Cấu hình:**

```json
{
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

| Tùy chọn   | Mặc định | Mô tả                                  |
| ---------- | -------- | -------------------------------------- |
| `enabled`  | `true`   | Bật/tắt heartbeat                      |
| `interval` | `30`     | Khoảng thời gian kiểm tra tính bằng phút (tối thiểu: 5) |

**Biến môi trường:**

* `NEUROCLAW_HEARTBEAT_ENABLED=false` để tắt
* `NEUROCLAW_HEARTBEAT_INTERVAL=60` để thay đổi khoảng thời gian

### Providers

> [!NOTE]
> Groq cung cấp chuyển đổi giọng nói thành văn bản miễn phí qua Whisper. Nếu được cấu hình, tin nhắn âm thanh từ bất kỳ kênh nào sẽ được tự động chuyển đổi ở cấp độ agent.

| Provider     | Mục đích                                | Lấy API Key                                                  |
| ------------ | --------------------------------------- | ------------------------------------------------------------ |
| `gemini`     | LLM (Gemini trực tiếp)                  | [aistudio.google.com](https://aistudio.google.com)           |
| `zhipu`      | LLM (Zhipu trực tiếp)                   | [bigmodel.cn](https://bigmodel.cn)                           |
| `volcengine` | LLM (Volcengine trực tiếp)              | [volcengine.com](https://www.volcengine.com) |
| `openrouter` | LLM (khuyến nghị, truy cập tất cả mô hình) | [openrouter.ai](https://openrouter.ai)                   |
| `anthropic`  | LLM (Claude trực tiếp)                  | [console.anthropic.com](https://console.anthropic.com)       |
| `openai`     | LLM (GPT trực tiếp)                     | [platform.openai.com](https://platform.openai.com)           |
| `deepseek`   | LLM (DeepSeek trực tiếp)                | [platform.deepseek.com](https://platform.deepseek.com)       |
| `qwen`       | LLM (Qwen trực tiếp)                    | [dashscope.console.aliyun.com](https://dashscope.console.aliyun.com) |
| `groq`       | LLM + **Chuyển đổi giọng nói** (Whisper)| [console.groq.com](https://console.groq.com)                 |
| `cerebras`   | LLM (Cerebras trực tiếp)                | [cerebras.ai](https://cerebras.ai)                           |
| `vivgrid`    | LLM (Vivgrid trực tiếp)                 | [vivgrid.com](https://vivgrid.com)                           |

### Cấu Hình Mô Hình (model_list)

> NeuroClaw sử dụng cách tiếp cận **lấy mô hình làm trung tâm**. Chỉ cần chỉ định định dạng `vendor/model` (ví dụ: `zhipu/glm-4.7`) để thêm provider mới — **không cần thay đổi code!**

#### 🔒 Cấu Hình Bảo Mật (Khuyến Nghị)

NeuroClaw hỗ trợ tách dữ liệu nhạy cảm (khóa API, token, secret) khỏi cấu hình chính bằng cách lưu chúng trong file `.security.yml`.

**Lợi ích chính:**
- **Bảo mật**: Dữ liệu nhạy cảm không bao giờ nằm trong file cấu hình chính
- **Chia sẻ dễ dàng**: Chia sẻ config.json mà không lộ khóa API
- **Quản lý phiên bản**: Thêm `.security.yml` vào `.gitignore`
- **Triển khai linh hoạt**: Các môi trường khác nhau có thể dùng file bảo mật khác nhau

**Thiết lập nhanh:**

1. Tạo `~/.neuroclaw/.security.yml` với các khóa API của bạn:
```yaml
model_list:
  gpt-5.4:
    api_keys:
      - "sk-proj-your-actual-openai-key"
  claude-sonnet-4.6:
    api_keys:
      - "sk-ant-your-actual-anthropic-key"
channel_list:
  telegram:
    token: "your-telegram-bot-token"
web:
  brave:
    api_keys:
      - "BSAyour-brave-api-key"
  glm_search:
    api_key: "your-glm-search-api-key"
```

2. Đặt quyền phù hợp:
```bash
chmod 600 ~/.neuroclaw/.security.yml
```

3. Xóa các trường nhạy cảm khỏi `config.json` (khuyến nghị):
```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4"
      // api_key loaded from .security.yml
    }
  ],
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      // token loaded from .security.yml
    }
  }
}
```

**Cách hoạt động:**
- Giá trị từ `.security.yml` được tự động ánh xạ vào các trường cấu hình
- Không cần cú pháp đặc biệt — chỉ cần bỏ các trường nhạy cảm khỏi config.json
- Nếu một trường có ở cả hai file, giá trị trong `.security.yml` được ưu tiên
- Bạn có thể trộn giá trị trực tiếp trong config.json với giá trị bảo mật

Để xem tài liệu đầy đủ, xem [`../security/security_configuration.md`](../security/security_configuration.md).

#### Tất Cả Vendor Được Hỗ Trợ

| Vendor                  | Giá trị `provider` | API Base mặc định                                   | Giao thức | API Key                                                          |
| ----------------------- | ----------------- | --------------------------------------------------- | --------- | ---------------------------------------------------------------- |
| **OpenAI**              | `openai`          | `https://api.openai.com/v1`                         | OpenAI    | [Lấy](https://platform.openai.com)                               |
| **Anthropic**           | `anthropic`       | `https://api.anthropic.com/v1`                      | Anthropic | [Lấy](https://console.anthropic.com)                             |
| **智谱 AI (GLM)**       | `zhipu`           | `https://open.bigmodel.cn/api/paas/v4`              | OpenAI    | [Lấy](https://open.bigmodel.cn/usercenter/proj-mgmt/apikeys)     |
| **DeepSeek**            | `deepseek`        | `https://api.deepseek.com/v1`                       | OpenAI    | [Lấy](https://platform.deepseek.com)                             |
| **Google Gemini**       | `gemini`          | `https://generativelanguage.googleapis.com/v1beta`  | Gemini    | [Lấy](https://aistudio.google.com/api-keys)                      |
| **Groq**                | `groq`            | `https://api.groq.com/openai/v1`                    | OpenAI    | [Lấy](https://console.groq.com)                                  |
| **Moonshot**            | `moonshot`        | `https://api.moonshot.cn/v1`                        | OpenAI    | [Lấy](https://platform.moonshot.cn)                              |
| **通义千问 (Qwen)**     | `qwen`            | `https://dashscope.aliyuncs.com/compatible-mode/v1` | OpenAI    | [Lấy](https://dashscope.console.aliyun.com)                      |
| **NVIDIA**              | `nvidia`          | `https://integrate.api.nvidia.com/v1`               | OpenAI    | [Lấy](https://build.nvidia.com)                                  |
| **Ollama**              | `ollama`          | `http://localhost:11434/v1`                         | OpenAI    | Cục bộ (không cần key)                                           |
| **LM Studio**           | `lmstudio`        | `http://localhost:1234/v1`                          | OpenAI    | Tùy chọn (mặc định cục bộ: không cần key)                        |
| **OpenRouter**          | `openrouter`      | `https://openrouter.ai/api/v1`                      | OpenAI    | [Lấy](https://openrouter.ai/keys)                                |
| **LiteLLM Proxy**       | `litellm`         | `http://localhost:4000/v1`                          | OpenAI    | Khóa proxy LiteLLM của bạn                                       |
| **VLLM**                | `vllm`            | `http://localhost:8000/v1`                          | OpenAI    | Cục bộ                                                           |
| **Cerebras**            | `cerebras`        | `https://api.cerebras.ai/v1`                        | OpenAI    | [Lấy](https://cerebras.ai)                                       |
| **VolcEngine (Doubao)** | `volcengine`      | `https://ark.cn-beijing.volces.com/api/v3`          | OpenAI    | [Lấy](https://www.volcengine.com)                                |
| **神算云**              | `shengsuanyun`    | `https://router.shengsuanyun.com/api/v1`            | OpenAI    | —                                                                |
| **BytePlus**            | `byteplus`        | `https://ark.ap-southeast.bytepluses.com/api/v3`    | OpenAI    | [Lấy](https://www.byteplus.com)                                  |
| **Vivgrid**             | `vivgrid`         | `https://api.vivgrid.com/v1`                        | OpenAI    | [Lấy](https://vivgrid.com)                                       |
| **LongCat**             | `longcat`         | `https://api.longcat.chat/openai`                   | OpenAI    | [Lấy](https://longcat.chat/platform)                             |
| **ModelScope (魔搭)**   | `modelscope`      | `https://api-inference.modelscope.cn/v1`            | OpenAI    | [Lấy token](https://modelscope.cn/my/tokens)                     |
| **Antigravity**         | `antigravity`     | Google Cloud                                        | Custom    | Chỉ OAuth                                                        |
| **GitHub Copilot**      | `github-copilot`  | `localhost:4321`                                    | gRPC      | —                                                                |

#### Cấu Hình Cơ Bản

```json
{
  "model_list": [
    {
      "model_name": "ark-code-latest",
      "provider": "volcengine",
      "model": "ark-code-latest",
      "api_keys": ["sk-your-api-key"]
    },
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_keys": ["sk-your-openai-key"]
    },
    {
      "model_name": "claude-sonnet-4.6",
      "provider": "anthropic",
      "model": "claude-sonnet-4.6",
      "api_keys": ["sk-ant-your-key"]
    },
    {
      "model_name": "glm-4.7",
      "provider": "zhipu",
      "model": "glm-4.7",
      "api_keys": ["your-zhipu-key"]
    }
  ],
  "agents": {
    "defaults": {
      "model_name": "gpt-5.4"
    }
  }
}
```

> **Lưu ý bảo mật**: Bạn có thể xóa các trường `api_keys` khỏi cấu hình và lưu chúng trong `.security.yml`. Xem [Cấu hình bảo mật](#-cấu-hình-bảo-mật-khuyến-nghị) ở trên để biết chi tiết.
>
> **Lưu ý**: Trường `enabled` có thể đặt `false` để tắt một entry model mà không cần xóa nó. Khi bỏ qua, nó mặc định là `true` trong quá trình migrate đối với các model có khóa API.

Quy tắc phân giải:

- Ưu tiên `"provider": "openai", "model": "gpt-5.4"` tường minh.
- Nếu đặt `provider`, NeuroClaw gửi `model` nguyên vẹn.
- Nếu bỏ qua `provider`, NeuroClaw coi đoạn đầu tiên trước `/` trong `model` là provider và mọi thứ sau `/` đầu tiên là ID model runtime.
- Nghĩa là `"model": "openrouter/openai/gpt-5.4"` vẫn hoạt động như một dạng tương thích và gửi `openai/gpt-5.4` tới OpenRouter.

#### Cấu Hình Streaming

Provider streaming dùng cơ chế double opt-in và bị tắt theo mặc định. Agent chỉ thử streaming khi channel hiện tại có `settings.streaming.enabled: true`, entry model đang dùng có `streaming.enabled: true`, và cả provider lẫn channel đều hỗ trợ streaming. Nếu thiếu bất kỳ điều kiện nào, NeuroClaw dùng đường dẫn yêu cầu không streaming thông thường.

Pico WebUI là channel đầu tiên được nối đầy đủ. Pico tạo message assistant đầu tiên bằng wire message hiện có `message.create`, sau đó cập nhật chính message đó bằng `message.update`; không thêm loại wire message Pico mới.

Hãy để trống `streaming` khi bạn không muốn dùng streaming. Bỏ qua block `streaming` nghĩa là đã tắt; bạn không cần viết `"streaming": {"enabled": false}`.

Ví dụ bật streaming:

```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_keys": ["sk-your-openai-key"],
      "streaming": {
        "enabled": true
      }
    }
  ],
  "channel_list": {
    "pico": {
      "enabled": true,
      "type": "pico",
      "settings": {
        "token": "YOUR_PICO_TOKEN",
        "streaming": {
          "enabled": true
        }
      }
    }
  }
}
```

| Trường | Kiểu | Mặc định | Mô tả |
| ------ | ---- | -------- | ----- |
| `channel_list.<name>.settings.streaming.enabled` | bool | `false` | Cho phép channel này hiển thị output streaming từ provider |
| `channel_list.<name>.settings.streaming.throttle_seconds` | int | Mặc định Pico sau khi bật: `0` | Khoảng cách tối thiểu giữa các cập nhật trung gian; nội dung cuối luôn được flush |
| `channel_list.<name>.settings.streaming.min_growth_chars` | int | Mặc định Pico sau khi bật: `1` | Số ký tự tăng tối thiểu trước khi gửi cập nhật trung gian; nội dung cuối luôn được flush |
| `model_list[].streaming.enabled` | bool | `false` | Cho phép entry model này thử yêu cầu provider streaming |

Các biến môi trường Telegram cũ vẫn tương thích: `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_ENABLED`, `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_THROTTLE_SECONDS`, và `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_MIN_GROWTH_CHARS`. Chúng chỉ áp dụng cho Telegram settings và không bật hoặc thay đổi `settings.streaming` của Pico.

Hành vi lỗi được giữ thận trọng: nếu streaming lỗi trước khi gửi bất kỳ chunk hiển thị nào, NeuroClaw thử lại một lần qua đường dẫn `Chat()` thông thường. Nếu đã có chunk hiển thị cho người dùng, NeuroClaw không gửi thêm một câu trả lời non-streaming thứ hai để tránh lặp output.

Với các trường yêu cầu TTS riêng của từng mô hình như tên `voice` giọng nói tùy chỉnh hoặc
`response_format: "mp3"`, dùng `model_list[].extra_body`.

#### Ví Dụ Theo Vendor

> **Mẹo**: Bạn có thể bỏ các trường `api_key` và lưu chúng trong `.security.yml` để bảo mật hơn. Xem [Cấu hình bảo mật](#-cấu-hình-bảo-mật-khuyến-nghị).

<details>
<summary><b>OpenAI</b></summary>

```json
{
  "model_name": "gpt-5.4",
  "provider": "openai",
  "model": "gpt-5.4"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>OpenRouter TTS (MAI Voice 2)</b></summary>

```json
{
  "model_name": "mai-voice-2",
  "provider": "openrouter",
  "model": "microsoft/mai-voice-2",
  "api_base": "https://openrouter.ai/api/v1",
  "extra_body": {
    "voice": "en-US-Harper:MAI-Voice-2",
    "response_format": "mp3"
  }
  // api_key: set in .security.yml
}
```

Kết hợp với:

```json
{
  "voice": {
    "tts_model_name": "mai-voice-2"
  }
}
```

</details>

<details>
<summary><b>VolcEngine (Doubao)</b></summary>

```json
{
  "model_name": "ark-code-latest",
  "provider": "volcengine",
  "model": "ark-code-latest"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>智谱 AI (GLM)</b></summary>

```json
{
  "model_name": "glm-4.7",
  "provider": "zhipu",
  "model": "glm-4.7"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>DeepSeek</b></summary>

```json
{
  "model_name": "deepseek-chat",
  "provider": "deepseek",
  "model": "deepseek-chat"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>Anthropic</b></summary>

```json
{
  "model_name": "claude-sonnet-4.6",
  "provider": "anthropic",
  "model": "claude-sonnet-4.6"
  // api_key: set in .security.yml
}
```

> Chạy `neuroclaw auth login --provider anthropic` để dán token API của bạn.

Với truy cập API Anthropic trực tiếp hoặc các endpoint tùy chỉnh chỉ hỗ trợ định dạng message gốc của Anthropic:

```json
{
  "model_name": "claude-opus-4-6",
  "provider": "anthropic-messages",
  "model": "claude-opus-4-6",
  "api_keys": ["sk-ant-your-key"],
  "api_base": "https://api.anthropic.com"
}
```

> Dùng `anthropic-messages` khi endpoint yêu cầu định dạng `/v1/messages` gốc của Anthropic thay vì `/v1/chat/completions` tương thích OpenAI.

</details>

<details>
<summary><b>Ollama (cục bộ)</b></summary>

```json
{
  "model_name": "llama3",
  "provider": "ollama",
  "model": "llama3"
}
```

</details>

<details>
<summary><b>LM Studio (cục bộ)</b></summary>

```json
{
  "model_name": "lmstudio-local",
  "provider": "lmstudio",
  "model": "openai/gpt-oss-20b"
}
```

`api_base` mặc định là `http://localhost:1234/v1`. Khóa API là tùy chọn trừ khi máy chủ LM Studio của bạn bật xác thực.<br/>
Với `provider` tường minh, NeuroClaw gửi `openai/gpt-oss-20b` nguyên vẹn tới LM Studio. Dạng tương thích cũ `"model": "lmstudio/openai/gpt-oss-20b"` vẫn phân giải về cùng ID model upstream khi bỏ qua `provider`.

</details>

<details>
<summary><b>Proxy tùy chỉnh / LiteLLM</b></summary>

```json
{
  "model_name": "my-custom-model",
  "provider": "openai",
  "model": "custom-model",
  "api_base": "https://my-proxy.com/v1"
  // api_key: set in .security.yml
}
```

Với `provider` tường minh, NeuroClaw gửi `model` nguyên vẹn. Nghĩa là `"provider": "litellm", "model": "lite-gpt4"` gửi `lite-gpt4`, còn `"provider": "litellm", "model": "openai/gpt-4o"` gửi `openai/gpt-4o`. Các dạng tương thích cũ `litellm/lite-gpt4` và `litellm/openai/gpt-4o` vẫn phân giải tương tự khi bỏ qua `provider`.

</details>

#### Cân Bằng Tải

Cấu hình nhiều endpoint cho cùng tên mô hình — NeuroClaw sẽ tự động round-robin:

**Cách 1: Nhiều khóa API trong .security.yml (Khuyến nghị)**

```yaml
# .security.yml
model_list:
  gpt-5.4:
    api_keys:
      - "sk-proj-key-1"
      - "sk-proj-key-2"
```

```json
// config.json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_base": "https://api.openai.com/v1"
      // api_keys loaded from .security.yml
    }
  ]
}
```

**Cách 2: Nhiều entry model**

```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_base": "https://api1.example.com/v1",
      "api_keys": ["sk-key1"]
    },
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_base": "https://api2.example.com/v1",
      "api_keys": ["sk-key2"]
    }
  ]
}
```

#### Di Chuyển Từ Cấu Hình `providers` Cũ

Cấu hình `providers` cũ đã **bị deprecated** và đã được loại bỏ trong V2. Các cấu hình V0/V1 hiện có sẽ được tự động migrate. Xem [docs/migration/model-list-migration.md](../migration/model-list-migration.md) để biết hướng dẫn đầy đủ.

### Kiến Trúc Provider

NeuroClaw định tuyến provider theo họ giao thức:

- **Tương thích OpenAI**: OpenRouter, Groq, Zhipu, endpoint kiểu vLLM và hầu hết các provider khác.
- **Gemini native**: Google Gemini qua các endpoint native `models/*:generateContent` và `models/*:streamGenerateContent`.
- **Anthropic**: Hành vi API Claude gốc.
- **Codex/OAuth**: Tuyến xác thực OAuth/token OpenAI.

Điều này giữ runtime nhẹ trong khi khiến backend OpenAI-compatible mới chủ yếu chỉ là thao tác cấu hình (`api_base` + `api_keys`).

<details>
<summary><b>Zhipu (định dạng providers cũ)</b></summary>

```json
{
  "agents": {
    "defaults": {
      "workspace": "~/.neuroclaw/workspace",
      "model": "glm-4.7",
      "max_tokens": 8192,
      "temperature": 0.7,
      "max_tool_iterations": 20,
      "max_parallel_turns": 1
    }
  },
  "providers": {
    "zhipu": {
      "api_key": "Your API Key",
      "api_base": "https://open.bigmodel.cn/api/paas/v4"
    }
  }
}
```

> **Lưu ý**: Định dạng `providers` đã bị deprecated. Hãy dùng định dạng `model_list` mới cùng `.security.yml` để bảo mật hơn.
>
> **`max_parallel_turns`**: Điều khiển xử lý đồng thời tin nhắn từ các phiên khác nhau. `1` (mặc định) = tuần tự; `>1` = song song. Tin nhắn trong cùng một phiên luôn được tuần tự hóa. Xem [tài liệu Steering](../architecture/steering.md) để biết chi tiết.

</details>

<details>
<summary><b>Ví dụ cấu hình đầy đủ</b></summary>

```json
{
  "agents": {
    "defaults": {
      "model_name": "claude-opus-4-5"
    }
  },
  "session": {
    "dm_scope": "per-channel-peer",
    "backlog_limit": 20
  },
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      // token: set in .security.yml
      "allow_from": ["123456789"]
    }
  },
  "tools": {
    "web": {
      "duckduckgo": {
        "enabled": true,
        "max_results": 5
      }
    }
  },
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

> **Lưu ý**: Các trường nhạy cảm (`api_key`, `token`, v.v.) có thể được bỏ và lưu trong `.security.yml` để bảo mật hơn.

</details>

### Nhật Ký Sự Kiện

NeuroClaw phát các sự kiện runtime từ agent, các kênh, gateway, message bus và MCP. Mặc định chỉ các sự kiện `agent.*` được in ra; các sự kiện khác vẫn đi vào event bus của runtime nhưng không vào log.

```json
{
  "events": {
    "logging": {
      "enabled": true,
      "include": ["agent.*"],
      "exclude": [],
      "min_severity": "info",
      "include_payload": false
    }
  }
}
```

Một cấu hình thường dùng:

```json
{
  "events": {
    "logging": {
      "include": ["*"],
      "exclude": ["agent.llm.delta"],
      "min_severity": "warn"
    }
  }
}
```

`include` / `exclude` nhận tên sự kiện chính xác hoặc mẫu như `gateway.*` và `channel.lifecycle.*`. `include_payload` mặc định tắt để toàn bộ tin nhắn người dùng và đối số công cụ không vào log; các sự kiện agent ghi các trường tóm tắt (độ dài, số lượng, trạng thái) thay vào đó.

Thêm trường và ví dụ: [Sự kiện runtime và nhật ký sự kiện](../architecture/runtime-events.md).

### Tác Vụ Đã Lên Lịch / Nhắc Nhở

NeuroClaw hỗ trợ tác vụ theo lịch qua công cụ `cron`.

```json
{
  "tools": {
    "cron": {
      "enabled": true,
      "exec_timeout_minutes": 5,
      "allow_command": true,
      "command_allowed_remotes": []
    }
  }
}
```

Tác vụ đã lên lịch được lưu trữ bền vững sau khi khởi động lại tại `~/.neuroclaw/workspace/cron/`.

Tác vụ cron dạng lệnh có thể chạy lệnh shell (`allow_command` mặc định là `true` và được báo khi khởi động là `NeuroClaw risk=cron_shell`; xem [Khả năng §2](../reference/capabilities.md)). Theo mặc định, các kênh từ xa không thể lên lịch tác vụ lệnh. Để cho phép một số kênh từ xa, đặt `command_allowed_remotes` thành các mục như `"telegram"` hoặc `"telegram:1234567890"`; chỉ dùng `"*"` nếu mọi kênh không rỗng đều được phép. Ký tự đại diện `"*"` có thể nguy hiểm vì bất kỳ kênh từ xa nào nói chuyện được với NeuroClaw đều có thể lên lịch lệnh shell. Cách này không bỏ qua `allow_command` (khi nó là `false`, một tác vụ cần `command_confirm: true`) hay các kiểm tra an toàn của exec.

### Chủ Đề Nâng Cao

| Chủ đề | Mô tả |
| ------ | ----- |
| [Cấu hình bảo mật](../security/security_configuration.md) | Lưu khóa API và secret trong file `.security.yml` riêng |
| [Lọc dữ liệu nhạy cảm](../security/sensitive_data_filtering.md) | Lọc khóa API và token khỏi kết quả công cụ trước khi gửi cho LLM |
| [Hệ Thống Hook](../architecture/hooks/README.md) | Hook hướng sự kiện: observer, interceptor, approval hook |
| [Steering](../architecture/steering.md) | Chèn tin nhắn vào vòng lặp agent đang chạy giữa các lệnh gọi công cụ |
| [SubTurn](../architecture/subturn.md) | Điều phối subagent, kiểm soát đồng thời, vòng đời |
| [Quản Lý Ngữ Cảnh](../architecture/agent-refactor/context.md) | Phát hiện ranh giới ngữ cảnh, kiểm tra ngân sách chủ động, nén |
