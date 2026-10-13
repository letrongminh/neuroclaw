# 安装 NeuroClaw

> 返回 [指南](README.md)

NeuroClaw 以一个静态二进制文件 `neuroclaw` 和一个 Web 控制台二进制文件 `neuroclaw-launcher` 的形式发布。发布版本在 [GitHub](https://github.com/letrongminh/neuroclaw/releases) 上。配置和工作区位于 `~/.neuroclaw`（设置 `NEUROCLAW_HOME` 可以移动它）。

## 选择文件

| 运行环境 | 下载 |
|:---|:---|
| Linux 服务器或云虚拟机 | `neuroclaw_Linux_x86_64.tar.gz`、`neuroclaw_Linux_arm64.tar.gz`，或 `.deb` / `.rpm` |
| 树莓派和其他开发板 | `Linux_arm64`（64 位系统）、`Linux_armv7`、`Linux_armv6`（Pi Zero/1）、`Linux_riscv64`、`Linux_loong64`、`Linux_mipsle`（软浮点路由器）、`Linux_s390x` |
| Docker | `ghcr.io/letrongminh/neuroclaw`（linux/amd64、arm64、riscv64）：[Docker 指南](docker.zh.md) |
| macOS | `neuroclaw_Darwin_arm64.tar.gz`、`neuroclaw_Darwin_x86_64.tar.gz` |
| Windows | `neuroclaw_Windows_x86_64.zip`、`neuroclaw_Windows_arm64.zip` |
| FreeBSD、NetBSD | `Freebsd_*`、`Netbsd_*` 压缩包 |
| Android | 在 Termux 中使用 `Linux_arm64`：[Termux 指南](android-termux.md)（英文） |
| 一颗 ESP32-S3 | 不是下载文件。见 [esp32/README.md](../../esp32/README.md) |

压缩包命名为 `neuroclaw_<OS>_<arch>.tar.gz`（Windows 为 `.zip`）。每个文件都列在发布页面的 `neuroclaw_<version>_checksums.txt` 中。

## 校验下载的文件

```bash
sha256sum --check --ignore-missing neuroclaw_*_checksums.txt     # Linux
shasum -a 256 --check --ignore-missing neuroclaw_*_checksums.txt # macOS
```

在 Windows 上，请把 `Get-FileHash <file>` 的结果与 `neuroclaw_<version>_checksums.txt` 中对应的那一行比较。

## 首次运行

```bash
neuroclaw onboard          # writes ~/.neuroclaw/config.json and the workspace
vim ~/.neuroclaw/config.json   # add a provider key to model_list
neuroclaw agent -m "Hello" # one message; without -m it is interactive
neuroclaw gateway          # long-running: chat apps, cron, heartbeat
```

配置请见[配置详解](configuration.zh.md)和[提供商](providers.zh.md)。`neuroclaw status` 会显示已经设置好的内容。启动时，NeuroClaw 会为每一项处于开启状态的高风险能力记录 `NeuroClaw risk=<name>`；见[能力说明](../reference/capabilities.md)。

## Linux

```bash
tar xzf neuroclaw_Linux_x86_64.tar.gz
sudo install -m 0755 neuroclaw neuroclaw-launcher /usr/local/bin/
```

### deb / rpm

软件包命名为 `neuroclaw_x86_64.deb`、`neuroclaw_aarch64.rpm` 等。它们会把两个二进制文件安装到 `/usr/bin`，并为启动器附带一个桌面入口。

```bash
sudo dpkg -i neuroclaw_x86_64.deb      # Debian, Ubuntu
sudo rpm -i neuroclaw_x86_64.rpm       # Fedora, RHEL
```

### systemd

让网关以独立用户运行。请先以该用户身份运行一次 `neuroclaw onboard`。

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

网关默认监听 `localhost:18790`。`neuroclaw gateway --host 0.0.0.0`（或 `NEUROCLAW_GATEWAY_HOST`）可以在单次运行中改变这一点。

## Web 控制台（启动器）

```bash
neuroclaw-launcher                 # http://localhost:18800, opens a browser
neuroclaw-launcher -console -no-browser
neuroclaw-launcher -public         # listen on all interfaces
```

参数：`-port`（默认 18800）、`-host`、`-public`、`-no-browser`、`-console`、`-lang`、`-d`。启动器会启动并管理网关。不要把它暴露给不受信任的网络；见[配置详解](configuration.zh.md#web-启动器控制台)。

## Docker

```bash
docker compose -f docker/docker-compose.yml --profile gateway up       # first run: writes config, exits
docker compose -f docker/docker-compose.yml --profile gateway up -d
docker compose -f docker/docker-compose.yml --profile launcher up -d   # web console on :18800
docker compose -f docker/docker-compose.yml run --rm neuroclaw-agent -m "Hello"
```

服务：`neuroclaw-gateway`（profile `gateway`）、`neuroclaw-launcher`（profile `launcher`）、`neuroclaw-agent`（profile `agent`）。数据位于 `docker/data`。详情：[Docker 指南](docker.zh.md)。

## macOS

压缩包没有签名，也没有经过公证。解压后，请清除隔离标志：

```bash
tar xzf neuroclaw_Darwin_arm64.tar.gz
xattr -d com.apple.quarantine neuroclaw neuroclaw-launcher
./neuroclaw onboard
```

## Windows

解压 `neuroclaw_Windows_x86_64.zip`。里面有 `neuroclaw.exe` 和 `neuroclaw-launcher.exe`。这些文件没有代码签名，所以 SmartScreen 可能会发出警告：只有在核对过校验和之后，才选择“更多信息”，再选择“仍要运行”。shell 工具通过 PowerShell 运行命令。

```powershell
.\neuroclaw.exe onboard
.\neuroclaw.exe gateway
```

## Android（Termux）

ARM64 手机上的 Termux 通过 `termux-chroot` 运行 `Linux_arm64` 压缩包。完整步骤：[Termux 指南](android-termux.md)（英文）。

## 更新

```bash
neuroclaw update
```

`neuroclaw update` 会下载本仓库的最新发布版本，并用 `neuroclaw_<version>_checksums.txt` 检查压缩包。参数：`--url`（发布页面或资源文件）、`--platform`、`--arch`。用 `dpkg` 或 `rpm` 安装的软件包，也可以通过安装更新的软件包来升级。Docker：`docker compose pull`。

## 作为 NeuroEdge 之家的大脑

见 [NeuroClaw 与 NeuroEdge](neuroedge.zh.md)。
