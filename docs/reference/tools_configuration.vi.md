# 🔧 Cấu Hình Công Cụ

> Quay lại [README](../project/README.vi.md)

Cấu hình công cụ của NeuroClaw nằm trong trường `tools` của `config.json`.

## Cấu trúc thư mục

```json
{
  "tools": {
    "web": {
      ...
    },
    "mcp": {
      ...
    },
    "exec": {
      ...
    },
    "cron": {
      ...
    },
    "skills": {
      ...
    }
  }
}
```

## Lọc dữ liệu nhạy cảm

Trước khi kết quả công cụ được gửi tới LLM, NeuroClaw có thể lọc các giá trị nhạy cảm (khóa API, token, secret) khỏi đầu ra. Điều này ngăn LLM nhìn thấy chính thông tin xác thực của nó.

Xem [Lọc dữ liệu nhạy cảm](../security/sensitive_data_filtering.md) để có tài liệu đầy đủ.

| Cấu hình | Kiểu | Mặc định | Mô tả |
|----------|------|----------|-------|
| `filter_sensitive_data` | bool | `true` | Bật/tắt bộ lọc |
| `filter_min_length` | int | `8` | Độ dài nội dung tối thiểu để kích hoạt lọc |

## Công cụ Web

Các công cụ web được sử dụng để tìm kiếm và tải nội dung web.

### Web Fetcher
Cài đặt chung để tải và xử lý nội dung trang web.

| Cấu hình            | Kiểu   | Mặc định      | Mô tả                                                                                        |
|----------------------|--------|---------------|-----------------------------------------------------------------------------------------------|
| `enabled`            | bool   | true          | Bật khả năng tải trang web.                                                                   |
| `fetch_limit_bytes`  | int    | 10485760      | Kích thước tối đa của payload trang web cần tải, tính bằng byte (mặc định là 10MB).          |
| `format`             | string | "plaintext"   | Định dạng đầu ra của nội dung đã tải. Tùy chọn: `plaintext` hoặc `markdown` (khuyến nghị).   |

### Brave

| Cấu hình      | Kiểu   | Mặc định | Mô tả                     |
|----------------|--------|----------|----------------------------|
| `enabled`      | bool     | false    | Bật tìm kiếm Brave                                               |
| `api_key`      | string   | -        | Khóa API Brave Search                                            |
| `api_keys`     | string[] | -        | Nhiều khóa API Brave Search để xoay vòng (ưu tiên hơn `api_key`) |
| `max_results`  | int      | 5        | Số kết quả tối đa                                                |

### DuckDuckGo

| Cấu hình      | Kiểu | Mặc định | Mô tả                        |
|----------------|------|----------|-------------------------------|
| `enabled`      | bool | false    | Bật tìm kiếm DuckDuckGo      |
| `max_results`  | int  | 5        | Số kết quả tối đa            |

### Sogou

| Cấu hình      | Kiểu | Mặc định | Mô tả                        |
|----------------|------|----------|-------------------------------|
| `enabled`      | bool | true     | Bật tìm kiếm Sogou           |
| `max_results`  | int  | 5        | Số kết quả tối đa            |

### Gemini Google Search

Gemini search dùng Gemini với Google Search grounding. Nó trả về câu trả lời do AI tổng hợp kèm trích dẫn từ Google Search.

| Cấu hình      | Kiểu   | Mặc định             | Mô tả                             |
|---------------|--------|----------------------|-----------------------------------|
| `enabled`     | bool   | false                | Bật Gemini Google Search          |
| `api_key`     | string | -                    | Khóa API Google Gemini            |
| `model`       | string | `gemini-2.5-flash`   | Model Gemini dùng để tìm kiếm     |
| `max_results` | int    | 5                    | Số trích dẫn tối đa               |

```json
{
  "tools": {
    "web": {
      "gemini": {
        "enabled": true,
        "api_key": "YOUR_GEMINI_API_KEY",
        "model": "gemini-2.5-flash",
        "max_results": 5
      }
    }
  }
}
```

### Baidu Search

