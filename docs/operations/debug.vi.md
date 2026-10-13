# Gỡ lỗi NeuroClaw

> Quay lại [README](../project/README.vi.md)

NeuroClaw thực hiện nhiều tương tác phức tạp ở hậu trường cho mỗi yêu cầu nhận được — từ định tuyến tin nhắn và đánh giá độ phức tạp, đến thực thi công cụ và thích ứng với lỗi mô hình. Khả năng xem chính xác những gì đang xảy ra là rất quan trọng, không chỉ để khắc phục các sự cố tiềm ẩn, mà còn để thực sự hiểu cách agent hoạt động.

## Khởi động NeuroClaw ở chế độ gỡ lỗi

Để nhận thông tin chi tiết về những gì agent đang thực hiện (yêu cầu LLM, lệnh gọi công cụ, định tuyến tin nhắn), bạn có thể khởi động gateway NeuroClaw với cờ gỡ lỗi:

```bash
neuroclaw gateway --debug
# or
neuroclaw gateway -d
```

Ở chế độ này, hệ thống sẽ định dạng log chi tiết và hiển thị bản xem trước của prompt hệ thống và kết quả thực thi công cụ.

## Nhãn rủi ro khi khởi động

Dù có đặt `--debug` hay không, `gateway` và `agent` ghi một cảnh báo `NeuroClaw risk=<name>` cho mỗi khả năng rủi ro đang bật (ví dụ `risk=shell`). Hãy tìm chúng trước tiên khi cần biết mô hình được phép làm gì trên máy này. Tên và ý nghĩa: [capabilities.md mục 2](../reference/capabilities.md#2-risk-labels).

## Tắt cắt ngắn log (log đầy đủ)

Theo mặc định, NeuroClaw cắt ngắn các chuỗi rất dài (như *Prompt Hệ thống* hoặc kết quả JSON lớn) trong log gỡ lỗi để giữ cho console dễ đọc.

Nếu bạn cần kiểm tra đầu ra đầy đủ của một lệnh hoặc payload chính xác được gửi đến mô hình LLM, bạn có thể sử dụng cờ `--no-truncate`.

**Lưu ý:** Cờ này *chỉ* hoạt động khi kết hợp với chế độ `--debug`.

```bash
neuroclaw gateway --debug --no-truncate

```

Khi cờ này được kích hoạt, chức năng cắt ngắn toàn cục sẽ bị vô hiệu hóa. Điều này cực kỳ hữu ích để:

* Xác minh cú pháp chính xác của các tin nhắn được gửi đến nhà cung cấp.
* Đọc đầu ra đầy đủ của các công cụ như `exec`, `web_fetch` hoặc `read_file`.
* Gỡ lỗi lịch sử phiên được lưu trong bộ nhớ.

## Hiển thị lệnh gọi công cụ trong log gỡ lỗi

Khi chế độ gỡ lỗi bật, agent phát ra các mục log có cấu trúc ở mỗi giai đoạn của vòng đời thực thi công cụ. Các mục này mang nhãn `component=agent` và dùng mức `INFO` hoặc `DEBUG` tùy lượng chi tiết:

| Thông điệp log | Mức | Trường chính | Mô tả |
|---|---|---|---|
| `LLM requested tool calls` | INFO | `tools`, `count`, `iteration` | Danh sách tên công cụ mà mô hình quyết định gọi |
| `Tool call: <name>(<args>)` | INFO | `tool`, `iteration` | Tên công cụ và bản xem trước đối số (cắt còn 200 ký tự) |
| `Sent tool result to user` | DEBUG | `tool`, `content_len` | Phát ra khi kết quả công cụ được chuyển tới kênh chat |
| `TTL tick after tool execution` | DEBUG | `agent_id`, `iteration` | Giảm TTL khám phá công cụ MCP sau mỗi vòng công cụ |
| `Async tool completed, publishing result` | INFO | `tool`, `content_len`, `channel` | Chỉ với các công cụ chạy bất đồng bộ trong nền |

### Đọc một mục log lệnh gọi công cụ

Một lệnh gọi công cụ đồng bộ thông thường tạo ra hai dòng liên tiếp trên console:

```
[...] [INFO] agent: LLM requested tool calls {tools=[web_search], count=1, iteration=1}
[...] [INFO] agent: Tool call: web_search({"query":"neuroclaw release notes"}) {tool=web_search, iteration=1}
```

Bản xem trước đối số bị giới hạn cứng ở **200 ký tự** trong log bất kể cờ `--no-truncate`, vì nó thuộc đường `INFO`. Hãy dùng `--no-truncate` cùng `--debug` để xem trường `tools_json` đầy đủ trong mục DEBUG `Full LLM request`, chứa mọi định nghĩa công cụ được gửi tới mô hình.

## Phản hồi công cụ theo thời gian thực trong chat (tool_feedback)

Log gỡ lỗi chỉ nằm phía máy chủ. Nếu bạn muốn agent gửi thông báo hiển thị thẳng vào kênh chat mỗi khi chạy một công cụ — hữu ích khi chia sẻ bot với người khác hoặc để minh bạch — hãy bật `tool_feedback` trong `config.json`:

```json
{
  "agents": {
    "defaults": {
      "tool_feedback": {
        "enabled": true,
        "max_args_length": 300,
        "separate_messages": true
      }
    }
  }
}
```

Khi `enabled` là `true`, mỗi lệnh gọi công cụ gửi một tin nhắn ngắn vào chat trước khi kết quả công cụ được trả về cho mô hình. Tin nhắn trông như sau:

```bash
🔧 `web_search`
{"query": "neuroclaw release notes"}
```

### Tùy chọn

| Trường | Kiểu | Mặc định | Mô tả |
|---|---|---|---|
| `enabled` | bool | `false` | Gửi thông báo chat cho mỗi lệnh gọi công cụ |
| `separate_messages` | bool | `false` | Giữ mỗi cập nhật phản hồi công cụ thành một tin nhắn chat riêng thay vì dùng lại một tin nhắn giữ chỗ/tiến độ |
| `max_args_length` | int | `300` | Số ký tự tối đa của đối số đã tuần tự hóa đưa vào thông báo |

### Biến môi trường

Cả hai trường cũng có thể đặt qua biến môi trường:

```bash
NEUROCLAW_AGENTS_DEFAULTS_TOOL_FEEDBACK_ENABLED=true
NEUROCLAW_AGENTS_DEFAULTS_TOOL_FEEDBACK_MAX_ARGS_LENGTH=300
```

> **Lưu ý:** `tool_feedback` độc lập với chế độ `--debug`. Nó hoạt động trong production và không đòi hỏi khởi động gateway với cờ đặc biệt nào.
