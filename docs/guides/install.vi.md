# Cài đặt NeuroClaw

> Quay lại [Hướng dẫn](README.md)

NeuroClaw được phát hành dưới dạng một binary tĩnh, `neuroclaw`, và một binary web console, `neuroclaw-launcher`. Các bản phát hành nằm trên [GitHub](https://github.com/letrongminh/neuroclaw/releases). Cấu hình và workspace nằm trong `~/.neuroclaw` (đặt `NEUROCLAW_HOME` để chuyển đi nơi khác).

## Chọn tệp

| Nơi chạy | Tải về |
|:---|:---|
| Máy chủ Linux hoặc VM đám mây | `neuroclaw_Linux_x86_64.tar.gz`, `neuroclaw_Linux_arm64.tar.gz`, hoặc `.deb` / `.rpm` |
| Raspberry Pi và các bo mạch khác | `Linux_arm64` (HĐH 64-bit), `Linux_armv7`, `Linux_armv6` (Pi Zero/1), `Linux_riscv64`, `Linux_loong64`, `Linux_mipsle` (router soft-float), `Linux_s390x` |
| Docker | `ghcr.io/letrongminh/neuroclaw` (linux/amd64, arm64, riscv64): [hướng dẫn Docker](docker.vi.md) |
| macOS | `neuroclaw_Darwin_arm64.tar.gz`, `neuroclaw_Darwin_x86_64.tar.gz` |
| Windows | `neuroclaw_Windows_x86_64.zip`, `neuroclaw_Windows_arm64.zip` |
| FreeBSD, NetBSD | Các bản nén `Freebsd_*`, `Netbsd_*` |
| Android | Termux với `Linux_arm64`: [hướng dẫn Termux](android-termux.md) |
| Một chip ESP32-S3 | Không phải tải về. Xem [esp32/README.md](../../esp32/README.md) |

Các bản nén được đặt tên `neuroclaw_<OS>_<arch>.tar.gz` (`.zip` trên Windows). Mọi tệp đều được liệt kê trong `neuroclaw_<version>_checksums.txt` ở trang phát hành.

## Kiểm tra bản tải về

```bash
sha256sum --check --ignore-missing neuroclaw_*_checksums.txt     # Linux
shasum -a 256 --check --ignore-missing neuroclaw_*_checksums.txt # macOS
```

Trên Windows, so sánh `Get-FileHash <file>` với dòng tương ứng trong `neuroclaw_<version>_checksums.txt`.

## Chạy lần đầu

```bash
neuroclaw onboard          # writes ~/.neuroclaw/config.json and the workspace
vim ~/.neuroclaw/config.json   # add a provider key to model_list
neuroclaw agent -m "Hello" # one message; without -m it is interactive
neuroclaw gateway          # long-running: chat apps, cron, heartbeat
```

Xem [Cấu hình](configuration.vi.md#web-launcher-dashboard) và [Providers](providers.vi.md) để biết về cấu hình. `neuroclaw status` cho biết những gì đã được thiết lập. Khi khởi động, NeuroClaw ghi log `NeuroClaw risk=<name>` cho mỗi khả năng rủi ro đang bật; xem [capabilities](../reference/capabilities.md).

## Linux

```bash
tar xzf neuroclaw_Linux_x86_64.tar.gz
sudo install -m 0755 neuroclaw neuroclaw-launcher /usr/local/bin/
```

### deb / rpm

Các gói được đặt tên `neuroclaw_x86_64.deb`, `neuroclaw_aarch64.rpm` v.v. Chúng cài cả hai binary vào `/usr/bin`, cùng một mục desktop cho launcher.

```bash
sudo dpkg -i neuroclaw_x86_64.deb      # Debian, Ubuntu
sudo rpm -i neuroclaw_x86_64.rpm       # Fedora, RHEL
```

### systemd

Chạy gateway bằng một người dùng riêng. Trước hết hãy chạy `neuroclaw onboard` một lần bằng người dùng đó.

```ini
# /etc/systemd/system/neuroclaw.service
[Unit]
Description=NeuroClaw gateway
After=network-online.target
Wants=network-online.target

[Service]
User=neuroclaw
ExecStart=/usr/local/bin/neuroclaw gateway
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

```bash
sudo useradd --system --create-home neuroclaw
sudo -u neuroclaw neuroclaw onboard
sudo systemctl enable --now neuroclaw
journalctl -u neuroclaw -f
```

Gateway mặc định lắng nghe ở `localhost:18790`. `neuroclaw gateway --host 0.0.0.0` (hoặc `NEUROCLAW_GATEWAY_HOST`) đổi điều đó cho một lần chạy.

## Web console (launcher)

```bash
neuroclaw-launcher                 # http://localhost:18800, opens a browser
neuroclaw-launcher -console -no-browser
neuroclaw-launcher -public         # listen on all interfaces
```

Cờ: `-port` (mặc định 18800), `-host`, `-public`, `-no-browser`, `-console`, `-lang`, `-d`. Launcher khởi động và quản lý gateway. Đừng để lộ nó ra các mạng không đáng tin; xem [Cấu hình](configuration.vi.md#web-launcher-dashboard).

## Docker

```bash
docker compose -f docker/docker-compose.yml --profile gateway up       # first run: writes config, exits
docker compose -f docker/docker-compose.yml --profile gateway up -d
docker compose -f docker/docker-compose.yml --profile launcher up -d   # web console on :18800
docker compose -f docker/docker-compose.yml run --rm neuroclaw-agent -m "Hello"
```

Các dịch vụ: `neuroclaw-gateway` (profile `gateway`), `neuroclaw-launcher` (profile `launcher`), `neuroclaw-agent` (profile `agent`). Dữ liệu nằm trong `docker/data`. Chi tiết: [hướng dẫn Docker](docker.vi.md).

## macOS

Các bản nén không được ký hay công chứng (notarize). Sau khi giải nén, hãy xóa cờ cách ly (quarantine):

```bash
tar xzf neuroclaw_Darwin_arm64.tar.gz
xattr -d com.apple.quarantine neuroclaw neuroclaw-launcher
./neuroclaw onboard
```

## Windows

Giải nén `neuroclaw_Windows_x86_64.zip`. Nó chứa `neuroclaw.exe` và `neuroclaw-launcher.exe`. Các tệp không được ký mã, nên SmartScreen có thể cảnh báo: chọn "More info", rồi "Run anyway", chỉ sau khi bạn đã kiểm tra checksum. Công cụ shell chạy lệnh qua PowerShell.

```powershell
.\neuroclaw.exe onboard
.\neuroclaw.exe gateway
```

## Android (Termux)

Termux trên điện thoại ARM64 chạy bản nén `Linux_arm64` qua `termux-chroot`. Các bước đầy đủ: [hướng dẫn Termux](android-termux.md).

## Cập nhật

```bash
neuroclaw update
```

`neuroclaw update` tải bản phát hành mới nhất của kho này và kiểm tra bản nén với `neuroclaw_<version>_checksums.txt`. Cờ: `--url` (trang phát hành hoặc một asset), `--platform`, `--arch`. Các gói cài bằng `dpkg` hoặc `rpm` cũng có thể cập nhật bằng cách cài gói mới hơn. Docker: `docker compose pull`.

## Làm bộ não của một nhà NeuroEdge

Xem [NeuroClaw với NeuroEdge](neuroedge.vi.md).