Baidu Search dùng [Qianfan AI Search API](https://cloud.baidu.com/doc/qianfan-api/s/Wmbq4z7e5), được hỗ trợ bởi AI và tối ưu cho truy vấn tiếng Trung.

| Cấu hình      | Kiểu   | Mặc định                                                        | Mô tả                              |
|----------------|--------|-----------------------------------------------------------------|------------------------------------|
| `enabled`      | bool   | false                                                           | Bật tìm kiếm Baidu                |
| `api_key`      | string | -                                                               | Khóa API Qianfan                   |
| `base_url`     | string | `https://qianfan.baidubce.com/v2/ai_search/web_search`         | URL API Baidu Search               |
| `max_results`  | int    | 10                                                              | Số kết quả tối đa                 |

```json
{
  "tools": {
    "web": {
      "baidu_search": {
        "enabled": true,
        "api_key": "YOUR_BAIDU_QIANFAN_API_KEY",
        "max_results": 10
      }
    }
  }
}
```

### Perplexity

| Cấu hình      | Kiểu   | Mặc định | Mô tả                        |
|----------------|--------|----------|-------------------------------|
| `enabled`      | bool     | false    | Bật tìm kiếm Perplexity                                          |
| `api_key`      | string   | -        | Khóa API Perplexity                                              |
| `api_keys`     | string[] | -        | Nhiều khóa API Perplexity để xoay vòng (ưu tiên hơn `api_key`)  |
| `max_results`  | int      | 5        | Số kết quả tối đa                                                |

### Kagi Search

Kagi Search dùng client OpenAPI chính thức của Kagi cho `POST /search` và trả về kết quả web thông thường từ `data.search`.

| Cấu hình      | Kiểu     | Mặc định                              | Mô tả                                          |
|---------------|----------|---------------------------------------|------------------------------------------------|
| `enabled`     | bool     | false                                 | Bật Kagi Search                                |
| `api_key`     | string   | -                                     | Khóa API Kagi                                  |
| `api_keys`    | string[] | -                                     | Nhiều khóa API để xoay vòng (ưu tiên hơn `api_key`) |
| `base_url`    | string   | `https://kagi.com/api/v1/search`      | Endpoint API Kagi Search                       |
| `max_results` | int      | 5                                     | Số kết quả tối đa                              |

```json
{
  "tools": {
    "web": {
      "provider": "kagi",
      "kagi": {
        "enabled": true,
        "max_results": 5,
        "base_url": "https://kagi.com/api/v1/search"
      }
    }
  }
}
```

Lưu khóa API Kagi trong `.security.yml`:

```yaml
web:
  kagi:
    api_keys:
      - "YOUR_KAGI_API_KEY"
```

Việc dùng API Kagi có thể được tính phí hoặc giới hạn riêng so với gói đăng ký Kagi thông thường, tùy tài khoản và thiết lập API của bạn.

### Tavily

| Cấu hình      | Kiểu   | Mặc định | Mô tả                              |
|----------------|--------|----------|------------------------------------|
| `enabled`      | bool   | false    | Bật tìm kiếm Tavily               |
| `api_key`      | string | -        | Khóa API Tavily                    |
| `base_url`     | string | -        | URL cơ sở Tavily tùy chỉnh        |
| `max_results`  | int    | 5        | Số kết quả tối đa                 |

### SearXNG

| Cấu hình      | Kiểu   | Mặc định                 | Mô tả                      |
|----------------|--------|--------------------------|----------------------------|
| `enabled`      | bool   | false                    | Bật tìm kiếm SearXNG       |
| `base_url`     | string | -                        | URL phiên bản SearXNG (ví dụ `http://localhost:8888`) |
| `max_results`  | int    | 5                        | Số kết quả tối đa          |

### GLM Search

| Cấu hình        | Kiểu   | Mặc định                                             | Mô tả                      |
|------------------|--------|------------------------------------------------------|----------------------------|
| `enabled`        | bool   | false                                                | Bật GLM Search             |
| `api_key`        | string | -                                                    | Khóa API GLM               |
| `base_url`       | string | `https://open.bigmodel.cn/api/paas/v4/web_search`   | URL API GLM Search         |
| `search_engine`  | string | `search_std`                                         | Loại công cụ tìm kiếm      |
| `max_results`    | int    | 5                                                    | Số kết quả tối đa          |

### Cài đặt Web bổ sung

| Cấu hình                 | Kiểu     | Mặc định | Mô tả                                                                  |
|--------------------------|----------|----------|-------------------------------------------------------------------------|
| `provider`               | string   | `auto`   | Nhà cung cấp tìm kiếm sẽ dùng; `auto` chọn trong các công cụ đã bật    |
| `proxy`                  | string   | `""`     | Proxy cho các công cụ web                                               |
| `prefer_native`          | bool     | true     | Ưu tiên tìm kiếm gốc của provider hơn các công cụ tìm kiếm đã cấu hình |
| `private_host_whitelist` | string[] | `[]`     | Các host riêng/nội bộ được phép tải trang web                           |

### Tham số công cụ `web_search`

Khi chạy, công cụ `web_search` nhận các tham số sau:

| Trường | Kiểu | Bắt buộc | Mô tả |
|--------|------|----------|-------|
| `query` | string | có | Chuỗi truy vấn tìm kiếm |
| `count` | integer | không | Số kết quả trả về. Mặc định: `10`, tối đa: `10` |
| `range` | string | không | Bộ lọc thời gian tùy chọn: `d` (ngày), `w` (tuần), `m` (tháng), `y` (năm) |

Nếu bỏ qua `range`, NeuroClaw tìm kiếm không giới hạn.
Với Kagi, `d`, `w` và `m` ánh xạ sang lens `time_relative` của Kagi; `y` ánh xạ sang lens `time_after` là ngày cách ngày hiện tại một năm.

### Ví dụ gọi `web_search`

```json
{
  "query": "ai agent news",
  "count": 10,
  "range": "w"
}
```

## Công cụ Exec

Công cụ exec được sử dụng để thực thi các lệnh shell.

| Cấu hình               | Kiểu  | Mặc định | Mô tả                                         |
|--------------------------|-------|----------|------------------------------------------------|
| `enabled`                | bool  | true     | Bật công cụ exec. Khi bật, nó được báo khi khởi động là `risk=shell` |
| `enable_deny_patterns`   | bool  | true     | Bật chặn lệnh nguy hiểm mặc định             |
| `custom_deny_patterns`   | array | []       | Mẫu từ chối tùy chỉnh (biểu thức chính quy)  |
| `custom_allow_patterns`  | array | []       | Mẫu cho phép tùy chỉnh (biểu thức chính quy) |
| `allow_remote`           | bool  | true     | Cho phép các lệnh gọi exec đến từ kênh từ xa (ứng dụng chat); `false` giới hạn exec ở các kênh nội bộ |
| `timeout_seconds`        | int   | 60       | Thời gian chờ mỗi lệnh tính bằng giây; `0` dùng mặc định (60) |

### Vô hiệu hóa Công cụ Exec

Để hoàn toàn vô hiệu hóa công cụ `exec`, đặt `enabled` thành `false`:

**Qua tệp cấu hình:**
```json
{
  "tools": {
    "exec": {
      "enabled": false
    }
  }
}
```

**Qua biến môi trường:**
```bash
NEUROCLAW_TOOLS_EXEC_ENABLED=false
```

> **Lưu ý:** Khi bị vô hiệu hóa, agent sẽ không thể thực thi lệnh shell. Điều này cũng ảnh hưởng đến khả năng chạy lệnh shell theo lịch của công cụ Cron.

### Chức năng

- **`enable_deny_patterns`**: Đặt thành `false` để tắt hoàn toàn các mẫu chặn lệnh nguy hiểm mặc định
- **`custom_deny_patterns`**: Thêm các mẫu regex từ chối tùy chỉnh; các lệnh khớp sẽ bị chặn
- **`custom_allow_patterns`**: Thêm các mẫu regex cho phép; các lệnh khớp được miễn khỏi các mẫu từ chối
- **`allow_remote`**: Đặt thành `false` để từ chối các lệnh gọi exec đến từ kênh từ xa

### Các mẫu lệnh bị chặn mặc định

Theo mặc định, NeuroClaw chặn các lệnh nguy hiểm sau:

- Lệnh xóa: `rm -rf`, `del /f/q`, `rmdir /s`
- Thao tác đĩa: `format`, `mkfs`, `diskpart`, `dd if=`, ghi vào `/dev/sd*`
- Thao tác hệ thống: `shutdown`, `reboot`, `poweroff`
- Thay thế lệnh: `$()`, `${}`, dấu backtick
- Pipe đến shell: `| sh`, `| bash`
- Leo thang đặc quyền: `sudo`, `chmod`, `chown`
- Điều khiển tiến trình: `pkill`, `killall`, `kill -9`
- Thao tác từ xa: `curl | sh`, `wget | sh`, `ssh`
- Quản lý gói: `apt`, `yum`, `dnf`, `npm install -g`, `pip install --user`
- Container: `docker run`, `docker exec`
- Git: `git push`, `git force`
- Khác: `eval`, `source *.sh`

### Hạn chế kiến trúc đã biết

Bộ bảo vệ exec chỉ xác thực lệnh cấp cao nhất được gửi đến NeuroClaw. Nó **không** kiểm tra đệ quy các tiến trình con được tạo bởi các công cụ build hoặc script sau khi lệnh đó bắt đầu chạy.

Ví dụ về các quy trình có thể bỏ qua bộ bảo vệ lệnh trực tiếp sau khi lệnh ban đầu được cho phép:

- `make run`
- `go run ./cmd/...`
- `cargo run`
- `npm run build`

Điều này có nghĩa là bộ bảo vệ hữu ích để chặn các lệnh trực tiếp rõ ràng nguy hiểm, nhưng nó **không phải** là sandbox đầy đủ cho các pipeline build chưa được xem xét. Nếu mô hình mối đe dọa của bạn bao gồm mã không đáng tin cậy trong workspace, hãy sử dụng cách ly mạnh hơn như container, VM hoặc quy trình phê duyệt xung quanh các lệnh build và chạy.

### Ví dụ cấu hình

```json
{
  "tools": {
    "exec": {
      "enable_deny_patterns": true,
      "custom_deny_patterns": [
        "\\brm\\s+-r\\b",
        "\\bkillall\\s+python"
      ]
    }
  }
}
```

## Công cụ Cron

Công cụ cron được sử dụng để lên lịch các tác vụ định kỳ.

| Cấu hình               | Kiểu | Mặc định | Mô tả                                              |
|--------------------------|------|----------|-----------------------------------------------------|
| `enabled`                | bool | true     | Đăng ký công cụ cron dành cho agent                 |
| `allow_command`          | bool | true     | Cho phép tác vụ dạng lệnh mà không cần xác nhận thêm. Khi `exec` bật, nó được báo khi khởi động là `risk=cron_shell` |
| `command_allowed_remotes` | array | []      | Các kênh từ xa hoặc giá trị `channel:chat_id` được phép chạy tác vụ lệnh; `*` cho phép mọi kênh và nguy hiểm trừ khi mọi kênh từ xa đều đáng tin |
| `exec_timeout_minutes`   | int  | 5        | Thời gian chờ thực thi tính bằng phút, 0 nghĩa là không giới hạn |

Về các loại lịch, chế độ thực thi (`deliver`, lượt agent và tác vụ lệnh), lưu trữ và các cổng bảo mật lệnh hiện tại, xem [Tác vụ đã lên lịch và Cron](cron.md).

## Công tắc từng công cụ

Mọi công cụ khác đều có công tắc `enabled` riêng dưới `tools.<name>` (biến môi trường `NEUROCLAW_TOOLS_<NAME>_ENABLED`). Giá trị mặc định lấy từ `pkg/config/defaults.go`:

| Công cụ | Mặc định | Ghi chú |
|---------|----------|---------|
| `read_file`, `write_file`, `append_file`, `edit_file`, `list_dir` | bật | `read_file` còn có `mode` (`bytes` hoặc `lines`, mặc định `bytes`) và `max_read_file_size` (mặc định 65536) |
| `load_image`, `send_file`, `web_fetch` | bật | |
| `message` | bật | `media_enabled` mặc định tắt |
| `spawn`, `subagent` | bật | |
| `spawn_status` | tắt | |
| `find_skills`, `install_skill` | bật | Chỉ có hiệu lực khi `tools.skills.enabled`; được báo là `risk=skill_install` |
| `send_tts` | tắt | |
| `i2c`, `spi`, `serial` | tắt | Công cụ phần cứng; `i2c` và `spi` chỉ dành cho Linux. Khi bật được báo là `risk=hardware_bus` |
| `media_cleanup` | bật | `max_age_minutes` 30, `interval_minutes` 5 |

Các nhãn rủi ro và ý nghĩa của từng nhãn nằm ở [NeuroClaw làm được gì, mục 2](capabilities.md#2-risk-labels).

<a id="mcp-tool"></a>
## Công cụ MCP

Công cụ MCP cho phép tích hợp với các máy chủ Model Context Protocol bên ngoài.

Nếu bạn không muốn sửa JSON thủ công, NeuroClaw cũng cung cấp CLI quản lý cấu hình MCP:

- `neuroclaw mcp add` — thêm hoặc cập nhật một máy chủ (hỗ trợ `--deferred` / `--no-deferred`)
- `neuroclaw mcp list` — liệt kê mọi máy chủ đã cấu hình kèm trạng thái và trạng thái deferred
- `neuroclaw mcp show <name>` — hiển thị chi tiết đầy đủ và danh sách công cụ của một máy chủ
- `neuroclaw mcp test <name>` — kiểm tra kết nối cho một máy chủ
- `neuroclaw mcp remove <name>` — xóa một mục máy chủ
- `neuroclaw mcp edit` — mở `config.json` trong `$EDITOR` để chỉnh sửa nâng cao

Các lệnh này quản lý cùng phần `tools.mcp.servers` được mô tả bên dưới. Xem [MCP Server CLI](mcp-cli.md).

### Khám phá công cụ (tải chậm)

Khi kết nối với nhiều máy chủ MCP, việc hiển thị hàng trăm công cụ cùng lúc có thể làm cạn kiệt cửa sổ ngữ cảnh của LLM và tăng chi phí API. Tính năng **Discovery** giải quyết vấn đề này bằng cách giữ các công cụ MCP *ẩn* theo mặc định.

Thay vì tải tất cả các công cụ, LLM được cung cấp một công cụ tìm kiếm nhẹ (sử dụng khớp từ khóa BM25 hoặc Regex). Khi LLM cần một khả năng cụ thể, nó tìm kiếm trong thư viện ẩn. Các công cụ khớp sau đó được tạm thời "mở khóa" và đưa vào ngữ cảnh trong số lượt được cấu hình (`ttl`).

### Cấu hình toàn cục

| Cấu hình    | Kiểu   | Mặc định | Mô tả                                        |
|-------------|--------|----------|-----------------------------------------------|
| `enabled`   | bool   | false    | Bật tích hợp MCP toàn cục                    |
| `discovery` | object | `{}`     | Cấu hình khám phá công cụ (xem bên dưới)    |
| `servers`   | object | `{}`     | Ánh xạ tên máy chủ đến cấu hình máy chủ     |

### Cấu hình Discovery (`discovery`)

| Cấu hình             | Kiểu | Mặc định | Mô tả                                                                                                                            |
|----------------------|------|----------|-----------------------------------------------------------------------------------------------------------------------------------|
| `enabled`            | bool | false    | Nếu true, các công cụ MCP bị ẩn và được tải theo yêu cầu qua tìm kiếm. Nếu false, tất cả công cụ được tải                      |
| `ttl`                | int  | 5        | Số lượt hội thoại mà một công cụ đã khám phá vẫn được mở khóa                                                                   |
| `max_search_results` | int  | 5        | Số công cụ tối đa được trả về cho mỗi truy vấn tìm kiếm                                                                         |
| `use_bm25`           | bool | true     | Bật công cụ tìm kiếm ngôn ngữ tự nhiên/từ khóa (`tool_search_tool_bm25`). **Cảnh báo**: tiêu tốn nhiều tài nguyên hơn tìm kiếm regex |
| `use_regex`          | bool | false    | Bật công cụ tìm kiếm mẫu regex (`tool_search_tool_regex`)                                                                        |

> **Lưu ý:** Nếu `discovery.enabled` là `true`, bạn **phải** bật ít nhất một công cụ tìm kiếm (`use_bm25` hoặc `use_regex`),
> nếu không ứng dụng sẽ không khởi động được.

### Cấu hình từng máy chủ

| Cấu hình   | Kiểu   | Bắt buộc | Mô tả                                     |
|------------|--------|----------|--------------------------------------------|
| `enabled`  | bool   | có       | Bật máy chủ MCP này                       |
| `deferred` | bool   | không    | Ghi đè chế độ deferred chỉ cho máy chủ này. `true` = công cụ bị ẩn và được tìm qua search; `false` = công cụ luôn hiển thị trong ngữ cảnh. Khi bỏ qua, áp dụng giá trị `discovery.enabled` toàn cục. |
| `type`     | string | không    | Loại truyền tải: `stdio`, `sse`, `http` (`streamable-http` được chấp nhận như tên khác của `http`) |
| `command`  | string | stdio    | Lệnh thực thi cho truyền tải stdio        |
| `args`     | array  | không    | Đối số lệnh cho truyền tải stdio          |
| `env`      | object | không    | Biến môi trường cho tiến trình stdio      |
| `env_file` | string | không    | Đường dẫn đến tệp môi trường cho tiến trình stdio |
| `url`      | string | sse/http | URL endpoint cho truyền tải `sse`/`http`  |
| `headers`  | object | không    | Header HTTP cho truyền tải `sse`/`http`   |
| `tls`      | object | không    | NeuroClaw: mutual TLS cho truyền tải `sse`/`http` (xem bên dưới) |

### Mutual TLS (`tls`)

Phần bổ sung của NeuroClaw. Một hộp nhà NeuroEdge khởi động bằng `mcp serve --http` từ chối mọi client không có chứng chỉ do nó ký. Đặt `tls` trên máy chủ `http` hoặc `sse` để brain xuất trình chứng chỉ client và chỉ tin CA của hộp, không cần cầu nối TLS bên cạnh.

| Cấu hình      | Kiểu   | Mô tả |
|---------------|--------|-------|
| `ca_file`     | string | Tệp PEM của CA đã ký chứng chỉ máy chủ. Để trống: dùng root hệ thống |
| `cert_file`   | string | Tệp PEM chứng chỉ của client này. Đặt cùng `key_file`, hoặc không đặt cả hai |
| `key_file`    | string | Tệp PEM khóa riêng của client này |
| `server_name` | string | Ghi đè tên được kiểm tra trong chứng chỉ máy chủ |

Đường dẫn có thể bắt đầu bằng `~`. Quy tắc, theo `pkg/mcp/tls.go`:

- `url` phải là `https://`; nếu không máy chủ sẽ không kết nối được với lỗi `tls is set but url ... is not https`.
- Kết nối dùng TLS 1.3 trở lên.
- Không có `tls`, transport HTTP mặc định được dùng, như trong PicoClaw.
- `tls` bị bỏ qua với máy chủ `stdio`.
- `neuroclaw mcp add` không có cờ cho `tls`; hãy thêm bằng cách sửa `config.json` (`neuroclaw mcp edit`). Chạy lại `mcp add` với cùng tên sẽ thay mục đó và làm mất khối `tls`.

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "servers": {
        "home": {
          "enabled": true,
          "type": "http",
          "url": "https://neuroedge-box.local:8443/mcp",
          "tls": {
            "ca_file": "~/.neuroclaw/pki/ca.pem",
            "cert_file": "~/.neuroclaw/pki/brain.pem",
            "key_file": "~/.neuroclaw/pki/brain.key",
            "server_name": "neuroedge-box.local"
          }
        }
      }
    }
  }
}
```

Xem [hướng dẫn NeuroEdge](../guides/neuroedge.vi.md) cho phía hộp nhà.

### Hành vi truyền tải

- Nếu bỏ qua `type`, truyền tải được tự động phát hiện:
    - `url` được đặt → `sse`
    - `command` được đặt → `stdio`
- `http` và `sse` đều sử dụng `url` + `headers` tùy chọn + `tls` tùy chọn.
- `neuroclaw mcp add` mặc định dùng `stdio` (xem [MCP Server CLI](mcp-cli.md)); việc tự phát hiện này chỉ áp dụng cho các mục viết tay.
- Một máy chủ `stdio` đang bật được báo khi khởi động là `risk=stdio_mcp`.
- `env` và `env_file` chỉ được áp dụng cho máy chủ `stdio`.

### Ví dụ cấu hình

#### 1) Máy chủ MCP Stdio

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "servers": {
        "filesystem": {
          "enabled": true,
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-filesystem",
            "/tmp"
          ]
        }
      }
    }
  }
}
```

