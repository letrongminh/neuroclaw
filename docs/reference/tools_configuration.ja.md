# 🔧 ツール設定

> [README](../project/README.ja.md) に戻る

NeuroClaw のツール設定は `config.json` の `tools` フィールドにあります。

## ディレクトリ構造

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

## 機密データのフィルタリング

ツールの結果を LLM に送る前に、NeuroClaw は出力から機密値（API キー、トークン、シークレット）を取り除けます。これにより、LLM が自身の認証情報を目にすることを防ぎます。

詳細は[機密データのフィルタリング](../security/sensitive_data_filtering.md)を参照してください。

| 設定項目 | 型 | デフォルト | 説明 |
|----------|------|------------|------|
| `filter_sensitive_data` | bool | `true` | フィルタリングを有効/無効にする |
| `filter_min_length` | int | `8` | フィルタリングを実行する最小コンテンツ長 |

## Web ツール

Web ツールはウェブ検索とフェッチに使用されます。

### Web Fetcher
ウェブページコンテンツの取得と処理に関する一般設定。

| 設定項目            | 型     | デフォルト    | 説明                                                                                   |
|---------------------|--------|---------------|----------------------------------------------------------------------------------------|
| `enabled`           | bool   | true          | ウェブページ取得機能を有効にする。                                                     |
| `fetch_limit_bytes` | int    | 10485760      | 取得するウェブページペイロードの最大サイズ（バイト単位、デフォルトは10MB）。            |
| `format`            | string | "plaintext"   | 取得コンテンツの出力形式。オプション：`plaintext` または `markdown`（推奨）。           |

### Brave

| 設定項目      | 型       | デフォルト | 説明                                           |
|---------------|----------|------------|------------------------------------------------|
| `enabled`     | bool     | false      | Brave 検索を有効にする                         |
| `api_key`     | string   | -          | Brave Search API キー                          |
| `api_keys`    | string[] | -          | ローテーション用の複数の API キー（`api_key` より優先） |
| `max_results` | int      | 5          | 最大結果数                                     |

### DuckDuckGo

| 設定項目      | 型   | デフォルト | 説明                        |
|---------------|------|------------|-----------------------------|
| `enabled`     | bool | false      | DuckDuckGo 検索を有効にする |
| `max_results` | int  | 5          | 最大結果数                  |

### Sogou

| 設定項目      | 型   | デフォルト | 説明                  |
|---------------|------|------------|-----------------------|
| `enabled`     | bool | true       | Sogou 検索を有効にする |
| `max_results` | int  | 5          | 最大結果数            |

### Gemini Google Search

Gemini 検索は、Google Search によるグラウンディングを備えた Gemini を使用します。Google Search の引用付きで、AI が合成した回答を返します。

| 設定項目      | 型     | デフォルト           | 説明                              |
|---------------|--------|----------------------|-----------------------------------|
| `enabled`     | bool   | false                | Gemini Google Search を有効にする |
| `api_key`     | string | -                    | Google Gemini API キー            |
| `model`       | string | `gemini-2.5-flash`   | 検索に使用する Gemini モデル      |
| `max_results` | int    | 5                    | 引用の最大数                      |

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

