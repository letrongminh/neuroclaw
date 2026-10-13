# 🔌 Nhà Cung Cấp và Cấu Hình Mô Hình

> Quay lại [README](../project/README.vi.md)

### Nhà Cung Cấp

> [!NOTE]
> Chuyển giọng nói thành văn bản có thể dùng một model đa phương thức đã cấu hình qua `voice.model_name`. Groq Whisper vẫn khả dụng làm phương án dự phòng khi chưa cấu hình model giọng nói.

| Provider     | Purpose                                 | Get API Key                                                  |
| ------------ | --------------------------------------- | ------------------------------------------------------------ |
| `gemini`     | LLM (Gemini direct)                     | [aistudio.google.com](https://aistudio.google.com)           |
| `zhipu`      | LLM (Zhipu direct)                      | [bigmodel.cn](https://bigmodel.cn)                           |
| `zai`        | LLM (Z.AI Coding Plan)                  | [z.ai](https://z.ai/manage-apikey/apikey-list)           |
| `volcengine` | LLM(Volcengine direct)                  | [volcengine.com](https://www.volcengine.com)                 |
| `openrouter` | LLM (recommended, access to all models) | [openrouter.ai](https://openrouter.ai)                       |
| `anthropic`  | LLM (Claude direct)                     | [console.anthropic.com](https://console.anthropic.com)       |
| `openai`     | LLM (GPT direct)                        | [platform.openai.com](https://platform.openai.com)           |
| `venice`     | LLM (Venice AI direct)                  | [venice.ai](https://venice.ai)                               |
| `nearai`     | LLM (NEAR AI Cloud TEE inference)       | [near.ai](https://near.ai)                                   |
| `deepseek`   | LLM (DeepSeek direct)                   | [platform.deepseek.com](https://platform.deepseek.com)       |
| `qwen`       | LLM (Qwen direct)                       | [dashscope.console.aliyun.com](https://dashscope.console.aliyun.com) |
| `groq`       | LLM + **Voice transcription** (Whisper) | [console.groq.com](https://console.groq.com)                 |
| `cerebras`   | LLM (Cerebras direct)                   | [cerebras.ai](https://cerebras.ai)                           |
| `vivgrid`    | LLM (Vivgrid direct)                    | [vivgrid.com](https://vivgrid.com)                           |
| `nvidia`     | LLM (NVIDIA NIM)                        | [build.nvidia.com](https://build.nvidia.com)                 |
| `moonshot`   | LLM (Kimi/Moonshot direct)              | [platform.moonshot.cn](https://platform.moonshot.cn)         |
| `minimax`    | LLM (Minimax direct)                    | [platform.minimaxi.com](https://platform.minimaxi.com)      |
| `avian`      | LLM (Avian direct)                      | [avian.io](https://avian.io)                                 |
| `mistral`    | LLM (Mistral direct)                    | [console.mistral.ai](https://console.mistral.ai)            |
| `longcat`    | LLM (Longcat direct)                    | [longcat.ai](https://longcat.ai)                             |
| `modelscope` | LLM (ModelScope direct)                 | [modelscope.cn](https://modelscope.cn)                       |
| `mimo`       | LLM (Xiaomi MiMo direct)                | [platform.xiaomimimo.com](https://platform.xiaomimimo.com)   |

### Cấu Hình Mô Hình (model_list)

> NeuroClaw ưu tiên cấu hình tường minh `provider` + `model` gốc (ví dụ `"provider": "zhipu", "model": "glm-4.7"`). Dạng một trường `provider/model` cũ vẫn được hỗ trợ để tương thích khi bỏ qua `provider`.

Về ví dụ điều phối agent và định tuyến mô hình nhẹ, xem [Hướng dẫn Routing](routing-guide.md).

Thiết kế này cũng cho phép **hỗ trợ đa agent** với lựa chọn provider linh hoạt:

- **Agent khác nhau, provider khác nhau**: Mỗi agent có thể sử dụng provider LLM riêng
- **Fallback mô hình**: Cấu hình mô hình chính và dự phòng cho khả năng phục hồi
- **Cân bằng tải**: Phân phối yêu cầu qua nhiều endpoint
- **Cấu hình tập trung**: Quản lý tất cả provider tại một nơi

#### 📋 Tất Cả Vendor Được Hỗ Trợ

| Vendor              | Giá trị `provider` | Default API Base                                    | Protocol  | API Key                                                          |
| ------------------- | ----------------- |-----------------------------------------------------| --------- | ---------------------------------------------------------------- |
| **OpenAI**          | `openai`          | `https://api.openai.com/v1`                         | OpenAI    | [Get Key](https://platform.openai.com)                           |
| **Venice AI**       | `venice`          | `https://api.venice.ai/api/v1`                      | OpenAI    | [Get Key](https://venice.ai)                                     |
| **NEAR AI Cloud**   | `nearai`          | `https://cloud-api.near.ai/v1`                      | OpenAI    | [Get Key](https://near.ai)                                       |
| **Anthropic**       | `anthropic`       | `https://api.anthropic.com/v1`                      | Anthropic | [Get Key](https://console.anthropic.com)                         |
| **智谱 AI (GLM)**   | `zhipu`           | `https://open.bigmodel.cn/api/paas/v4`              | OpenAI    | [Get Key](https://open.bigmodel.cn/usercenter/proj-mgmt/apikeys) |
| **Z.AI Coding Plan** | `openai`         | `https://api.z.ai/api/coding/paas/v4`               | OpenAI    | [Get Key](https://z.ai/manage-apikey/apikey-list)                |
| **DeepSeek**        | `deepseek`        | `https://api.deepseek.com/v1`                       | OpenAI    | [Get Key](https://platform.deepseek.com)                         |
| **Google Gemini**   | `gemini`          | `https://generativelanguage.googleapis.com/v1beta`  | Gemini    | [Get Key](https://aistudio.google.com/api-keys)                  |
| **Groq**            | `groq`            | `https://api.groq.com/openai/v1`                    | OpenAI    | [Get Key](https://console.groq.com)                              |
| **Moonshot**        | `moonshot`        | `https://api.moonshot.cn/v1`                        | OpenAI    | [Get Key](https://platform.moonshot.cn)                          |
| **通义千问 (Qwen)** | `qwen`            | `https://dashscope.aliyuncs.com/compatible-mode/v1` | OpenAI    | [Get Key](https://dashscope.console.aliyun.com)                  |
| **NVIDIA**          | `nvidia`          | `https://integrate.api.nvidia.com/v1`               | OpenAI    | [Get Key](https://build.nvidia.com)                              |
| **Ollama**          | `ollama`          | `http://localhost:11434/v1`                         | OpenAI    | Local (no key needed)                                            |
| **LM Studio**       | `lmstudio`        | `http://localhost:1234/v1`                          | OpenAI    | Optional (local default: no key)                                 |
| **OpenRouter**      | `openrouter`      | `https://openrouter.ai/api/v1`                      | OpenAI    | [Get Key](https://openrouter.ai/keys)                            |
| **LiteLLM Proxy**   | `litellm`         | `http://localhost:4000/v1`                          | OpenAI    | Your LiteLLM proxy key                                           |
| **VLLM**            | `vllm`            | `http://localhost:8000/v1`                          | OpenAI    | Local                                                            |
| **Cerebras**        | `cerebras`        | `https://api.cerebras.ai/v1`                        | OpenAI    | [Get Key](https://cerebras.ai)                                   |
| **VolcEngine (Doubao)** | `volcengine`  | `https://ark.cn-beijing.volces.com/api/v3`          | OpenAI    | [Get Key](https://www.volcengine.com) |
| **神算云**          | `shengsuanyun`    | `https://router.shengsuanyun.com/api/v1`            | OpenAI    | -                                                                |
| **BytePlus**        | `byteplus`        | `https://ark.ap-southeast.bytepluses.com/api/v3`    | OpenAI    | [Get Key](https://www.byteplus.com)                              |
| **Vivgrid**         | `vivgrid`         | `https://api.vivgrid.com/v1`                        | OpenAI    | [Get Key](https://vivgrid.com)                                   |
| **LongCat**         | `longcat`         | `https://api.longcat.chat/openai`                   | OpenAI    | [Get Key](https://longcat.chat/platform)                         |
| **ModelScope (魔搭)**| `modelscope`     | `https://api-inference.modelscope.cn/v1`            | OpenAI    | [Get Token](https://modelscope.cn/my/tokens)                     |
| **Xiaomi MiMo**     | `mimo`            | `https://api.xiaomimimo.com/v1`                     | OpenAI    | [Get Key](https://platform.xiaomimimo.com)                       |
| **Azure OpenAI**    | `azure`           | `https://{resource}.openai.azure.com`               | Azure     | [Get Key](https://portal.azure.com)                              |
| **Antigravity**     | `antigravity`     | Google Cloud                                        | Custom    | OAuth only                                                       |
| **GitHub Copilot**  | `github-copilot`  | `localhost:4321`                                    | gRPC      | -                                                                |

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

#### Các trường entry `model_list`

| Trường | Kiểu | Bắt buộc | Mô tả |
|--------|------|----------|------|
| `model_name` | string | Có | Tên duy nhất để tham chiếu model này trong cấu hình agent |
| `provider` | string | Không | Định danh provider ưu tiên. Khi có, NeuroClaw gửi `model` nguyên vẹn tới provider đó |
| `model` | string | Có | ID model gốc khi đã đặt `provider`. Nếu bỏ qua `provider`, dạng cũ `provider/model` vẫn được hỗ trợ |
| `api_keys` | string[] | Có* | Khóa API xác thực. Nhiều khóa cho phép xoay vòng theo yêu cầu. Không cần thiết cho provider nội bộ (Ollama, LM Studio, VLLM) |
| `api_base` | string | Không | Ghi đè URL endpoint API mặc định |
| `proxy` | string | Không | URL proxy HTTP cho entry model này |
| `user_agent` | string | Không | Header `User-Agent` tùy chỉnh gửi với yêu cầu API (được hỗ trợ bởi provider OpenAI-compatible, Gemini, Anthropic và Azure) |
| `request_timeout` | int | Không | Timeout yêu cầu tính bằng giây (mặc định khác nhau tùy provider) |
| `max_tokens_field` | string | Không | Ghi đè tên trường max tokens trong request body (ví dụ: `max_completion_tokens` cho model o1) |
| `thinking_level` | string | Không | Mức độ tư duy mở rộng: `off`, `low`, `medium`, `high`, `xhigh` hoặc `adaptive` |
| `tool_schema_transform` | string | Không | Phép biến đổi tương thích tùy chọn cho schema tham số của tool. Mặc định: tắt. Giá trị hỗ trợ: `simple`. |
| `extra_body` | object | Không | Các trường bổ sung để chèn vào mỗi request body |
| `custom_headers` | object | Không | Các HTTP header bổ sung chèn vào mỗi yêu cầu (ví dụ: `{"X-Source":"coding-plan"}`). Nếu khóa trùng với header có sẵn, giá trị tùy chỉnh ghi đè header có sẵn (ví dụ: `Authorization`, `User-Agent`, `Content-Type`, `Accept`). |
| `streaming.enabled` | bool | Không | Opt-in cho provider streaming trên entry model này. Mặc định là `false` và channel đang hoạt động cũng cần `settings.streaming.enabled` là `true` |
| `rpm` | int | Không | Giới hạn tốc độ yêu cầu mỗi phút |
| `fallbacks` | string[] | Không | Tên model dự phòng cho failover tự động |
| `enabled` | bool | Không | Kích hoạt hay vô hiệu hóa entry model này (mặc định: `true`) |

Khi không dùng streaming, hãy bỏ qua block `streaming`. Viết `"streaming": {"enabled": false}` là tùy chọn và không cần thiết trong cấu hình sinh tự động hay viết tay.

`extra_body` đặc biệt hữu ích cho các trường TTS riêng của model trên các route speech
tương thích OpenAI, ví dụ tên `voice` tùy chỉnh hoặc `response_format: "mp3"`.

#### Tương thích Tool Schema

Theo mặc định, NeuroClaw chuyển tiếp JSON Schema của tool nguyên vẹn.

Một số provider từ chối các tính năng JSON Schema nâng cao như `$ref`, `$defs`, `anyOf`, `oneOf`, `allOf`, `pattern`, hoặc ràng buộc số/chuỗi trong khai báo tool. Với các model đó, bạn có thể bật phép biến đổi tương thích cho từng entry model bằng `tool_schema_transform`.

Dùng `simple` khi provider upstream chỉ chấp nhận tập con schema hàm kiểu bảo thủ:

```json
{
  "model_name": "gemini-2.5-flash-safe-tools",
  "provider": "gemini",
  "model": "gemini-2.5-flash",
  "api_keys": ["your-gemini-key"],
  "tool_schema_transform": "simple"
}
```

Ghi chú:

- Mặc định là tắt. Nếu bỏ qua `tool_schema_transform`, NeuroClaw gửi schema tool gốc.
- Cài đặt này theo từng entry model, nên bạn chỉ bật cho những provider cần nó.

#### Phân giải Provider / Model

NeuroClaw phân giải `provider` và ID model lúc chạy theo các quy tắc sau:

- Nếu đặt `provider`, `model` được dùng nguyên trạng.
- Nếu bỏ qua `provider`, NeuroClaw coi đoạn đầu tiên trước `/` trong `model` là provider và phần còn lại sau `/` đầu tiên là ID model lúc chạy.

Ví dụ:

| Cấu hình | Provider được phân giải | Model gửi lên upstream |
| --- | --- | --- |
| `"provider": "openai", "model": "gpt-5.4"` | `openai` | `gpt-5.4` |
| `"model": "openai/gpt-5.4"` | `openai` | `gpt-5.4` |
| `"provider": "openrouter", "model": "openai/gpt-5.4"` | `openrouter` | `openai/gpt-5.4` |
| `"model": "openrouter/openai/gpt-5.4"` | `openrouter` | `openai/gpt-5.4` |

#### Chuyển giọng nói thành văn bản

Bạn có thể cấu hình một model riêng cho việc chuyển âm thanh thành văn bản bằng `voice.model_name`. Nhờ đó bạn tái sử dụng các provider đa phương thức hiện có hỗ trợ đầu vào âm thanh thay vì chỉ dựa vào Groq.

Nếu không cấu hình `voice.model_name`, NeuroClaw vẫn dùng phiên âm của Groq làm phương án dự phòng khi có khóa API Groq.

```json
{
  "model_list": [
    {
      "model_name": "voice-gemini",
      "provider": "gemini",
      "model": "gemini-2.5-flash",
      "api_keys": ["your-gemini-key"]
    }
  ],
  "voice": {
    "model_name": "voice-gemini",
    "echo_transcription": false
  },
  "providers": {
    "groq": {
      "api_key": "gsk_xxx"
    }
  }
}
```

#### Tổng hợp giọng nói

Bạn có thể cấu hình một model text-to-speech riêng bằng `voice.tts_model_name`.
Khi provider cần các trường yêu cầu TTS riêng của model, hãy đặt chúng trong
`model_list[].extra_body`.

Ví dụ với OpenRouter `microsoft/mai-voice-2`:

```json
{
  "model_list": [
    {
      "model_name": "mai-voice-2",
      "provider": "openrouter",
      "model": "microsoft/mai-voice-2",
      "api_base": "https://openrouter.ai/api/v1",
      "extra_body": {
        "voice": "en-US-Harper:MAI-Voice-2",
        "response_format": "mp3"
      },
      "api_keys": ["sk-or-your-openrouter-key"]
    }
  ],
  "voice": {
    "tts_model_name": "mai-voice-2"
  }
}
```

Các ghi chú quan trọng:

- NeuroClaw vẫn dùng route `/audio/speech` tương thích OpenAI cho thiết lập này.
- Yêu cầu TTS mặc định dùng `voice: alloy` và `response_format: opus`.
- Ghi đè các mặc định đó bằng `extra_body` khi model TTS bạn chọn cần giá trị khác.

#### Ví Dụ Theo Vendor

**OpenAI**

```json
{
  "model_name": "gpt-5.4",
  "provider": "openai",
  "model": "gpt-5.4",
  "api_keys": ["sk-..."]
}
```

**NEAR AI Cloud**

```json
{
  "model_name": "nearai-glm",
  "provider": "nearai",
  "model": "zai-org/GLM-5.1-FP8",
  "api_keys": ["your-nearai-api-key"]
}
```

**VolcEngine (Doubao)**

```json
{
  "model_name": "ark-code-latest",
  "provider": "volcengine",
  "model": "ark-code-latest",
  "api_keys": ["sk-..."]
}
```

**智谱 AI (GLM)**

```json
{
  "model_name": "glm-4.7",
  "provider": "zhipu",
  "model": "glm-4.7",
  "api_keys": ["your-key"]
}
```

**Z.AI Coding Plan (GLM)**
> Z.AI và 智谱 AI là hai thương hiệu của cùng một nhà cung cấp. Với Z.AI Coding Plan, hãy dùng khóa model `openai` và api base như dưới đây, thay vì cấu hình zhipu
```json
{
  "model_name": "glm-4.7",
  "provider": "openai",
  "model": "glm-4.7",
  "api_keys": ["your-z.ai-key"],
  "api_base": "https://api.z.ai/api/coding/paas/v4"
}
```

**DeepSeek**

```json
{
  "model_name": "deepseek-chat",
  "provider": "deepseek",
  "model": "deepseek-chat",
  "api_keys": ["sk-..."]
}
```

**Anthropic (với API key)**

```json
{
  "model_name": "claude-sonnet-4.6",
  "provider": "anthropic",
  "model": "claude-sonnet-4.6",
  "api_keys": ["sk-ant-your-key"]
}
```

> Chạy `neuroclaw auth login --provider anthropic` để dán API token.

**Anthropic Messages API (định dạng native)**

Để truy cập trực tiếp API Anthropic hoặc endpoint tùy chỉnh chỉ hỗ trợ định dạng message native của Anthropic:

```json
{
  "model_name": "claude-opus-4-6",
  "provider": "anthropic-messages",
  "model": "claude-opus-4-6",
  "api_keys": ["sk-ant-your-key"],
  "api_base": "https://api.anthropic.com"
}
```

> Sử dụng giao thức `anthropic-messages` khi:
> - Sử dụng proxy bên thứ ba chỉ hỗ trợ endpoint native `/v1/messages` của Anthropic (không tương thích OpenAI `/v1/chat/completions`)
> - Kết nối đến dịch vụ như MiniMax, Synthetic yêu cầu định dạng message native của Anthropic
> - Giao thức `anthropic` hiện tại trả về lỗi 404 (cho thấy endpoint không hỗ trợ định dạng tương thích OpenAI)
>
> **Lưu ý:** Giao thức `anthropic` sử dụng định dạng tương thích OpenAI (`/v1/chat/completions`), trong khi `anthropic-messages` sử dụng định dạng native của Anthropic (`/v1/messages`). Chọn dựa trên định dạng endpoint hỗ trợ.

**Ollama (local)**

```json
{
  "model_name": "llama3",
  "provider": "ollama",
  "model": "llama3"
}
```

**LM Studio (local)**

```json
{
  "model_name": "lmstudio-local",
  "provider": "lmstudio",
  "model": "openai/gpt-oss-20b"
}
```

`api_base` mặc định là `http://localhost:1234/v1`. API key là tùy chọn trừ khi máy chủ LM Studio bật xác thực.<br/>
Khi đặt `provider` tường minh, NeuroClaw gửi `openai/gpt-oss-20b` nguyên vẹn tới máy chủ LM Studio. Dạng tương thích cũ `"model": "lmstudio/openai/gpt-oss-20b"` vẫn phân giải ra cùng ID model upstream khi bỏ qua `provider`.

**Proxy/API Tùy Chỉnh**

```json
{
  "model_name": "my-custom-model",
  "provider": "openai",
  "model": "custom-model",
  "api_base": "https://my-proxy.com/v1",
  "api_keys": ["sk-..."],
  "user_agent": "MyApp/1.0",
  "request_timeout": 300
}
```

**LiteLLM Proxy**

```json
{
  "model_name": "lite-gpt4",
  "provider": "litellm",
  "model": "lite-gpt4",
  "api_base": "http://localhost:4000/v1",
  "api_keys": ["sk-..."]
}
```

Khi đặt `provider` tường minh, NeuroClaw gửi `model` nguyên vẹn. Nghĩa là `"provider": "litellm", "model": "lite-gpt4"` gửi `lite-gpt4`, còn `"provider": "litellm", "model": "openai/gpt-4o"` gửi `openai/gpt-4o`. Các dạng tương thích cũ `litellm/lite-gpt4` và `litellm/openai/gpt-4o` vẫn phân giải giống vậy khi bỏ qua `provider`.

**Z.AI Coding Plan**

Nếu endpoint Zhipu tiêu chuẩn (`https://open.bigmodel.cn/api/paas/v4`) trả về 429 (mã 1113: không đủ số dư), hãy thử dùng endpoint Z.AI Coding Plan:

```json
{
  "model_name": "glm-4.7",
  "provider": "openai",
  "model": "glm-4.7",
  "api_keys": ["your-zhipu-api-key"],
  "api_base": "https://api.z.ai/api/coding/paas/v4"
}
```

**Lưu ý:** Endpoint Z.AI Coding Plan và endpoint Zhipu tiêu chuẩn dùng cùng định dạng API key nhưng tính phí riêng. Nếu gặp lỗi 429 với endpoint Zhipu tiêu chuẩn, endpoint Z.AI Coding Plan có thể còn số dư.

#### Cân Bằng Tải

Cấu hình nhiều endpoint cho cùng tên mô hình — NeuroClaw sẽ tự động round-robin giữa chúng:

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

#### Failover Mô Hình Tự Động (Cascade)

NeuroClaw đã hỗ trợ failover tự động khi bạn cấu hình `primary` + `fallbacks` trong cài đặt model của agent.
Chuỗi fallback lúc chạy thử ứng viên kế tiếp khi gặp lỗi có thể thử lại như HTTP `429`, lỗi quota/giới hạn tốc độ và lỗi timeout.
Nó cũng theo dõi thời gian chờ (cooldown) cho từng ứng viên để tránh thử lại ngay một đích vừa lỗi.

```json
{
  "model_list": [
    {
      "model_name": "qwen-main",
      "provider": "openai",
      "model": "qwen3.5:cloud",
      "api_base": "https://api.example.com/v1",
      "api_keys": ["sk-main"]
    },
    {
      "model_name": "deepseek-backup",
      "provider": "deepseek",
      "model": "deepseek-chat",
      "api_keys": ["sk-backup-1"]
    },
    {
      "model_name": "gemini-backup",
      "provider": "gemini",
      "model": "gemini-2.5-flash",
      "api_keys": ["sk-backup-2"]
    }
  ],
  "agents": {
    "defaults": {
      "model_name": "qwen-main",
      "model_fallbacks": ["deepseek-backup", "gemini-backup"]
    }
  }
}
```

Nếu bạn dùng failover ở mức khóa cho cùng một model, NeuroClaw có thể đi qua thêm các ứng viên theo khóa trước khi chuyển sang các bản dự phòng khác model.

#### Di Chuyển Từ Cấu Hình Legacy `providers`

Cấu hình `providers` cũ đã **bị deprecated** và đã được loại bỏ trong V2. Các cấu hình V0/V1 hiện có sẽ được tự động migrate.

**Cấu hình cũ (ngừng hỗ trợ):**

```json
{
  "providers": {
    "zhipu": {
      "api_key": "your-key",
      "api_base": "https://open.bigmodel.cn/api/paas/v4"
    }
  },
  "agents": {
    "defaults": {
      "provider": "zhipu",
      "model": "glm-4.7"
    }
  }
}
```

**Cấu hình mới (khuyến nghị):**

```json
{
  "version": 3,
  "model_list": [
    {
      "model_name": "glm-4.7",
      "provider": "zhipu",
      "model": "glm-4.7",
      "api_keys": ["your-key"]
    }
  ],
  "agents": {
    "defaults": {
      "model_name": "glm-4.7"
    }
  }
}
```

Để xem hướng dẫn di chuyển chi tiết, xem [migration/model-list-migration.md](../migration/model-list-migration.md).

### Kiến Trúc Provider

NeuroClaw định tuyến provider theo họ giao thức:

- Giao thức tương thích OpenAI: OpenRouter, gateway tương thích OpenAI, Groq, Zhipu, và endpoint kiểu vLLM.
- Giao thức Gemini native: Google Gemini qua các endpoint native `models/*:generateContent` và `models/*:streamGenerateContent`.
- Giao thức Anthropic: Hành vi API native của Claude.
- Đường dẫn Codex/OAuth: Tuyến xác thực OAuth/token của OpenAI.

Điều này giữ runtime nhẹ trong khi làm cho backend tương thích OpenAI mới chủ yếu là thao tác cấu hình (`api_base` + `api_keys`).

<details>
<summary><b>Zhipu</b></summary>

**1. Lấy API key và URL base**

* Lấy [API key](https://bigmodel.cn/usercenter/proj-mgmt/apikeys)

**2. Cấu hình**

```json
{
  "agents": {
    "defaults": {
      "workspace": "~/.neuroclaw/workspace",
      "model_name": "glm-4.7",
      "max_tokens": 8192,
      "temperature": 0.7,
      "max_tool_iterations": 20
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

**3. Chạy**

```bash
neuroclaw agent -m "Hello"
```

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
    "dm_scope": "per-channel-peer"
  },
  "providers": {
    "openrouter": {
      "api_key": "sk-or-v1-xxx"
    },
    "groq": {
      "api_key": "gsk_xxx"
    }
  },
  "voice": {
    "model_name": "voice-gemini",
    "echo_transcription": false
  },
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      "token": "123456:ABC...",
      "allow_from": ["123456789"]
    },
    "discord": {
      "enabled": true,
      "type": "discord",
      "token": "",
      "allow_from": [""]
    },
    "whatsapp": {
      "enabled": false,
      "type": "whatsapp",
      "bridge_url": "ws://localhost:3001",
      "use_native": false,
      "session_store_path": "",
      "allow_from": []
    },
    "feishu": {
      "enabled": false,
      "type": "feishu",
      "app_id": "cli_xxx",
      "app_secret": "xxx",
      "encrypt_key": "",
      "verification_token": "",
      "allow_from": []
    },
    "qq": {
      "enabled": false,
      "type": "qq",
      "app_id": "",
      "app_secret": "",
      "allow_from": []
    }
  },
  "tools": {
    "web": {
      "brave": {
        "enabled": false,
        "api_key": "BSA...",
        "max_results": 5
      },
      "duckduckgo": {
        "enabled": true,
        "max_results": 5
      },
      "perplexity": {
        "enabled": false,
        "api_key": "",
        "max_results": 5
      },
      "searxng": {
        "enabled": false,
        "base_url": "http://localhost:8888",
        "max_results": 5
      }
    },
    "cron": {
      "exec_timeout_minutes": 5,
      "allow_command": true,
      "command_allowed_remotes": []
    }
  },
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

</details>

---

## 📝 So Sánh API Key

| Service          | Pricing                  | Use Case                              |
| ---------------- | ------------------------ | ------------------------------------- |
| **OpenRouter**   | Free: 200K tokens/month  | Multiple models (Claude, GPT-4, etc.) |
| **Volcengine CodingPlan** | ¥9.9/first month | Best for Chinese users, multiple SOTA models (Doubao, DeepSeek, etc.) |
| **Zhipu**        | Free: 200K tokens/month  | Suitable for Chinese users                |
| **Brave Search** | $5/1000 queries          | Web search functionality              |
| **SearXNG**      | Free (self-hosted)       | Privacy-focused metasearch (70+ engines) |
| **Groq**         | Free tier available      | Fast inference (Llama, Mixtral)       |
| **Cerebras**     | Free tier available      | Fast inference (Llama, Qwen, etc.)    |
| **LongCat**      | Free: up to 5M tokens/day | Fast inference                       |
| **ModelScope**   | Free: 2000 requests/day  | Inference (Qwen, GLM, DeepSeek, etc.) |