#### 2) Máy chủ MCP từ xa SSE/HTTP

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "servers": {
        "remote-mcp": {
          "enabled": true,
          "type": "sse",
          "url": "https://example.com/mcp",
          "headers": {
            "Authorization": "Bearer YOUR_TOKEN"
          }
        }
      }
    }
  }
}
```

#### 3) Thiết lập MCP quy mô lớn với khám phá công cụ được bật

*Trong ví dụ này, LLM chỉ thấy `tool_search_tool_bm25`. Nó sẽ tìm kiếm và mở khóa động các công cụ Github hoặc Postgres chỉ khi được người dùng yêu cầu.*

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "discovery": {
        "enabled": true,
        "ttl": 5,
        "max_search_results": 5,
        "use_bm25": true,
        "use_regex": false
      },
      "servers": {
        "github": {
          "enabled": true,
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-github"
          ],
          "env": {
            "GITHUB_PERSONAL_ACCESS_TOKEN": "YOUR_GITHUB_TOKEN"
          }
        },
        "postgres": {
          "enabled": true,
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-postgres",
            "postgresql://user:password@localhost/dbname"
          ]
        },
        "slack": {
          "enabled": true,
          "type": "slack",
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-slack"
          ],
          "env": {
            "SLACK_BOT_TOKEN": "YOUR_SLACK_BOT_TOKEN",
            "SLACK_TEAM_ID": "YOUR_SLACK_TEAM_ID"
          }
        }
      }
    }
  }
}
```