Baidu Search は [Qianfan AI Search API](https://cloud.baidu.com/doc/qianfan-api/s/Wmbq4z7e5) を使用します。AI を活用しており、中国語のクエリに最適化されています。

| 設定項目      | 型     | デフォルト                                             | 説明                          |
|---------------|--------|--------------------------------------------------------|-------------------------------|
| `enabled`     | bool   | false                                                  | Baidu 検索を有効にする        |
| `api_key`     | string | -                                                      | Qianfan API キー              |
| `base_url`    | string | `https://qianfan.baidubce.com/v2/ai_search/web_search` | Baidu Search API URL          |
| `max_results` | int    | 10                                                     | 最大結果数                    |

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

| 設定項目      | 型       | デフォルト | 説明                                           |
|---------------|----------|------------|------------------------------------------------|
| `enabled`     | bool     | false      | Perplexity 検索を有効にする                    |
| `api_key`     | string   | -          | Perplexity API キー                            |
| `api_keys`    | string[] | -          | ローテーション用の複数の API キー（`api_key` より優先） |
| `max_results` | int      | 5          | 最大結果数                                     |

### Kagi Search

Kagi Search は公式の Kagi OpenAPI クライアントで `POST /search` を呼び出し、`data.search` から通常のウェブ検索結果を返します。

| 設定項目      | 型       | デフォルト                            | 説明                                           |
|---------------|----------|---------------------------------------|------------------------------------------------|
| `enabled`     | bool     | false                                 | Kagi Search を有効にする                       |
| `api_key`     | string   | -                                     | Kagi API キー                                  |
| `api_keys`    | string[] | -                                     | ローテーション用の複数の API キー（`api_key` より優先） |
| `base_url`    | string   | `https://kagi.com/api/v1/search`      | Kagi Search API エンドポイント                 |
| `max_results` | int      | 5                                     | 最大結果数                                     |

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

Kagi の API キーは `.security.yml` に保存します：

```yaml
web:
  kagi:
    api_keys:
      - "YOUR_KAGI_API_KEY"
```

Kagi API の利用は、アカウントや API の設定によっては、通常の Kagi サブスクリプションとは別に課金または制限される場合があります。

### Tavily

| 設定項目      | 型     | デフォルト | 説明                           |
|---------------|--------|------------|--------------------------------|
| `enabled`     | bool   | false      | Tavily 検索を有効にする        |
| `api_key`     | string | -          | Tavily API キー                |
| `base_url`    | string | -          | カスタム Tavily API ベース URL |
| `max_results` | int    | 5          | 最大結果数                     |

### SearXNG

| 設定項目      | 型     | デフォルト | 説明                      |
|---------------|--------|------------|---------------------------|
| `enabled`     | bool   | false      | SearXNG 検索を有効にする  |
| `base_url`    | string | -          | SearXNG インスタンス URL（例：`http://localhost:8888`） |
| `max_results` | int    | 5          | 最大結果数                |

### GLM Search

| 設定項目        | 型     | デフォルト                                        | 説明                      |
|-----------------|--------|---------------------------------------------------|---------------------------|
| `enabled`       | bool   | false                                             | GLM Search を有効にする   |
| `api_key`       | string | -                                                 | GLM API キー              |
| `base_url`      | string | `https://open.bigmodel.cn/api/paas/v4/web_search` | GLM Search API URL        |
| `search_engine` | string | `search_std`                                      | 検索エンジンタイプ        |
| `max_results`   | int    | 5                                                 | 最大結果数                |

### その他の Web 設定

| 設定項目                 | 型       | デフォルト | 説明                                                           |
|--------------------------|----------|------------|----------------------------------------------------------------|
| `provider`               | string   | `auto`     | 使用する検索 provider。`auto` は有効な検索エンジンの中から選択する |
| `proxy`                  | string   | `""`       | Web ツール用のプロキシ                                         |
| `prefer_native`          | bool     | true       | 設定済みの検索エンジンより、provider のネイティブ検索を優先する |
| `private_host_whitelist` | string[] | `[]`       | Web 取得を許可するプライベート/内部ホスト                      |

### `web_search` ツールのパラメータ

実行時、`web_search` ツールは次のパラメータを受け付けます：

| フィールド | 型 | 必須 | 説明 |
|------------|------|------|------|
| `query` | string | はい | 検索クエリ文字列 |
| `count` | integer | いいえ | 返す結果数。デフォルト：`10`、最大：`10` |
| `range` | string | いいえ | 任意の期間フィルター：`d`（日）、`w`（週）、`m`（月）、`y`（年） |

`range` を省略すると、NeuroClaw は期間を制限せずに検索します。
Kagi では、`d`、`w`、`m` は Kagi lens の `time_relative` に対応し、`y` は現在の日付の 1 年前を指す lens の `time_after` に対応します。

### `web_search` の呼び出し例

```json
{
  "query": "ai agent news",
  "count": 10,
  "range": "w"
}
```

## Exec ツール

Exec ツールはシェルコマンドの実行に使用されます。

| 設定項目                | 型    | デフォルト | 説明                               |
|-------------------------|-------|------------|------------------------------------|
| `enabled`               | bool  | true       | Exec ツールを有効にする。有効な場合、起動時に `risk=shell` として報告される |
| `enable_deny_patterns`  | bool  | true       | デフォルトの危険コマンドブロックを有効にする |
| `custom_deny_patterns`  | array | []         | カスタム拒否パターン（正規表現）   |
| `custom_allow_patterns` | array | []         | カスタム許可パターン（正規表現）   |
| `allow_remote`          | bool  | true       | リモート channel（チャットアプリ）からの exec 呼び出しを許可する。`false` にすると exec は内部 channel に限定される |
| `timeout_seconds`       | int   | 60         | コマンドごとのタイムアウト（秒）。`0` はデフォルト（60）を使用 |

### Exec ツールの無効化

`exec` ツールを完全に無効にするには、`enabled` を `false` に設定します：

**設定ファイル経由：**
```json
{
  "tools": {
    "exec": {
      "enabled": false
    }
  }
}
```

**環境変数経由：**
```bash
NEUROCLAW_TOOLS_EXEC_ENABLED=false
```

> **注意：** 無効にすると、エージェントはシェルコマンドを実行できなくなります。これは Cron ツールがスケジュールされたシェルコマンドを実行する能力にも影響します。

### 機能

- **`enable_deny_patterns`**：`false` に設定すると、デフォルトの危険コマンドブロックパターンを完全に無効にします
- **`custom_deny_patterns`**：カスタム拒否正規表現パターンを追加します。一致するコマンドはブロックされます
- **`custom_allow_patterns`**：許可の正規表現パターンを追加します。一致するコマンドは拒否パターンの対象外になります
- **`allow_remote`**：`false` に設定すると、リモート channel からの exec 呼び出しを拒否します

### デフォルトでブロックされるコマンドパターン

デフォルトで、NeuroClaw は以下の危険なコマンドをブロックします：

- 削除コマンド：`rm -rf`、`del /f/q`、`rmdir /s`
- ディスク操作：`format`、`mkfs`、`diskpart`、`dd if=`、`/dev/sd*` への書き込み
- システム操作：`shutdown`、`reboot`、`poweroff`
- コマンド置換：`$()`、`${}`、バッククォート
- シェルへのパイプ：`| sh`、`| bash`
- 権限昇格：`sudo`、`chmod`、`chown`
- プロセス制御：`pkill`、`killall`、`kill -9`
- リモート操作：`curl | sh`、`wget | sh`、`ssh`
- パッケージ管理：`apt`、`yum`、`dnf`、`npm install -g`、`pip install --user`
- コンテナ：`docker run`、`docker exec`
- Git：`git push`、`git force`
- その他：`eval`、`source *.sh`

### 既知のアーキテクチャ上の制限

exec ガードは NeuroClaw に送信されたトップレベルのコマンドのみを検証します。そのコマンドの実行開始後にビルドツールやスクリプトが生成する子プロセスを再帰的に検査することは**ありません**。

初期コマンドが許可された後、直接コマンドガードをバイパスできるワークフローの例：

- `make run`
- `go run ./cmd/...`
- `cargo run`
- `npm run build`

これは、明らかに危険な直接コマンドのブロックには有用ですが、未レビューのビルドパイプラインに対する完全なサンドボックスでは**ありません**。脅威モデルにワークスペース内の信頼できないコードが含まれる場合は、コンテナ、VM、またはビルド・実行コマンドに対する承認フローなど、より強力な分離を使用してください。

### 設定例

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

## Cron ツール

Cron ツールは定期タスクのスケジューリングに使用されます。

| 設定項目                  | 型       | デフォルト | 説明                                                           |
|---------------------------|----------|------------|----------------------------------------------------------------|
| `enabled`                 | bool     | true       | エージェント向けの cron ツールを登録する                       |
| `allow_command`           | bool     | true       | 追加の確認なしでコマンドジョブを許可する。`exec` が有効な場合、起動時に `risk=cron_shell` として報告される |
| `command_allowed_remotes` | string[] | []         | コマンドジョブを許可するリモート channel または `channel:chat_id` の値。`*` はすべての channel を許可するため、すべてのリモート channel を信頼できる場合を除き危険です |
| `exec_timeout_minutes`    | int      | 5          | 実行タイムアウト（分）、0 は無制限                             |

スケジュールの種類、実行モード（`deliver`、agent ターン、コマンドジョブ）、永続化、現在のコマンドセキュリティゲートについては、[スケジュールタスクと Cron ジョブ](cron.md)を参照してください。

## ツールごとのスイッチ

その他のすべてのツールには、`tools.<name>` の下に独自の `enabled` スイッチがあります（環境変数 `NEUROCLAW_TOOLS_<NAME>_ENABLED`）。デフォルト値は `pkg/config/defaults.go` のものです：

| ツール | デフォルト | 備考 |
|--------|------------|------|
| `read_file`、`write_file`、`append_file`、`edit_file`、`list_dir` | on | `read_file` には `mode`（`bytes` または `lines`、デフォルトは `bytes`）と `max_read_file_size`（デフォルトは 65536）もあります |
| `load_image`、`send_file`、`web_fetch` | on | |
| `message` | on | `media_enabled` はデフォルトで off |
| `spawn`、`subagent` | on | |
| `spawn_status` | off | |
| `get_diagnostics` | on | [診断ツール](#diagnostics-tool)を参照 |
| `find_skills`、`install_skill` | on | `tools.skills.enabled` が有効な場合のみ機能する。`risk=skill_install` として報告される |
| `send_tts` | off | |
| `i2c`、`spi`、`serial` | off | ハードウェアツール。`i2c` と `spi` は Linux のみ。有効な場合は `risk=hardware_bus` として報告される |
| `media_cleanup` | on | `max_age_minutes` は 30、`interval_minutes` は 5 |

リスクラベルとそれぞれの意味は、[NeuroClaw にできること、セクション 2](capabilities.md#2-risk-labels)にあります。

<a id="diagnostics-tool"></a>
## 診断ツール

`get_diagnostics` は、NeuroClaw 自身の状態をモデルが報告するためのツールです。読み取り専用で引数はなく、NeuroClaw-ESP のツールと同じ名前です。次の内容を JSON で返します。

- `version`、`git_commit`、`go_version`、`os`、`arch`、`num_cpu`
- プロセスの `started_at` と `uptime_sec`
- `memory`（ランタイムから得た Go ヒープと合計バイト数、GC 回数）と `goroutines`
- `workspace_disk`：エージェントのワークスペースがあるファイルシステムの空き容量と総容量（Linux、macOS、FreeBSD、Windows。それ以外では取得できない旨を `error` に記載）
- `channels_enabled`：有効なチャネル名
- `models`：デフォルトモデル、そのフォールバック、`cooldowns`（起動後に失敗したモデルごとに 1 件。フォールバックチェーンがスキップしている間は `available: false`、`remaining_sec`、課金エラーの場合は `disabled_reason`）
- `llm_budget`：上限、この 1 時間と今日の呼び出し回数、リセット時刻（[LLM 呼び出し予算](rate-limiting.md#llm-call-budget)、英語）
- `risk_labels`：有効になっているリスクのある機能（起動時のログと同じもの）
- `last_llm_error`：最後に失敗した LLM 呼び出しの時刻とメッセージ。なければ `null`

報告するのは名前、カウンター、状態だけです。API キー、トークン、URL は決して含まれません。最後の LLM エラーは、URL から認証情報とクエリ文字列を取り除いたうえで[機密データのフィルタリング](../security/sensitive_data_filtering.md)を通します。

| 設定 | 型 | デフォルト | 説明 |
|------|----|------------|------|
| `enabled` | bool | true | `get_diagnostics` ツールを登録する |

```json
{
  "tools": {
    "get_diagnostics": {
      "enabled": true
    }
  }
}
```

環境変数：`NEUROCLAW_TOOLS_GET_DIAGNOSTICS_ENABLED`

## MCP ツール

MCP ツールは外部の Model Context Protocol サーバーとの統合を可能にします。

JSON を手作業で編集したくない場合は、NeuroClaw の MCP 設定マネージャー CLI も使えます：

- `neuroclaw mcp add` — サーバーを追加または更新（`--deferred` / `--no-deferred` に対応）
- `neuroclaw mcp list` — 設定済みのすべてのサーバーを、状態と deferred の状態とともに一覧表示
- `neuroclaw mcp show <name>` — 1 つのサーバーの詳細とツール一覧を表示
- `neuroclaw mcp test <name>` — 1 つのサーバーの接続確認
- `neuroclaw mcp remove <name>` — サーバーエントリを削除
- `neuroclaw mcp edit` — 高度な編集のため、`$EDITOR` で `config.json` を開く

これらのコマンドは、以下で説明する `tools.mcp.servers` セクションと同じものを管理します。コマンドの構文、例、動作の詳細は [MCP Server CLI](mcp-cli.md) を参照してください。

### ツールディスカバリ（遅延読み込み）

複数の MCP サーバーに接続する場合、数百のツールを同時に公開すると LLM のコンテキストウィンドウを使い果たし、API コストが増加する可能性があります。**Discovery** 機能は、MCP ツールをデフォルトで*非表示*にすることでこの問題を解決します。

すべてのツールを読み込む代わりに、LLM には軽量な検索ツール（BM25 キーワードマッチングまたは正規表現を使用）が提供されます。LLM が特定の機能を必要とする場合、非表示のライブラリを検索します。一致するツールは一時的に「アンロック」され、設定されたターン数（`ttl`）の間コンテキストに注入されます。

### グローバル設定

| 設定項目    | 型     | デフォルト | 説明                                 |
|-------------|--------|------------|--------------------------------------|
| `enabled`   | bool   | false      | MCP 統合をグローバルに有効にする     |
| `discovery` | object | `{}`       | ツールディスカバリ設定（下記参照）   |
| `servers`   | object | `{}`       | サーバー名からサーバー設定へのマップ |

### Discovery 設定（`discovery`）

| 設定項目             | 型   | デフォルト | 説明                                                                                                          |
|----------------------|------|------------|---------------------------------------------------------------------------------------------------------------|
| `enabled`            | bool | false      | グローバルのデフォルト：`true` の場合、すべての MCP ツールは非表示になり、検索を通じてオンデマンドで読み込まれます。`false` の場合、すべてのツールがコンテキストに読み込まれます。個々のサーバーは、サーバーごとの `deferred` フィールドでこれを上書きできます |
| `ttl`                | int  | 5          | 発見されたツールがアンロック状態を維持する会話ターン数                                                        |
| `max_search_results` | int  | 5          | 検索クエリごとに返されるツールの最大数                                                                        |
| `use_bm25`           | bool | true       | 自然言語/キーワード検索ツール（`tool_search_tool_bm25`）を有効にする。**警告**：正規表現検索よりリソースを消費します |
| `use_regex`          | bool | false      | 正規表現パターン検索ツール（`tool_search_tool_regex`）を有効にする                                            |

> **注意：** `discovery.enabled` が `true` の場合、少なくとも1つの検索エンジン（`use_bm25` または `use_regex`）を有効にする**必要があります**。
> そうしないとアプリケーションの起動に失敗します。

### サーバーごとの設定

| 設定項目   | 型     | 必須     | 説明                                   |
|------------|--------|----------|----------------------------------------|
| `enabled`  | bool   | はい     | この MCP サーバーを有効にする          |
| `deferred` | bool   | いいえ   | このサーバーのみ deferred モードを上書きする。`true` = ツールは非表示になり、検索で発見できる。`false` = ツールは常にコンテキストに表示される。省略した場合は、グローバルの `discovery.enabled` の値が適用される |
| `type`     | string | いいえ   | トランスポートタイプ：`stdio`、`sse`、`http`（`streamable-http` は `http` の別名として受け付けられる） |
| `command`  | string | stdio    | stdio トランスポートの実行コマンド     |
| `args`     | array  | いいえ   | stdio トランスポートのコマンド引数     |
| `env`      | object | いいえ   | stdio プロセスの環境変数               |
| `env_file` | string | いいえ   | stdio プロセスの環境ファイルパス       |
| `url`      | string | sse/http | `sse`/`http` トランスポートのエンドポイント URL |
| `headers`  | object | いいえ   | `sse`/`http` トランスポートの HTTP ヘッダー |
| `tls`      | object | いいえ   | NeuroClaw：`sse`/`http` トランスポート用の相互 TLS（下記参照） |

### 相互 TLS (`tls`)

NeuroClaw で追加された機能です。`mcp serve --http` で起動した NeuroEdge のホームボックスは、自分が署名した証明書を持たないクライアントをすべて拒否します。`http` または `sse` のサーバーに `tls` を設定すると、ブレインがクライアント証明書を提示し、ボックスの CA だけを信頼します。横に TLS ブリッジを置く必要はありません。

| 設定項目      | 型     | 説明 |
|---------------|--------|------|
| `ca_file`     | string | サーバー証明書に署名した CA の PEM ファイル。空の場合：システムのルート証明書 |
| `cert_file`   | string | このクライアントの証明書の PEM ファイル。`key_file` と一緒に設定するか、どちらも設定しない |
| `key_file`    | string | このクライアントの秘密鍵の PEM ファイル |
| `server_name` | string | サーバー証明書で検証される名前を上書きする |

パスは `~` で始めることができます。`pkg/mcp/tls.go` に基づくルール：

- `url` は `https://` である必要があります。そうでない場合、サーバーは `tls is set but url ... is not https` で接続に失敗します。
- 接続は TLS 1.3 以上を使用します。
- `tls` がない場合は、PicoClaw と同様に、デフォルトの HTTP トランスポートが使われます。
- `stdio` サーバーでは `tls` は無視されます。
- `neuroclaw mcp add` には `tls` 用のフラグがありません。`config.json` を編集して追加してください（`neuroclaw mcp edit`）。同じ名前で `mcp add` を再実行すると、エントリが置き換えられ、`tls` ブロックは失われます。

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

ホームボックス側については、[NeuroEdge ガイド](../guides/neuroedge.md)を参照してください。

### トランスポートの動作

- `type` を省略した場合、トランスポートは自動検出されます：
    - `url` が設定されている → `sse`
    - `command` が設定されている → `stdio`
- `http` と `sse` はどちらも `url` + オプションの `headers` + オプションの `tls` を使用します。
- `neuroclaw mcp add` のデフォルトは `stdio` です（[MCP Server CLI](mcp-cli.md) を参照）。この自動検出は手書きのエントリにのみ適用されます。
- 有効な `stdio` サーバーは、起動時に `risk=stdio_mcp` として報告されます。
- `env` と `env_file` は `stdio` サーバーにのみ適用されます。

### 設定例

#### 1) Stdio MCP サーバー

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

#### 2) リモート SSE/HTTP MCP サーバー

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

#### 3) ツールディスカバリを有効にした大規模 MCP セットアップ

*この例では、LLM は `tool_search_tool_bm25` のみを認識します。ユーザーからリクエストがあった場合にのみ、Github や Postgres のツールを動的に検索してアンロックします。*

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

#### 4) 混在セットアップ：サーバーごとの deferred の上書き

*ディスカバリはグローバルに有効ですが、`filesystem` は常に表示されるよう固定し、`context7` はグローバルのデフォルト（deferred）に従います。`aws` はグローバルのデフォルトと同じですが、明示的に deferred モードを選択しています。*

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

> **ヒント：** サーバーごとの `deferred` は `discovery.enabled` とは独立しています。グローバルでは
> `discovery.enabled: false`（デフォルトですべてのツールが表示される）のままにしておき、
> ツール数の多い個別のサーバーにだけ `"deferred": true` を付けて、それらのツールでコンテキストが
> 膨らむのを避けることができます。

## Skills ツール

Skills ツールは、ClawHub や GitHub などのレジストリを通じたスキルの発見とインストールを設定します。

### レジストリ

| 設定項目                           | 型     | デフォルト           | 説明                                         |
|------------------------------------|--------|----------------------|----------------------------------------------|
| `registries.clawhub.enabled`       | bool   | true                 | ClawHub レジストリを有効にする               |
| `registries.clawhub.base_url`      | string | `https://clawhub.ai` | ClawHub ベース URL                           |
| `registries.clawhub.auth_token`    | string | `""`                 | より高いレート制限のためのオプションの Bearer トークン |
| `registries.clawhub.search_path`   | string | `""`                 | 検索 API パス                                |
| `registries.clawhub.skills_path`   | string | `""`                 | Skills API パス                              |
| `registries.clawhub.download_path` | string | `""`                 | ダウンロード API パス                        |
| `registries.clawhub.timeout`       | int    | 0                    | リクエストタイムアウト（秒）（0 = デフォルト） |
| `registries.clawhub.max_zip_size`  | int    | 0                    | スキル zip の最大サイズ（バイト）（0 = デフォルト） |
| `registries.clawhub.max_response_size` | int | 0                   | API レスポンスの最大サイズ（バイト）（0 = デフォルト） |
| `registries.github.enabled`        | bool   | true                 | レジストリ設定による GitHub からのインストールを有効にする |
| `registries.github.base_url`       | string | `https://github.com` | GitHub または GitHub Enterprise のベース URL |
| `registries.github.auth_token`     | string | `""`                 | GitHub の personal access token              |
| `registries.github.proxy`          | string | `""`                 | GitHub API リクエスト用の HTTP プロキシ      |

### レガシーの GitHub 設定

`github.*` は非推奨です。代わりに `registries.github.*` を使用してください。レガシーのフィールドは互換性のため引き続きサポートされますが、将来削除されます。

| 設定項目           | 型     | デフォルト           | 説明                           |
|--------------------|--------|----------------------|--------------------------------|
| `github.base_url`  | string | `https://github.com` | 非推奨の GitHub ベース URL     |
| `github.proxy`     | string | `""`                 | 非推奨の GitHub プロキシ       |
| `github.token`     | string | `""`                 | 非推奨の GitHub トークン       |

### 検索設定

| 設定項目                  | 型   | デフォルト | 説明                                       |
|---------------------------|------|------------|--------------------------------------------|
| `max_concurrent_searches` | int  | 2          | スキル検索リクエストの最大同時実行数       |
| `search_cache.max_size`   | int  | 50         | キャッシュする検索結果の最大数             |
| `search_cache.ttl_seconds`| int  | 300        | キャッシュの TTL（秒）                     |

### 設定例

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

## 環境変数

すべての設定オプションは `NEUROCLAW_TOOLS_<SECTION>_<KEY>` 形式の環境変数で上書きできます：

例：

- `NEUROCLAW_TOOLS_WEB_BRAVE_ENABLED=true`
- `NEUROCLAW_TOOLS_EXEC_ENABLED=false`
- `NEUROCLAW_TOOLS_EXEC_ENABLE_DENY_PATTERNS=false`
- `NEUROCLAW_TOOLS_CRON_EXEC_TIMEOUT_MINUTES=10`
- `NEUROCLAW_TOOLS_MCP_ENABLED=true`
- `NEUROCLAW_TOOLS_MCP_MAX_INLINE_TEXT_CHARS=16384`

注意：ネストされたマップ形式の設定（例：`tools.mcp.servers.<name>.*`）は環境変数ではなく `config.json` で設定します。

MCP ツールでは、`tools.mcp.max_inline_text_chars` がモデルのコンテキストにインラインで保持するテキスト結果の量を制御します。しきい値はバイトではなく Unicode 文字（Go の rune）で数えます。たとえば `16384` は最大 16,384 文字をインラインで保持することを意味し、CJK などのマルチバイトテキストでは 16 KB を超えることがあります。このしきい値を超えると、NeuroClaw は MCP のテキスト結果をエージェントのワークスペースにローカルのアーティファクトとして保存し、ペイロード全体をコンテキストに注入する代わりに、短いメモと構造化された `[file:...]` アーティファクトパスをモデルに渡します。
