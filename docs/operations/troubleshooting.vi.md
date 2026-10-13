# 🐛 Khắc Phục Sự Cố

> Quay lại [README](../project/README.vi.md)

## "model ... not found in model_list" hoặc OpenRouter "free is not a valid model ID"

**Triệu chứng:** Bạn thấy một trong các lỗi sau:

- `Error creating provider: model "openrouter/free" not found in model_list`
- OpenRouter trả về 400: `"free is not a valid model ID"`

**Nguyên nhân:** NeuroClaw phân giải provider/model theo hai bước:

- Nếu `provider` được đặt, trường `model` được gửi nguyên vẹn đến provider đó.
- Nếu bỏ qua `provider`, NeuroClaw suy ra provider từ đoạn đầu tiên trước dấu `/` và gửi mọi thứ sau dấu `/` đầu tiên làm ID mô hình lúc chạy.

Với định tuyến tầng miễn phí của OpenRouter, cấu hình ưu tiên là đặt `provider` tường minh.

- **Sai:** `"model": "free"` → không chọn provider OpenRouter nào, nên `free` không phải là tuyến mô hình OpenRouter hợp lệ.
- **Đúng:** `"provider": "openrouter", "model": "free"` → OpenRouter nhận được `free`.
- **Cũng được hỗ trợ:** `"model": "openrouter/free"` → provider được phân giải là `openrouter`, ID mô hình lúc chạy là `free`.

**Cách sửa:** Trong `~/.neuroclaw/config.json` (hoặc đường dẫn cấu hình của bạn):

1. **agents.defaults.model_name** phải khớp với một `model_name` trong `model_list` (ví dụ: `"openrouter-free"`).
2. Mục đó nên đặt **provider** là `openrouter`, và **model** phải là ID mô hình OpenRouter hợp lệ, ví dụ:
   - `"free"` – tầng miễn phí tự động
   - `"google/gemini-2.0-flash-exp:free"`
   - `"meta-llama/llama-3.1-8b-instruct:free"`

Đoạn ví dụ:

```json
{
  "agents": {
    "defaults": {
      "model_name": "openrouter-free"
    }
  },
  "model_list": [
    {
      "model_name": "openrouter-free",
      "provider": "openrouter",
      "model": "free",
      "api_keys": ["sk-or-v1-YOUR_OPENROUTER_KEY"],
      "api_base": "https://openrouter.ai/api/v1"
    }
  ]
}
```

Lấy khóa của bạn tại [OpenRouter Keys](https://openrouter.ai/keys).
