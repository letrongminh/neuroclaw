> Quay lại [README](../../project/README.vi.md)

# Discord

Discord là ứng dụng chat thoại, video và văn bản miễn phí được thiết kế cho cộng đồng. NeuroClaw kết nối với máy chủ Discord qua Discord Bot API, hỗ trợ nhận và gửi tin nhắn.

## Cấu hình

```json
{
  "agents": {
    "defaults": {
      "tool_feedback": {
        "enabled": true,
        "max_args_length": 300
      }
    }
  },
  "channel_list": {
    "discord": {
      "enabled": true,
      "type": "discord",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"],
      "placeholder": {
        "enabled": true,
        "text": ["Thinking... 💭"]
      },
      "group_trigger": {
        "mention_only": false
      },
      "reasoning_channel_id": ""
    }
  }
}
```

| Trường        | Kiểu   | Bắt buộc | Mô tả                                                                       |
| ------------- | ------ | -------- | --------------------------------------------------------------------------- |
| enabled       | bool   | Có       | Có bật kênh Discord hay không                                               |
| token         | string | Có       | Token Bot Discord                                                           |
| allow_from    | array  | Không    | Danh sách trắng ID người dùng; để trống thì ai cũng vào được (gateway ghi cảnh báo); hãy đặt giá trị, hoặc dùng `["*"]` nếu cố ý cho tất cả |
| placeholder   | object | Không    | Cấu hình tin nhắn giữ chỗ hiển thị khi agent đang làm việc                 |
| group_trigger | object | Không    | Cài đặt kích hoạt nhóm (ví dụ: { "mention_only": false })                  |
| reasoning_channel_id | string | Không | ID kênh đích tùy chọn cho đầu ra suy luận                              |

## Phản hồi thực thi hiển thị

Discord có thể hiển thị ba loại phản hồi "đang làm việc":

1. Chỉ báo đang gõ: tự động, không cần cấu hình thêm.
2. Tin nhắn giữ chỗ: bật `channel_list.discord.placeholder.enabled` để gửi tin nhắn `Thinking...` hiển thị, sau đó được sửa thành câu trả lời cuối.
3. Phản hồi thực thi công cụ: bật `agents.defaults.tool_feedback.enabled` để gửi một tin nhắn ngắn trước mỗi lệnh gọi công cụ, ví dụ:

```text
🔧 `web_search`
Checking the latest NeuroClaw release notes before I answer.
```

Nếu bạn chỉ thấy `Bot is typing`, hãy kiểm tra `placeholder.enabled` hoặc `tool_feedback.enabled` đã thực sự được đặt trong cấu hình chạy của bạn.

## Hướng dẫn thiết lập

1. Truy cập [Discord Developer Portal](https://discord.com/developers/applications) và tạo ứng dụng mới
2. Bật các Intents:
   - Message Content Intent
   - Server Members Intent
3. Lấy Bot Token
4. Điền Bot Token vào file cấu hình
5. Mời bot vào máy chủ và cấp các quyền cần thiết (ví dụ: gửi tin nhắn, đọc lịch sử tin nhắn)
