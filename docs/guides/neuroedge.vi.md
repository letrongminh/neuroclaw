# NeuroClaw với NeuroEdge

> Quay lại [Hướng dẫn](README.md)

Một hộp nhà [NeuroEdge](https://github.com/letrongminh/neuroedge-init) điều khiển các thiết bị trong nhà sau một cổng (gate). NeuroClaw là bộ não: nó đọc cuộc trò chuyện của bạn, suy nghĩ và gọi các công cụ của hộp qua MCP. Hộp quyết định từng lệnh gọi theo quy tắc của ngôi nhà và, với bất cứ việc gì không thể hoàn tác, hỏi chủ nhà tại nhà.

## Kết nối tới hộp

Hộp phục vụ MCP qua HTTPS bằng `neuroedge mcp serve --http`. Nó yêu cầu một chứng chỉ client do chính nó ký. NeuroClaw xuất trình chứng chỉ đó bằng khối `tls` của một máy chủ MCP kiểu `http`, nên không cần cầu nối.

Trong `~/.neuroclaw/config.json`:

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

| Trường `tls` | Ý nghĩa |
|:---|:---|
| `ca_file` | CA dạng PEM đã ký chứng chỉ của hộp. Chỉ CA này được tin. |
| `cert_file`, `key_file` | Chứng chỉ và khóa client của bộ não. Đặt cả hai hoặc không đặt cái nào. |
| `server_name` | Tên cần kiểm tra trong chứng chỉ của hộp, nếu khác với host trong `url`. |

Quy tắc từ `pkg/mcp/tls.go`: `url` phải là `https` khi đặt `tls`; tối thiểu là TLS 1.3; đường dẫn chấp nhận `~/`. Kiểm tra bằng:

```bash
neuroclaw mcp test neuroedge
```

## Mô hình được dặn gì

Một công cụ của hộp trả về kết quả kèm trạng thái. NeuroClaw biến kết quả không phải `ALLOW` thuần thành các chỉ dẫn cố định cho mô hình (`pkg/tools/integration/neuroedge_result.go`), nên câu trả lời giống nhau trên mọi mô hình.

| Trạng thái | Mô hình được dặn |
|:---|:---|
| `BLOCK` kèm yêu cầu xác nhận | Hộp chưa thực hiện. Chủ nhà phải phê duyệt trên trang NeuroEdge tại nhà trong khoảng số giây đã nêu. Mô hình không thể phê duyệt, và phê duyệt trong chat không được tính. Hãy báo người dùng rằng đang chờ; đừng gọi lại công cụ; đừng nói là đã xong. |
| `BLOCK` không có xác nhận | Hộp từ chối, kèm lý do và tiêu chí không đạt. Hãy báo người dùng rằng chưa thực hiện và vì sao; đừng thử lại với đối số khác hay tìm cách lách quy tắc. |
| `REJECTED` | Hộp từ chối chính lệnh gọi (công cụ không tồn tại hoặc đối số nằm ngoài schema). Không có gì được chạy. |

Kết quả từ các máy chủ MCP khác không bị đụng tới.

## Nhãn rủi ro

Khi khởi động, NeuroClaw ghi log `NeuroClaw risk=<name>` cho mỗi khả năng cho phép mô hình với tới ngoài cuộc trò chuyện, workspace của nó và các công cụ của hộp:

| Nhãn | Bật khi |
|:---|:---|
| `shell` | `tools.exec` (mặc định bật) |
| `cron_shell` | `tools.cron.allow_command` cùng với `exec` (mặc định bật) |
| `skill_install` | `tools.install_skill` hoặc `find_skills`, với `tools.skills` bật (mặc định bật) |
| `hardware_bus` | `tools.i2c`, `spi` hoặc `serial` (mặc định tắt) |
| `process_hooks` | một mục đang bật trong `hooks.processes` (mặc định tắt) |
| `usb_monitor` | `devices.monitor_usb` cùng với `devices` (mặc định tắt) |
| `stdio_mcp` | một máy chủ stdio trong `tools.mcp.servers` (mặc định tắt) |

Nhãn chỉ để thông báo; chúng không chặn gì. Ý nghĩa từng nhãn: [capabilities](../reference/capabilities.md).

## Cùng máy với hộp: đóng bộ não lại

Cổng của hộp chỉ bao quát các lệnh gọi đi qua nó. Một shell hoặc bus phần cứng trên máy cũng chạy hộp có thể với tới thiết bị mà bỏ qua cổng. Nếu bộ não dùng chung máy với hộp, hãy tắt các thứ đó:

```json
{
  "agents": { "defaults": { "restrict_to_workspace": true } },
  "tools": {
    "exec": { "enabled": false },
    "cron": { "allow_command": false },
    "install_skill": { "enabled": false },
    "find_skills": { "enabled": false },
    "skills": {
      "registries": {
        "clawhub": { "enabled": false },
        "github": { "enabled": false }
      }
    },
    "i2c": { "enabled": false },
    "spi": { "enabled": false },
    "serial": { "enabled": false }
  },
  "hooks": { "enabled": false },
  "devices": { "enabled": false }
}
```

Gộp nó vào cấu hình của bạn; giữ nguyên khối `mcp` ở trên. Khởi động lại NeuroClaw và kiểm tra rằng log không còn dòng `risk=` nào cho các mục này.

Ngoài ra, hãy chạy bộ não bằng một người dùng hệ điều hành riêng (xem unit systemd trong [hướng dẫn cài đặt](install.vi.md#systemd)) không có quyền truy cập vào các khóa của hộp ngoài chứng chỉ client của chính bộ não, và không truy cập các device node.

## Trên máy khác hoặc NeuroEdge Cloud

Khi bộ não chạy trên một máy khác, hoặc trên NeuroEdge Cloud, cổng vẫn quyết định mọi hành động lên thiết bị. Shell và các bus phần cứng chỉ với tới máy mà bộ não đang chạy, nên hãy để chúng theo nhu cầu của bạn.

## NeuroClaw-ESP

Cùng bộ não đó trên một chip ESP32-S3, với cổng nằm ngay trên chip: [esp32/README.md](../../esp32/README.md). Nó được gắn nhãn `risk=single_chip` vì bộ não và cổng dùng chung một không gian địa chỉ.
