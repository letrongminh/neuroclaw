# NeuroClaw と NeuroEdge

> [ガイド](README.md)に戻る

[NeuroEdge](https://github.com/letrongminh/neuroedge-init) のホームボックスは、家のデバイスをゲートの内側で動かします。NeuroClaw は頭脳です。チャットを読み、考え、MCP を通じてボックスのツールを呼び出します。ボックスは各呼び出しを家のルールで判断し、取り消せない操作については家にいるオーナーに確認を求めます。

## ボックスに接続する

ボックスは `neuroedge mcp serve --http` で MCP を HTTPS で提供します。ボックス自身が署名したクライアント証明書が必要です。NeuroClaw は `http` タイプの MCP サーバーの `tls` ブロックでそれを提示するため、ブリッジは不要です。

`~/.neuroclaw/config.json` に次のように書きます。

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

| `tls` フィールド | 意味 |
|:---|:---|
| `ca_file` | ボックスの証明書に署名した PEM 形式の CA。この CA だけを信頼します。 |
| `cert_file`, `key_file` | 頭脳のクライアント証明書と鍵。両方を設定するか、どちらも設定しないかのどちらかです。 |
| `server_name` | `url` のホストと異なる場合に、ボックスの証明書で確認する名前。 |

`pkg/mcp/tls.go` の規則：`tls` を設定する場合、`url` は `https` でなければなりません。最小バージョンは TLS 1.3 です。パスには `~/` を使えます。次のコマンドで確認します。

```bash
neuroclaw mcp test neuroedge
```

## モデルに伝える内容

ボックスのツールはステータス付きの結果を返します。NeuroClaw は、単純な `ALLOW` ではない結果を、モデルへの固定の指示に変換します（`pkg/tools/integration/neuroedge_result.go`）。そのため、どのモデルでも同じ答えになります。

| ステータス | モデルに伝える内容 |
|:---|:---|
| `BLOCK`（confirmation あり） | ボックスはまだ実行していません。オーナーが、示された秒数ほどの間に、家の NeuroEdge ページで承認する必要があります。モデルは承認できず、チャットでの承認も無効です。ユーザーには待機中だと伝え、ツールを再度呼ばず、完了したとは言わないこと。 |
| `BLOCK`（confirmation なし） | ボックスは、理由と満たされなかった基準を添えて拒否しました。ユーザーには実行されなかったことと理由を伝え、別の引数で再試行したり、ルールを回避する方法を探したりしないこと。 |
| `REJECTED` | ボックスが呼び出し自体を拒否しました（未知のツール、またはスキーマ外の引数）。何も実行されていません。 |

他の MCP サーバーの結果には手を加えません。

## リスクラベル

起動時に、モデルがチャット、ワークスペース、ボックスのツールの外に手を伸ばせる機能ごとに、NeuroClaw は `NeuroClaw risk=<name>` をログに出力します。

| ラベル | オンになる条件 |
|:---|:---|
| `shell` | `tools.exec`（デフォルトでオン） |
| `cron_shell` | `exec` とともに `tools.cron.allow_command`（デフォルトでオン） |
| `skill_install` | `tools.skills` がオンのときの `tools.install_skill` または `find_skills`（デフォルトでオン） |
| `hardware_bus` | `tools.i2c`、`spi`、`serial`（デフォルトでオフ） |
| `process_hooks` | `hooks.processes` の有効なエントリ（デフォルトでオフ） |
| `usb_monitor` | `devices` とともに `devices.monitor_usb`（デフォルトでオフ） |
| `stdio_mcp` | `tools.mcp.servers` の stdio サーバー（デフォルトでオフ） |

ラベルは知らせるだけで、何もブロックしません。それぞれの意味：[capabilities](../reference/capabilities.md)。

## ボックスと同じマシンの場合：頭脳を閉じる

ボックスのゲートが対象とするのは、ゲートを通る呼び出しだけです。ボックスも動いているマシン上のシェルやハードウェアバスは、ゲートを迂回してデバイスに届くことがあります。頭脳がボックスとマシンを共有している場合は、それらをオフにします。

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

これを既存の設定にマージし、上の `mcp` ブロックは残してください。NeuroClaw を再起動し、これらについての `risk=` 行がログに出ていないことを確認します。

あわせて、頭脳は専用の OS ユーザーで実行してください（[インストールガイド](install.ja.md#systemd)の systemd ユニットを参照）。そのユーザーには、頭脳自身のクライアント証明書以外のボックスの鍵へのアクセス権も、デバイスノードへのアクセス権も与えません。

## 別のマシンまたは NeuroEdge Cloud の場合

頭脳が別のマシンや NeuroEdge Cloud で動く場合でも、デバイスの操作はすべてゲートが判断します。シェルとハードウェアバスが届くのは頭脳が動いているマシンだけなので、必要に応じて設定してください。

## NeuroClaw-ESP

同じ頭脳を ESP32-S3 1 個で動かし、ゲートも同じチップに載せます：[esp32/README.md](../../esp32/README.md)。頭脳とゲートが同じアドレス空間を共有するため、`risk=single_chip` のラベルが付きます。
