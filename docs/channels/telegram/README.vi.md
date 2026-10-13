> Quay lại [README](../../project/README.vi.md)

# Telegram

Kênh Telegram sử dụng long polling qua Telegram Bot API để giao tiếp dựa trên bot. Hỗ trợ tin nhắn văn bản, tệp đính kèm đa phương tiện (ảnh, giọng nói, âm thanh, tài liệu), chuyển giọng nói thành văn bản ([thiết lập](../../guides/providers.vi.md)) và xử lý lệnh tích hợp sẵn.

## Cấu hình

```json
{
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      "token": "123456789:ABCdefGHIjklMNOpqrsTUVwxyz",
      "allow_from": ["123456789"],
      "proxy": "",
      "use_markdown_v2": false,
      "media_group_delay_ms": 500
    }
  }
}
```

| Trường         | Kiểu   | Bắt buộc | Mô tả                                                                    |
| -------------- | ------ | -------- | ------------------------------------------------------------------------ |
| enabled        | bool   | Có       | Có bật kênh Telegram hay không                                           |
| token          | string | Có       | Token API Bot Telegram                                                   |
| allow_from     | array  | Không    | Danh sách trắng ID người dùng; để trống thì ai cũng vào được (gateway ghi cảnh báo); hãy đặt giá trị, hoặc dùng `["*"]` nếu cố ý cho tất cả |
| proxy          | string | Không    | URL proxy để kết nối với Telegram API (ví dụ: http://127.0.0.1:7890)    |
| use_markdown_v2 | bool   | Không    | Bật định dạng Telegram MarkdownV2                                        |
| media_group_delay_ms | int | Không   | Độ trễ chờ yên lặng trước khi xử lý nhóm media/album Telegram. Mặc định 500 ms |

## Hướng dẫn thiết lập

1. Tìm kiếm `@BotFather` trong Telegram
2. Gửi lệnh `/newbot` và làm theo hướng dẫn để tạo bot mới
3. Lấy Token API HTTP
4. Điền Token vào file cấu hình
5. (Tùy chọn) Cấu hình `allow_from` để giới hạn ID người dùng được phép tương tác (có thể lấy ID qua `@userinfobot`)

## Lệnh tích hợp

Telegram tự động đăng ký các lệnh bot cấp cao nhất của NeuroClaw khi khởi động, gồm `/start`, `/help`, `/show`, `/list` và `/use`.

Các lệnh liên quan đến skill:

- `/list skills` liệt kê các skill đã cài mà agent hiện tại thấy được.
- `/list mcp` liệt kê các máy chủ MCP đã cấu hình và cho biết chúng bị trì hoãn (deferred) hay đã kết nối.
- `/show mcp <server>` liệt kê các công cụ đang hoạt động của một máy chủ MCP đã kết nối.
- `/use <skill> <message>` ép dùng một skill cho một yêu cầu.
- `/use <skill>` chuẩn bị skill cho tin nhắn tiếp theo của bạn trong cùng cuộc chat.
- `/use clear` xóa ghi đè skill đang chờ.

Ví dụ:

```text
/list skills
/list mcp
/show mcp github
/use git explain how to squash the last 3 commits
/use git
explain how to squash the last 3 commits
```

## Định dạng nâng cao

Bạn có thể đặt `use_markdown_v2: true` để bật các tùy chọn định dạng nâng cao. Điều này cho phép bot sử dụng toàn bộ các tính năng của Telegram MarkdownV2, bao gồm các kiểu lồng nhau, spoiler và các khối chiều rộng cố định tùy chỉnh.

```json
{
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"],
      "use_markdown_v2": true
    }
  }
}
```