#### 4) Thiết lập hỗn hợp: ghi đè deferred theo từng máy chủ

*Discovery được bật toàn cục, nhưng `filesystem` được ghim luôn hiển thị còn `context7` theo mặc định toàn cục
(deferred). `aws` chủ động chọn chế độ deferred dù nó trùng với mặc định toàn cục.*

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "discovery": {
        "enabled": true,
        "ttl": 5,
        "max_search_results": 5,
        "use_bm25": true
      },
      "servers": {
        "filesystem": {
          "enabled": true,
          "command": "npx",
          "args": ["-y", "@modelcontextprotocol/server-filesystem", "/workspace"],
          "deferred": false
        },
        "context7": {
          "enabled": true,
          "command": "npx",
          "args": ["-y", "@upstash/context7-mcp"]
        },
        "aws": {
          "enabled": true,
          "command": "npx",
          "args": ["-y", "aws-mcp-server"],
          "deferred": true
        }
      }
    }
  }
}
```

> **Mẹo:** `deferred` theo từng máy chủ độc lập với `discovery.enabled`. Bạn có thể giữ
> `discovery.enabled: false` toàn cục (mọi công cụ hiển thị theo mặc định) mà vẫn đánh dấu từng máy chủ
> có nhiều công cụ là `"deferred": true` để tránh làm rối ngữ cảnh bằng các công cụ của chúng.

<a id="skills-tool"></a>
## Công cụ Skills

Công cụ skills cấu hình khám phá và cài đặt skill thông qua các registry như ClawHub và GitHub.

### Registry

| Cấu hình                           | Kiểu   | Mặc định             | Mô tả                                        |
|------------------------------------|--------|----------------------|----------------------------------------------|
| `registries.clawhub.enabled`       | bool   | true                 | Bật registry ClawHub                         |
| `registries.clawhub.base_url`      | string | `https://clawhub.ai` | URL cơ sở ClawHub                            |
| `registries.clawhub.auth_token`    | string | `""`                 | Token Bearer tùy chọn để có giới hạn tốc độ cao hơn |
| `registries.clawhub.search_path`   | string | `""`                 | Đường dẫn API tìm kiếm                       |
| `registries.clawhub.skills_path`   | string | `""`                 | Đường dẫn API Skills                         |
| `registries.clawhub.download_path` | string | `""`                 | Đường dẫn API tải xuống                      |
| `registries.clawhub.timeout`       | int    | 0                    | Timeout yêu cầu tính bằng giây (0 = mặc định) |
| `registries.clawhub.max_zip_size`  | int    | 0                    | Kích thước tối đa của zip skill, tính bằng byte (0 = mặc định) |
| `registries.clawhub.max_response_size` | int | 0                   | Kích thước phản hồi API tối đa, tính bằng byte (0 = mặc định) |
| `registries.github.enabled`        | bool   | true                 | Bật cài đặt từ GitHub qua cấu hình registry   |
| `registries.github.base_url`       | string | `https://github.com` | URL cơ sở GitHub hoặc GitHub Enterprise       |
| `registries.github.auth_token`     | string | `""`                 | Personal access token của GitHub              |
| `registries.github.proxy`          | string | `""`                 | Proxy HTTP cho các yêu cầu API GitHub         |

