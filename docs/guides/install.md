# Install NeuroClaw

> Back to [Guides](README.md)

NeuroClaw ships as one static binary, `neuroclaw`, and a web console binary, `neuroclaw-launcher`. Releases are on [GitHub](https://github.com/letrongminh/neuroclaw/releases). Config and workspace live in `~/.neuroclaw` (set `NEUROCLAW_HOME` to move it).

## Pick a file

| Where it runs | Download |
|:---|:---|
| Linux server or cloud VM | `neuroclaw_Linux_x86_64.tar.gz`, `neuroclaw_Linux_arm64.tar.gz`, or a `.deb` / `.rpm` |
| Raspberry Pi and other boards | `Linux_arm64` (64-bit OS), `Linux_armv7`, `Linux_armv6` (Pi Zero/1), `Linux_riscv64`, `Linux_loong64`, `Linux_mipsle` (soft-float routers), `Linux_s390x` |
| Docker | `ghcr.io/letrongminh/neuroclaw` (linux/amd64, arm64, riscv64): [Docker guide](docker.md) |
| macOS | `neuroclaw_Darwin_arm64.tar.gz`, `neuroclaw_Darwin_x86_64.tar.gz` |
| Windows | `neuroclaw_Windows_x86_64.zip`, `neuroclaw_Windows_arm64.zip` |
| FreeBSD, NetBSD | `Freebsd_*`, `Netbsd_*` archives |
| Android | Termux with `Linux_arm64`: [Termux guide](android-termux.md) |
| One ESP32-S3 | Not a download. See [esp32/README.md](../../esp32/README.md) |

Archives are named `neuroclaw_<OS>_<arch>.tar.gz` (`.zip` on Windows). Every file is listed in `neuroclaw_<version>_checksums.txt` on the release page.

## Verify a download

```bash
sha256sum --check --ignore-missing neuroclaw_*_checksums.txt     # Linux
shasum -a 256 --check --ignore-missing neuroclaw_*_checksums.txt # macOS
```

On Windows, compare `Get-FileHash <file>` with the line in `neuroclaw_<version>_checksums.txt`.

## First run

```bash
neuroclaw onboard          # writes ~/.neuroclaw/config.json and the workspace
vim ~/.neuroclaw/config.json   # add a provider key to model_list
neuroclaw agent -m "Hello" # one message; without -m it is interactive
neuroclaw gateway          # long-running: chat apps, cron, heartbeat
```

See [Configuration](configuration.md) and [Providers](providers.md) for the config. `neuroclaw status` shows what is set up. At startup NeuroClaw logs `NeuroClaw risk=<name>` for each risky capability that is on; see [capabilities](../reference/capabilities.md).

## Linux

```bash
tar xzf neuroclaw_Linux_x86_64.tar.gz
sudo install -m 0755 neuroclaw neuroclaw-launcher /usr/local/bin/
```

### deb / rpm

Packages are named `neuroclaw_x86_64.deb`, `neuroclaw_aarch64.rpm` and so on. They install both binaries to `/usr/bin`, plus a desktop entry for the launcher.

```bash
sudo dpkg -i neuroclaw_x86_64.deb      # Debian, Ubuntu
sudo rpm -i neuroclaw_x86_64.rpm       # Fedora, RHEL
```

### systemd

Run the gateway as its own user. Run `neuroclaw onboard` once as that user first.

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

The gateway listens on `localhost:18790` by default. `neuroclaw gateway --host 0.0.0.0` (or `NEUROCLAW_GATEWAY_HOST`) changes that for one run.

## Web console (launcher)

```bash
neuroclaw-launcher                 # http://localhost:18800, opens a browser
neuroclaw-launcher -console -no-browser
neuroclaw-launcher -public         # listen on all interfaces
```

Flags: `-port` (default 18800), `-host`, `-public`, `-no-browser`, `-console`, `-lang`, `-d`. The launcher starts and manages the gateway. Do not expose it to untrusted networks; see [Configuration](configuration.md#web-launcher-dashboard).

## Docker

```bash
docker compose -f docker/docker-compose.yml --profile gateway up       # first run: writes config, exits
docker compose -f docker/docker-compose.yml --profile gateway up -d
docker compose -f docker/docker-compose.yml --profile launcher up -d   # web console on :18800
docker compose -f docker/docker-compose.yml run --rm neuroclaw-agent -m "Hello"
```

Services: `neuroclaw-gateway` (profile `gateway`), `neuroclaw-launcher` (profile `launcher`), `neuroclaw-agent` (profile `agent`). Data is in `docker/data`. Details: [Docker guide](docker.md).

## macOS

The archives are not signed or notarized. After extracting, clear the quarantine flag:

```bash
tar xzf neuroclaw_Darwin_arm64.tar.gz
xattr -d com.apple.quarantine neuroclaw neuroclaw-launcher
./neuroclaw onboard
```

## Windows

Unzip `neuroclaw_Windows_x86_64.zip`. It holds `neuroclaw.exe` and `neuroclaw-launcher.exe`. The files are not code-signed, so SmartScreen may warn: choose "More info", then "Run anyway", only after you checked the checksum. The shell tool runs commands through PowerShell.

```powershell
.\neuroclaw.exe onboard
.\neuroclaw.exe gateway
```

## Android (Termux)

Termux on an ARM64 phone runs the `Linux_arm64` archive through `termux-chroot`. Full steps: [Termux guide](android-termux.md).

## Update

```bash
neuroclaw update
```

`neuroclaw update` downloads the newest release of this repository and checks the archive against `neuroclaw_<version>_checksums.txt`. Flags: `--url` (a release page or asset), `--platform`, `--arch`. Packages installed with `dpkg` or `rpm` can also be updated by installing the newer package. Docker: `docker compose pull`.

## As the brain of a NeuroEdge home

See [NeuroClaw with NeuroEdge](neuroedge.md).
