# NeuroClaw 🦞

Mọi thứ [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 làm được — ứng dụng chat, nhà cung cấp LLM,
bộ nhớ, nhắc việc, shell, skill, MCP, web console và launcher — và là bộ não của một ngôi nhà
[NeuroEdge](https://github.com/letrongminh/neuroedge-init). Khi nối với một hộp nhà NeuroEdge,
nó với tới các thiết bị trong nhà qua các công cụ MCP của hộp; hộp quyết định theo quy tắc của ngôi nhà và,
với bất cứ việc gì không thể hoàn tác, sẽ hỏi bạn tại nhà trước. Mọi thứ cho phép mô hình với tới ngoài
cuộc trò chuyện (shell, bus phần cứng, cài skill, hook, máy chủ MCP stdio) được bật hoặc tắt theo cấu hình
của bạn, và mỗi thứ đang bật đều được nêu tên khi khởi động (`risk=…`).

Chạy nó trên bo mạch Linux, máy chủ (NeuroEdge Cloud), máy Mac hoặc PC Windows; các bản phát hành bao phủ
cùng các nền tảng như PicoClaw. Cài đặt: [docs/guides/install.vi.md](../guides/install.vi.md).

Hoặc ngay trên chip: [`esp32/`](../../esp32/) là **NeuroClaw-ESP**, cùng bộ não đó được viết lại bằng C
thành một component ESP-IDF cho một ESP32-S3 — Telegram, LLM (có dự phòng), bộ nhớ, nhắc việc,
heartbeat, tìm kiếm web, tin nhắn và tệp gửi cho chủ nhà, và cả cổng (gate) trên một con chip vài đô la,
với cùng workspace và công cụ như ở đây ([giống gì, khác gì](../../esp32/README.md)), được gắn nhãn
`risk=single_chip` vì khi đó bộ não và cổng dùng chung một không gian địa chỉ.

NeuroClaw là một hard fork của [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 (MIT). Xem
[NOTICE](../../NOTICE).

## NeuroClaw khác PicoClaw ở đâu

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| MCP qua HTTP | Chỉ header | Có cả mutual TLS: `tls.ca_file`, `cert_file`, `key_file`, `server_name`, TLS 1.3. Một hộp NeuroEdge (`neuroedge mcp serve --http`) yêu cầu chứng chỉ client, nên không cần cầu nối. |
| Shell, phần cứng, cài skill, hook, MCP stdio | Bật hoặc tắt theo cấu hình | Giống vậy, với mặc định của PicoClaw; mỗi thứ đang bật được ghi log khi khởi động dưới dạng `NeuroClaw risk=<name>` (`shell`, `cron_shell`, `hardware_bus`, `skill_install`, `process_hooks`, `usb_monitor`, `stdio_mcp`; [`pkg/config/risk.go`](../../pkg/config/risk.go)). Một ngôi nhà NeuroEdge muốn đóng bộ não của mình thì nói rõ trong `config.json`. |
| Cổng nói "hãy hỏi chủ nhà" | Chỉ là một kết quả công cụ bình thường | Khi một công cụ của hộp trả về `BLOCK` kèm `confirmation`, NeuroClaw nói thẳng với mô hình rằng chưa có gì được chạy, rằng chỉ chủ nhà tại nhà mới phê duyệt được, và không được thử lại ([`pkg/tools/integration/neuroedge_result.go`](../../pkg/tools/integration/neuroedge_result.go)). |
| `update` | Tải các bản phát hành của `sipeed/picoclaw` | Tải các bản phát hành của kho này, được kiểm tra theo checksum |
| Tên | `picoclaw`, `~/.picoclaw`, `PICOCLAW_*` | `neuroclaw`, `~/.neuroclaw`, `NEUROCLAW_*` |

Mọi thứ còn lại là của PicoClaw: kênh chat (Telegram và nhiều hơn), nhà cung cấp LLM, bộ nhớ và
phiên, cron và heartbeat, tìm kiếm web, tin nhắn, skill dựng sẵn, subagent, web console và
launcher (`neuroclaw-launcher`), image Docker (`ghcr.io/letrongminh/neuroclaw`), bản dựng hằng đêm,
gói deb/rpm và DMG cho macOS. Các hướng dẫn của nó
nằm trong [`docs/`](../) ([README gốc](PICOCLAW_README.md)); ở đó hãy đọc `picoclaw` thành
`neuroclaw`.

**[NeuroClaw làm được gì](../reference/capabilities.md)** liệt kê mọi khả năng của cả hai bộ não, cái gì
bật theo mặc định, mỗi nhãn rủi ro có nghĩa gì, và những gì đã biết là chưa đúng.

## Kết nối với một hộp NeuroEdge

Hướng dẫn: [docs/guides/neuroedge.vi.md](../guides/neuroedge.vi.md).

Hộp chạy `neuroedge mcp serve --http --ui …`; khóa của nó lấy từ
`demo/i2f-chat-den-cua/tao-khoa.py` của NeuroEdge. Trong `~/.neuroclaw/config.json`:

```json
"tools": {
  "mcp": {
    "enabled": true,
    "servers": {
      "neuroedge": {
        "enabled": true,
        "type": "http",
        "url": "https://home-box.example:8443/mcp",
        "headers": { "Authorization": "Bearer <the brain's token>" },
        "tls": {
          "ca_file": "~/neuroedge-keys/ca.pem",
          "cert_file": "~/neuroedge-keys/nao.pem",
          "key_file": "~/neuroedge-keys/nao.key"
        }
      }
    }
  }
}
```

`neuroclaw mcp test neuroedge` phải in `reachable (3 tools)` với agent demo.

## Build

```bash
CGO_ENABLED=0 go build -tags goolm,stdjson -o neuroclaw ./cmd/neuroclaw
CGO_ENABLED=0 go test -tags goolm,stdjson ./...
```

`make build-launcher` dựng web console (`web/`, cần pnpm); `make build-all` và goreleaser
(`.goreleaser.yaml`) dựng mọi nền tảng. Trên macOS có ba test của công cụ shell lỗi giống như trong
PicoClaw v0.3.1; CI chạy trên Linux, nơi chúng đạt.

**Nâng cấp từ bản NeuroClaw trước khi có nhãn rủi ro** (2026-10-13): các bản đó ép tắt shell,
lệnh cron, cài skill và giám sát USB, và `onboard` lưu chúng ở trạng thái tắt trong
`config.json`. Hãy bật lại ở đó, hoặc bắt đầu lại từ mặc định bằng `neuroclaw config reset`.

## Giấy phép

MIT, như PicoClaw. Bản quyền (c) 2026 những người đóng góp cho PicoClaw và những người đóng góp cho NeuroClaw.