### Cấu hình GitHub cũ (legacy)

`github.*` đã bị deprecated. Hãy dùng `registries.github.*`. Các trường cũ vẫn được hỗ trợ để tương thích và sẽ bị loại bỏ sau này.

| Cấu hình           | Kiểu   | Mặc định             | Mô tả                          |
|--------------------|--------|----------------------|--------------------------------|
| `github.base_url`  | string | `https://github.com` | URL cơ sở GitHub (deprecated)  |
| `github.proxy`     | string | `""`                 | Proxy GitHub (deprecated)      |
| `github.token`     | string | `""`                 | Token GitHub (deprecated)      |

### Cài đặt tìm kiếm

| Cấu hình                  | Kiểu | Mặc định | Mô tả                                      |
|---------------------------|------|----------|--------------------------------------------|
| `max_concurrent_searches` | int  | 2        | Số yêu cầu tìm kiếm skill đồng thời tối đa |
| `search_cache.max_size`   | int  | 50       | Số kết quả tìm kiếm tối đa được cache      |
| `search_cache.ttl_seconds`| int  | 300      | TTL của cache tính bằng giây               |

### Ví dụ cấu hình

```json
{
  "tools": {
    "skills": {
      "registries": {
        "clawhub": {
          "enabled": true,
          "base_url": "https://clawhub.ai",
          "auth_token": "",
          "search_path": "",
          "skills_path": "",
          "download_path": "",
          "timeout": 0,
          "max_zip_size": 0,
          "max_response_size": 0
        },
        "github": {
          "enabled": true,
          "base_url": "https://github.com",
          "auth_token": "",
          "proxy": ""
        }
      },
      "github": {
        "base_url": "https://github.com",
        "proxy": "",
        "token": ""
      },
      "max_concurrent_searches": 2,
      "search_cache": {
        "max_size": 50,
        "ttl_seconds": 300
      }
    }
  }
}
```

