# NeuroClaw 🦞

[PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 ができることはすべて（チャットアプリ、LLM プロバイダー、メモリ、リマインダー、シェル、スキル、MCP、Web コンソールとランチャー）、さらに [NeuroEdge](https://github.com/letrongminh/neuroedge-init) ホームの頭脳として動きます。NeuroEdge のホームボックスに接続すると、ボックスの MCP ツールを通じて家のデバイスにアクセスします。ツールは家のルールに従って判断し、取り消せない操作については先に家にいるあなたへ確認を求めます。モデルがチャットの外に手を伸ばせるようになる機能（シェル、ハードウェアバス、スキルのインストール、フック、stdio MCP サーバー）は、設定どおりにオン・オフされ、オンになっているものは起動時に名前が出力されます（`risk=…`）。

Linux ボード、サーバー（NeuroEdge Cloud）、Mac、Windows PC で動作します。リリースの対象プラットフォームは PicoClaw と同じです。インストール：[docs/guides/install.ja.md](../guides/install.ja.md)。

チップ上で動かすこともできます。[`esp32/`](../../esp32/) は **NeuroClaw-ESP** で、同じ頭脳を ESP-IDF コンポーネントとして C で書き直し、ESP32-S3 1 個で動かします。Telegram、LLM（フォールバック付き）、メモリ、リマインダー、ハートビート、Web 検索、オーナーへのメッセージとファイル、そしてゲートまでを数ドルのチップに載せ、ワークスペースとツールもここと同じです（[同じ点と違う点](../../esp32/README.md)）。頭脳とゲートが同じアドレス空間を共有するため、`risk=single_chip` のラベルが付きます。

NeuroClaw は [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1（MIT）のハードフォークです。[NOTICE](../../NOTICE) を参照してください。

## PicoClaw からの変更点

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| HTTP 経由の MCP | ヘッダーのみ | 相互 TLS にも対応：`tls.ca_file`、`cert_file`、`key_file`、`server_name`、TLS 1.3。NeuroEdge ボックス（`neuroedge mcp serve --http`）はクライアント証明書を必須とするため、ブリッジは不要です。 |
| シェル、ハードウェア、スキルのインストール、フック、stdio MCP | 設定でオン・オフ | 同じで、デフォルトも PicoClaw と同じです。オンになっているものは起動時に `NeuroClaw risk=<name>` として記録されます（`shell`、`cron_shell`、`hardware_bus`、`skill_install`、`process_hooks`、`usb_monitor`、`stdio_mcp`。[`pkg/config/risk.go`](../../pkg/config/risk.go)）。頭脳を閉じたい NeuroEdge ホームは `config.json` でそう指定します。 |
| 「オーナーに聞く」と言うゲート | 通常のツール結果 | ボックスのツールが `confirmation` 付きの `BLOCK` を返すと、NeuroClaw はモデルに、何も実行されていないこと、承認できるのは家にいるオーナーだけであること、再試行しないことをはっきり伝えます（[`pkg/tools/integration/neuroedge_result.go`](../../pkg/tools/integration/neuroedge_result.go)）。 |
| `update` | `sipeed/picoclaw` のリリースをダウンロード | このリポジトリのリリースをダウンロードし、チェックサムで検証 |
| 名前 | `picoclaw`、`~/.picoclaw`、`PICOCLAW_*` | `neuroclaw`、`~/.neuroclaw`、`NEUROCLAW_*` |

それ以外は PicoClaw のものです。チャットチャンネル（Telegram ほか）、LLM プロバイダー、メモリとセッション、cron とハートビート、Web 検索、メッセージ、組み込みスキル、サブエージェント、Web コンソールとランチャー（`neuroclaw-launcher`）、Docker イメージ（`ghcr.io/letrongminh/neuroclaw`）、ナイトリービルド、deb/rpm パッケージ、macOS の DMG。ガイドは [`docs/`](../) にあります（[元の README](PICOCLAW_README.md)）。そこでは `picoclaw` を `neuroclaw` と読み替えてください。

**[NeuroClaw にできること](../reference/capabilities.md)** は、両方の頭脳のすべての機能、デフォルトでオンのもの、各リスクラベルの意味、既知の問題を一覧にしています。

## NeuroEdge ボックスに接続する

ガイド：[docs/guides/neuroedge.ja.md](../guides/neuroedge.ja.md)。

ボックス側は `neuroedge mcp serve --http --ui …` を実行し、鍵は NeuroEdge の `demo/i2f-chat-den-cua/tao-khoa.py` で作成します。`~/.neuroclaw/config.json` に次のように書きます。

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

デモのエージェントでは、`neuroclaw mcp test neuroedge` が `reachable (3 tools)` と表示するはずです。

## ビルド

```bash
CGO_ENABLED=0 go build -tags goolm,stdjson -o neuroclaw ./cmd/neuroclaw
CGO_ENABLED=0 go test -tags goolm,stdjson ./...
```

`make build-launcher` は Web コンソール（`web/`、pnpm が必要）をビルドします。`make build-all` と goreleaser（`.goreleaser.yaml`）はすべてのプラットフォームをビルドします。macOS では、PicoClaw v0.3.1 と同様にシェルツールのテストが 3 件失敗します。CI は Linux で実行され、そこでは通ります。

**リスクラベル導入前の NeuroClaw ビルドからのアップグレード**（2026-10-13）：それらのビルドは、シェル、cron コマンド、スキルのインストール、USB 監視を強制的にオフにし、`onboard` がオフのまま `config.json` に保存していました。`config.json` でオンに戻すか、`neuroclaw config reset` でデフォルトからやり直してください。

## ライセンス

MIT（PicoClaw と同じ）。Copyright (c) 2026 PicoClaw contributors and NeuroClaw contributors.
