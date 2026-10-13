# NeuroClaw のインストール

> [ガイド](README.md)に戻る

NeuroClaw は、静的バイナリ 1 つの `neuroclaw` と、Web コンソール用バイナリの `neuroclaw-launcher` として配布されます。リリースは [GitHub](https://github.com/letrongminh/neuroclaw/releases) にあります。設定とワークスペースは `~/.neuroclaw` に置かれます（移動するには `NEUROCLAW_HOME` を設定します）。

## ファイルを選ぶ

| 動かす場所 | ダウンロード |
|:---|:---|
| Linux サーバーまたはクラウド VM | `neuroclaw_Linux_x86_64.tar.gz`、`neuroclaw_Linux_arm64.tar.gz`、または `.deb` / `.rpm` |
| Raspberry Pi などのボード | `Linux_arm64`（64 ビット OS）、`Linux_armv7`、`Linux_armv6`（Pi Zero/1）、`Linux_riscv64`、`Linux_loong64`、`Linux_mipsle`（ソフトフロートのルーター）、`Linux_s390x` |
| Docker | `ghcr.io/letrongminh/neuroclaw` （linux/amd64、arm64、riscv64）：[Docker ガイド](docker.ja.md) |
| macOS | `neuroclaw_Darwin_arm64.tar.gz`, `neuroclaw_Darwin_x86_64.tar.gz` |
| Windows | `neuroclaw_Windows_x86_64.zip`, `neuroclaw_Windows_arm64.zip` |
| FreeBSD、NetBSD | `Freebsd_*`、`Netbsd_*` アーカイブ |
| Android | Termux で `Linux_arm64` を使用：[Termux ガイド](android-termux.md) |
| ESP32-S3 1 個 | ダウンロードではありません。[esp32/README.md](../../esp32/README.md) を参照 |

アーカイブ名は `neuroclaw_<OS>_<arch>.tar.gz` です（Windows は `.zip`）。すべてのファイルはリリースページの `checksums.txt` に記載されています。

## ダウンロードの検証

```bash
sha256sum --check --ignore-missing checksums.txt     # Linux
shasum -a 256 --check --ignore-missing checksums.txt # macOS
```

Windows では、`Get-FileHash <file>` の出力と `checksums.txt` の該当行を比べます。

## 初回起動

```bash
neuroclaw onboard          # ~/.neuroclaw/config.json とワークスペースを作成
vim ~/.neuroclaw/config.json   # model_list にプロバイダーのキーを追加
neuroclaw agent -m "Hello" # メッセージを 1 つ送る。-m を付けない場合は対話モード
neuroclaw gateway          # 常駐：チャットアプリ、cron、ハートビート
```

設定については[設定ガイド](configuration.ja.md)と[プロバイダー](providers.ja.md)を参照してください。`neuroclaw status` で設定済みの内容を確認できます。起動時に、オンになっているリスクのある機能ごとに `NeuroClaw risk=<name>` がログに出力されます。詳しくは [capabilities](../reference/capabilities.md) を参照してください。

## Linux

```bash
tar xzf neuroclaw_Linux_x86_64.tar.gz
sudo install -m 0755 neuroclaw neuroclaw-launcher /usr/local/bin/
```

### deb / rpm

パッケージ名は `neuroclaw_x86_64.deb`、`neuroclaw_aarch64.rpm` などです。両方のバイナリが `/usr/bin` にインストールされ、ランチャー用のデスクトップエントリも追加されます。

```bash
sudo dpkg -i neuroclaw_x86_64.deb      # Debian、Ubuntu
sudo rpm -i neuroclaw_x86_64.rpm       # Fedora、RHEL
```

### systemd

Gateway は専用のユーザーで実行します。先にそのユーザーで `neuroclaw onboard` を 1 回実行してください。

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

Gateway はデフォルトで `localhost:18790` で待ち受けます。`neuroclaw gateway --host 0.0.0.0`（または `NEUROCLAW_GATEWAY_HOST`）で、その実行に限って変更できます。

## Web コンソール（ランチャー）

```bash
neuroclaw-launcher                 # http://localhost:18800、ブラウザが開く
neuroclaw-launcher -console -no-browser
neuroclaw-launcher -public         # すべてのインターフェースで待ち受け
```

フラグ：`-port`（デフォルト 18800）、`-host`、`-public`、`-no-browser`、`-console`、`-lang`、`-d`。ランチャーは Gateway を起動して管理します。信頼できないネットワークには公開しないでください。[設定ガイド](configuration.ja.md)を参照してください。

## Docker

```bash
docker compose -f docker/docker-compose.yml --profile gateway up       # 初回：設定を書き込んで終了
docker compose -f docker/docker-compose.yml --profile gateway up -d
docker compose -f docker/docker-compose.yml --profile launcher up -d   # Web コンソールは :18800
docker compose -f docker/docker-compose.yml run --rm neuroclaw-agent -m "Hello"
```

サービス：`neuroclaw-gateway`（プロファイル `gateway`）、`neuroclaw-launcher`（プロファイル `launcher`）、`neuroclaw-agent`（プロファイル `agent`）。データは `docker/data` にあります。詳細：[Docker ガイド](docker.ja.md)。

## macOS

アーカイブは署名も公証もされていません。展開したら、隔離フラグを削除します。

```bash
tar xzf neuroclaw_Darwin_arm64.tar.gz
xattr -d com.apple.quarantine neuroclaw neuroclaw-launcher
./neuroclaw onboard
```

## Windows

`neuroclaw_Windows_x86_64.zip` を展開します。中には `neuroclaw.exe` と `neuroclaw-launcher.exe` が入っています。ファイルにはコード署名がないため、SmartScreen が警告を出すことがあります。チェックサムを確認したうえで、「詳細情報」から「実行」を選んでください。シェルツールは PowerShell 経由でコマンドを実行します。

```powershell
.\neuroclaw.exe onboard
.\neuroclaw.exe gateway
```

## Android（Termux）

ARM64 スマートフォンの Termux では、`Linux_arm64` アーカイブを `termux-chroot` 経由で実行します。手順の全体：[Termux ガイド](android-termux.md)。

## アップデート

```bash
neuroclaw update
```

`neuroclaw update` は、このリポジトリの最新リリースをダウンロードし、アーカイブを `checksums.txt` と照合します。フラグ：`--url`（リリースページまたはアセット）、`--platform`、`--arch`。`dpkg` や `rpm` でインストールしたパッケージは、新しいパッケージをインストールして更新することもできます。Docker の場合は `docker compose pull` です。

## NeuroEdge ホームの頭脳として

[NeuroClaw と NeuroEdge](neuroedge.ja.md)を参照してください。