## Biến môi trường

Tất cả các tùy chọn cấu hình có thể được ghi đè qua biến môi trường với định dạng `NEUROCLAW_TOOLS_<SECTION>_<KEY>`:

Ví dụ:

- `NEUROCLAW_TOOLS_WEB_BRAVE_ENABLED=true`
- `NEUROCLAW_TOOLS_EXEC_ENABLED=false`
- `NEUROCLAW_TOOLS_EXEC_ENABLE_DENY_PATTERNS=false`
- `NEUROCLAW_TOOLS_CRON_EXEC_TIMEOUT_MINUTES=10`
- `NEUROCLAW_TOOLS_MCP_ENABLED=true`
- `NEUROCLAW_TOOLS_MCP_MAX_INLINE_TEXT_CHARS=16384`

Lưu ý: Cấu hình kiểu map lồng nhau (ví dụ `tools.mcp.servers.<name>.*`) được cấu hình trong `config.json` thay vì qua biến môi trường.

Với các công cụ MCP, `tools.mcp.max_inline_text_chars` kiểm soát lượng văn bản kết quả được giữ nguyên trong ngữ cảnh của mô hình. Ngưỡng được đếm theo ký tự Unicode (rune của Go), không phải byte. Ví dụ, `16384` nghĩa là tối đa 16.384 ký tự nằm trong ngữ cảnh, có thể chiếm hơn 16 KB với văn bản đa byte như tiếng Trung, Nhật, Hàn. Vượt ngưỡng này, NeuroClaw lưu kết quả văn bản MCP thành một artifact cục bộ trong workspace của agent và đưa cho mô hình một ghi chú ngắn cùng đường dẫn artifact có cấu trúc `[file:...]` thay vì chèn toàn bộ payload vào ngữ cảnh.
