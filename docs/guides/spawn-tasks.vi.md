# 🔄 Tác Vụ Bất Đồng Bộ và Spawn

> Quay lại [README](../project/README.vi.md)

NeuroClaw hỗ trợ **thực thi tác vụ bất đồng bộ** qua công cụ `spawn`. Công cụ này chủ yếu được hệ thống **Heartbeat** dùng để chạy các tác vụ dài mà không chặn vòng lặp agent chính.

## Heartbeat

Hệ thống heartbeat định kỳ kiểm tra `workspace/HEARTBEAT.md` để tìm các tác vụ đã lên lịch. Ở lần chạy đầu tiên, một mẫu mặc định được tự động tạo. Bạn có thể tùy chỉnh để định nghĩa các tác vụ nhanh (xử lý trực tiếp) và tác vụ dài (ủy quyền qua `spawn`).

**Ví dụ `HEARTBEAT.md`:**

```markdown
## Quick Tasks (respond directly)

- Report current time

## Long Tasks (use spawn for async)

- Search the web for AI news and summarize
- Check email and report important messages
```

**Hành vi chính:**

| Tính năng               | Mô tả                                                                  |
| ----------------------- | ---------------------------------------------------------------------- |
| **spawn**               | Tạo subagent bất đồng bộ, không chặn heartbeat                         |
| **Ngữ cảnh độc lập**    | Subagent có ngữ cảnh riêng, không có lịch sử phiên                     |
| **Công cụ message**     | Subagent giao tiếp trực tiếp với người dùng qua công cụ message        |
| **Không chặn**          | Sau khi spawn, heartbeat tiếp tục sang tác vụ kế tiếp                  |

#### Cách Giao Tiếp Subagent Hoạt Động

```
Heartbeat được kích hoạt
    ↓
Agent đọc HEARTBEAT.md
    ↓
Cho tác vụ dài: spawn subagent
    ↓                           ↓
Tiếp tục tác vụ tiếp theo    Subagent làm việc độc lập
    ↓                           ↓
Tất cả tác vụ hoàn thành     Subagent sử dụng công cụ "message"
    ↓                           ↓
Phản hồi HEARTBEAT_OK        Người dùng nhận kết quả trực tiếp
```

Subagent có quyền truy cập công cụ (message, web_search, v.v.) và có thể giao tiếp với người dùng độc lập mà không cần qua agent chính.

**Cấu hình:**

```json
{
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

| Tùy chọn   | Mặc định | Mô tả                                              |
| ---------- | -------- | -------------------------------------------------- |
| `enabled`  | `true`   | Bật/tắt heartbeat                                  |
| `interval` | `30`     | Khoảng thời gian kiểm tra tính bằng phút (tối thiểu: 5) |

**Biến môi trường:**

* `NEUROCLAW_HEARTBEAT_ENABLED=false` để tắt
* `NEUROCLAW_HEARTBEAT_INTERVAL=60` để thay đổi khoảng thời gian
