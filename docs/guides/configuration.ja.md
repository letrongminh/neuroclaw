# ⚙️ 設定ガイド

> [README](../project/README.ja.md) に戻る

## ⚙️ 設定詳細

設定ファイルパス: `~/.neuroclaw/config.json`

> **セキュリティ設定：** API キー、トークンなどの機密データの保存方法は [セキュリティ設定ガイド](../security/security_configuration.md) を参照してください。

### 環境変数

環境変数を使用してデフォルトパスを上書きできます。ポータブルインストール、コンテナ化デプロイ、または neuroclaw をシステムサービスとして実行する場合に便利です。これらの変数は独立しており、異なるパスを制御します。

| 変数              | 説明                                                                                                                             | デフォルトパス            |
|-------------------|-----------------------------------------------------------------------------------------------------------------------------------------|---------------------------|
| `NEUROCLAW_CONFIG` | 設定ファイルのパスを上書きします。neuroclaw がどの `config.json` を読み込むかを直接指定し、他のすべての場所を無視します。 | `~/.neuroclaw/config.json` |
| `NEUROCLAW_HOME`   | neuroclaw データのルートディレクトリを上書きします。`workspace` やその他のデータディレクトリのデフォルト場所を変更します。          | `~/.neuroclaw`             |

**例：**

```bash
# Run neuroclaw using a specific config file
# The workspace path will be read from within that config file
NEUROCLAW_CONFIG=/etc/neuroclaw/production.json neuroclaw gateway

# Run neuroclaw with all its data stored in /opt/neuroclaw
# Config will be loaded from the default ~/.neuroclaw/config.json
# Workspace will be created at /opt/neuroclaw/workspace
NEUROCLAW_HOME=/opt/neuroclaw neuroclaw agent

# Use both for a fully customized setup
NEUROCLAW_HOME=/srv/neuroclaw NEUROCLAW_CONFIG=/srv/neuroclaw/main.json neuroclaw gateway
```

### Gateway ログレベル

`gateway.log_level` は Gateway のログ詳細度を制御します。`config.json` で設定できます。

```json
{
  "gateway": {
    "log_level": "warn"
  }
}
```

省略した場合のデフォルト値は `warn` です。サポートされる値：`debug`、`info`、`warn`、`error`、`fatal`。

環境変数 `NEUROCLAW_LOG_LEVEL` でも上書きできます。

### ワークスペースレイアウト

NeuroClaw は設定されたワークスペース（デフォルト: `~/.neuroclaw/workspace`）にデータを保存します。

```
~/.neuroclaw/workspace/
├── sessions/          # Conversation sessions and history
├── memory/           # Long-term memory (MEMORY.md)
├── state/            # Persistent state (last channel, etc.)
├── cron/             # Scheduled jobs database
├── skills/           # Custom skills
├── AGENT.md          # Agent behavior guide
├── HEARTBEAT.md      # Periodic task prompts (checked every 30 min)
├── IDENTITY.md       # Agent identity
├── SOUL.md           # Agent soul
└── USER.md           # User preferences
```

> **注意：** `AGENT.md`、`SOUL.md`、`USER.md` および `memory/MEMORY.md` への変更は、ファイル更新時刻（mtime）の追跡により実行時に自動検出されます。これらのファイルを編集した後に **gateway を再起動する必要はありません** — Agent は次のリクエスト時に最新の内容を自動的に読み込みます。

### Agent の自己進化

`evolution` ブロックは NeuroClaw の自己進化ランタイムを制御します。有効にすると、Agent は完了したターンを学習レコードとして記録します。上位のモードでは、繰り返し成功したパターンをグループ化し、スキルのドラフトを生成し、承認されたドラフトをワークスペースのスキルに適用することもできます。

```json
{
  "evolution": {
    "enabled": false,
    "mode": "observe",
    "state_dir": "",
    "min_task_count": 2,
    "min_success_ratio": 0.7,
    "cold_path_trigger": "after_turn",
    "cold_path_times": []
  }
}
```

| フィールド | デフォルト | 説明 |
|-----------|-----------|------|
| `enabled` | `false` | 完了した Agent ターンの学習レコード記録を有効にします。ハートビートのターンは無視されます。 |
| `mode` | `observe` | `observe` はデータの記録のみ行います。`draft` はスキルのドラフト候補を生成できます。`apply` は承認されたドラフトをワークスペースのスキルに適用できます。 |
| `state_dir` | `""` | 進化の状態を保存するディレクトリ（任意）。空の場合はワークスペース配下のデフォルトを使います。 |
| `min_task_count` | `2` | パターンがドラフト生成の対象になるために必要な、関連タスクレコードの最小数。 |
| `min_success_ratio` | `0.7` | タスククラスタの最小成功率。`0` より大きく `1` 以下の値を指定します。 |
| `cold_path_trigger` | `after_turn` | ドラフト生成を `after_turn`（ターン後）、`scheduled`（スケジュール）で実行します。`manual` にすると自動の cold path 実行を無効にします。ユーザー向けの手動トリガーはまだありません。`draft` と `apply` モードでのみ有効です。 |
| `cold_path_times` | `[]` | `cold_path_trigger` が `scheduled` のときの実行時刻。`HH:MM` 形式の文字列で指定します。 |

スキルを変更せずに学習レコードだけを確認したい場合は、まず `observe` を使います。NeuroClaw にレビュー可能な改善案を用意させたい場合は `draft` を使います。承認されたドラフトでワークスペースのスキルが更新されても構わない場合にのみ `apply` を使ってください。

### リクエストコンテキストポリシー

`turn_profile` は `agents.defaults.turn_profile` に置く任意のリクエストコンテキストポリシーです。未設定、または `"enabled": false` の場合、NeuroClaw は通常動作のままです。`"enabled": true` にすると、同じポリシーが各新規ターンに適用されます。

各ブロックは同じ `mode` を使います。

| Mode | 意味 |
| --- | --- |
| `default` | そのブロックで NeuroClaw の通常動作を維持します。ブロックまたは `mode` が省略された場合も `default` として扱います。 |
| `off` | そのターンでそのブロックを無効にします。 |
| `custom` | 許可リストを使います。このバージョンでは `skills` と `tools` のみ対応し、`history` や `system_prompt` で使うと検証エラーになります。 |

ブロックの意味：

| ブロック | 制御する内容 |
| --- | --- |
| `history` | 過去のセッション履歴と要約の読み込み、ユーザー/アシスタント/ツールメッセージの保存、コンテキスト取り込み、圧縮と要約。 |
| `system_prompt` | NeuroClaw の既定の identity、ワークスペース指示、メモリ、実行時コンテキスト、要約の注入。`off` でも外部リクエストの system prompt は利用できます。 |
| `skills` | Skill カタログと active skill のプロンプト内容。`custom.allow` は列挙した skill 名だけをプロンプトコンテキストに残します。 |
| `tools` | モデルに公開され、実行を許可されるツール。`custom.allow` は登録済みで列挙されたツール名だけを残します。 |

`system_prompt.mode` が `off` で、ツールが表示され、外部 system prompt もない場合、NeuroClaw は既存のツール使用ルールを最小のフォールバックプロンプトとして使います。`tools.mode` が `off` の場合、このフォールバックは追加されません。

Web ツールだけを残すクリーンなポリシーの例：

```json
{
  "agents": {
    "defaults": {
      "turn_profile": {
        "enabled": true,
        "history": { "mode": "off" },
        "system_prompt": { "mode": "off" },
        "skills": { "mode": "off" },
        "tools": {
          "mode": "custom",
          "allow": ["web_search", "web_fetch"]
        }
      }
    }
  }
}
```

### Web ランチャーダッシュボード

**neuroclaw-launcher** は、最初にパスワードでのサインインを求めるブラウザ UI を提供します。初回起動時は `/launcher-setup` を開いてダッシュボードのパスワードを作成します。以降の手動サインインは `/launcher-login` を使います。

- **設定ファイル**: `config.json` と同じディレクトリ（または `NEUROCLAW_CONFIG` が指すファイルのディレクトリ）。ランチャー専用のファイルは `launcher-config.json` です。
- **パスワードの保存**: 対応プラットフォームでは、パスワードは bcrypt ハッシュとして `launcher-auth.db` に保存されます。SQLite のパスワードストアが使えないプラットフォームでは、bcrypt ハッシュは `launcher-config.json` に保存されます。
- **旧形式の移行**: 古い `launcher_token` の値は一度だけパスワードログインに移行され、保存済みのランチャー設定から削除されます。
- **ローカル自動ログイン**: 起動後にランチャーがローカルブラウザを自動で開く場合、ループバック専用のワンショットのブートストラップエンドポイントを使ってセッション cookie を自動設定します。
- **非対応の認証方式**: URL トークンログイン（`?token=...`）、`NEUROCLAW_LAUNCHER_TOKEN`、`Authorization: Bearer` によるダッシュボード認証は、サポートされなくなりました。
- **サインアウト**: **`Content-Type: application/json`** を付けて **`POST /api/auth/logout`** を使います（body は `{}` で構いません）。ログアウトに GET の URL を使わないでください（CSRF 対策）。
- **ブルートフォース対策**: **`POST /api/auth/login`** は **クライアント IP ごとに 1 分あたりのレート制限**があります（超過時は HTTP 429）。
- **セッション期間**: HttpOnly のセッション cookie はデフォルトで約 **31 日**有効ですが、ランチャープロセスを再起動するとセッションは無効になります。

### スキルソース

デフォルトでは、スキルは以下の順序で読み込まれます。

1. `~/.neuroclaw/workspace/skills`（ワークスペース）
2. `~/.neuroclaw/skills`（グローバル）
3. `<ビルド時埋め込みパス>/skills`（ビルトイン、ビルド時に設定）

高度な/テスト用セットアップでは、以下の環境変数でビルトインスキルのルートを上書きできます。

```bash
export NEUROCLAW_BUILTIN_SKILLS=/path/to/skills
```

### チャットチャネルからスキルとコマンドを使う

スキルをインストールし、MCP サーバーを設定すると、チャットチャネルから直接確認したり明示的に適用したりできます。

- `/list skills` は現在の Agent から見えるインストール済みスキル名を表示します。
- `/list mcp` は設定済みの MCP サーバーを、有効/遅延/接続済みの状態とともに表示します。
- `/show mcp <server>` は接続済みの MCP サーバーが公開しているアクティブなツールを表示します。
- `/use <skill> <message>` は 1 回のリクエストだけそのスキルを強制します。
- `/use <skill>` は同じチャットセッション内の次のメッセージにそのスキルを予約します。
- `/use clear` は `/use <skill>` で設定した保留中のスキル上書きを解除します。
- `/btw <question>` は現在のセッション履歴を変更せずに即時の横道の質問を送ります。`/btw` はツールなしの直接質問として処理され、通常のツール実行フローには入りません。

例：

```text
/list skills
/list mcp
/show mcp github
/use git explain how to squash the last 3 commits
/btw remind me what we already decided about the deploy plan
/use italiapersonalfinance
dammi le ultime news
```

### 統一コマンド実行ポリシー

- 汎用スラッシュコマンドは `pkg/agent/loop.go` 内の `commands.Executor` を通じて統一的に実行されます。
- チャネルアダプターはローカルで汎用コマンドを消費しなくなりました。受信テキストを bus/agent パスに転送するだけです。Telegram は起動時に `/start`、`/help`、`/show`、`/list`、`/use`、`/btw` などのサポート済みコマンドを引き続き自動登録します。
- 未登録のスラッシュコマンド（例: `/foo`）は通常の LLM 処理にパススルーされます。
- 登録済みだが現在のチャネルでサポートされていないコマンド（例: WhatsApp での `/show`）は、明示的なユーザー向けエラーを返し、以降の処理を停止します。

### セッション分離

セッションスコープは、チャット、ユーザー、スレッド、スペースの間でどこまでメモリを共有するかを制御します。

- グローバルのデフォルトには `session.dimensions` を使います。
- 個別のルーティング例外には、dispatch ルールの `session_dimensions` を使います。

手順ごとのレシピと分離パターンは [Session ガイド](session-guide.md) を参照してください。

### ルーティング

ルーティングは `agents.dispatch.rules` で設定します。

各ルールは、チャネルが生成する正規化済みの受信コンテキストに対してマッチします。ルールは上から順に評価され、最初にマッチしたルールが採用されます。どのルールにもマッチしない場合、NeuroClaw は設定されたデフォルト Agent にフォールバックします。

サポートされるマッチフィールド：

* `channel`
* `account`
* `space`
* `chat`
* `topic`
* `sender`
* `mentioned`

マッチ値はセッションシステムと同じスコープの表記を使います。

* `space`: `workspace:t001`、`guild:123456`
* `chat`: `direct:user123`、`group:-100123`、`channel:c123`
* `topic`: `topic:42`
* `sender`: プラットフォームごとの正規化された送信者識別子

ルールは `session_dimensions` で、グローバルの `session.dimensions` の値を任意に上書きできます。これにより、旧来の `bindings` や `dm_scope` 形式を復活させずに、ルーティングとセッション割り当てを揃えられます。

例：

```json
{
  "agents": {
    "list": [
      { "id": "main", "default": true },
      { "id": "support" },
      { "id": "sales" }
    ],
    "dispatch": {
      "rules": [
        {
          "name": "vip in support group",
          "agent": "sales",
          "when": {
            "channel": "telegram",
            "chat": "group:-1001234567890",
            "sender": "12345"
          },
          "session_dimensions": ["chat", "sender"]
        },
        {
          "name": "telegram support group",
          "agent": "support",
          "when": {
            "channel": "telegram",
            "chat": "group:-1001234567890"
          },
          "session_dimensions": ["chat"]
        }
      ]
    }
  },
  "session": {
    "dimensions": ["chat"]
  }
}
```

上の例では、VIP ルールを、より広いグループルールより前に置く必要があります。ルーティングは厳密に順序どおりに評価されるため、より具体的なルールを前に、より広いフォールバックのルールを後ろに置いてください。

より完全なルーティングとモデルティアの例は [ルーティングガイド](routing-guide.md) を参照してください。

### Agent のツール許可リスト

Agent ごとのツール宣言は `config.json` ではなく、`AGENT.md` の frontmatter に書きます。

frontmatter に `tools` がない場合、Agent は通常のグローバルに有効なツールセットを使います。`tools` がある場合、NeuroClaw はその Agent に対して列挙されたランタイムツールだけを登録します。

```md
---
name: Research Agent
description: Specialist for web research and in-depth analysis.
tools: [read_file, write_file, web_search, web_fetch, message]
skills: [deep-research]
mcpServers: [web-index]
---

You are the research agent.
```

注意：

- これは許可リストであり、単なる優先ヒントではありません。
- ツール名はランタイムのツール名と 1:1 で照合されます。
- `web_search`、`web_fetch`、`spawn`、`subagent`、`send_file` などのランタイムツール名を使ってください。
- `AGENT.md` のツール宣言はランタイム/ツール側で使われますが、discovery プロンプトには注入されません。

### Agent Discovery（自動）

Agent が spawn 可能なピアを持ち、`spawn` を呼び出せる場合、NeuroClaw は毎ターン、その Agent の system prompt に構造化された Agent レジストリを注入します。追加の `list_agents` ツール呼び出しは不要です。

このレジストリは、特に `spawn` で対象の `agent_id` を指定する場合に、委任を具体的で確実なものにするためのものです。

各エントリには次の項目が含まれます。

| フィールド | 意味 |
|-----------|------|
| `id` | 安定した Agent ID |
| `name` | `AGENT.md` frontmatter の Agent アイデンティティ名 |
| `description` | `AGENT.md` frontmatter の Agent アイデンティティの説明 |

重要な動作：

- discovery セクションは、現在の Agent が `spawn` ツールを持ち、かつ `subagents.allow_agents` で spawn を許可されたピア Agent のみを含む場合にだけ表示されます。
- 現在の Agent 自身と spawn できないピアは省略されるため、モデルは利用できない Agent を前提に計画しません。
- discovery は意図的に軽量です。ピアを選ぶために必要な `id`、`name`、`description` のみをモデルに渡します。
- `config.json` はインフラ層のままです：ワークスペース、デフォルト Agent の選択、ルーティング、サブ Agent の権限。これらの権限は discovery の可視性も制御します。
- `AGENT.md` はアイデンティティ層のままです。委任が行われるとき、ランタイム/ツールのコードは引き続き `tools`、`skills`、`mcpServers`、`model` フィールドを使えます。

注入される形式の例：

```json
{
  "agents": [
    {
      "id": "research",
      "name": "Research Agent",
      "description": "Specialist for long-form investigation and web work."
    }
  ]
}
```

実際には、汎用 Agent がロールの説明に基づいてピアを選び、そのピアの `agent_id` で `spawn` を呼び出します。残りはランタイムが解決します。

### 🔒 セキュリティサンドボックス

NeuroClaw はデフォルトでサンドボックス環境で実行されます。Agent は設定されたワークスペース内のファイルアクセスとコマンド実行のみが可能です。

#### デフォルト設定

```json
{
  "agents": {
    "defaults": {
      "workspace": "~/.neuroclaw/workspace",
      "restrict_to_workspace": true
    }
  }
}
```

| オプション              | デフォルト値            | 説明                                  |
| ----------------------- | ----------------------- | ------------------------------------- |
| `workspace`             | `~/.neuroclaw/workspace` | Agent の作業ディレクトリ              |
| `restrict_to_workspace` | `true`                  | ファイル/コマンドアクセスをワークスペース内に制限 |

#### 保護されたツール

`restrict_to_workspace: true` の場合、以下のツールがサンドボックス化されます。

| ツール        | 機能             | 制限                               |
| ------------- | ---------------- | ---------------------------------- |
| `read_file`   | ファイル読み取り | ワークスペース内のファイルのみ     |
| `write_file`  | ファイル書き込み | ワークスペース内のファイルのみ     |
| `list_dir`    | ディレクトリ一覧 | ワークスペース内のディレクトリのみ |
| `edit_file`   | ファイル編集     | ワークスペース内のファイルのみ     |
| `append_file` | ファイル追記     | ワークスペース内のファイルのみ     |
| `exec`        | コマンド実行     | コマンドパスはワークスペース内必須 |

#### 追加の Exec 保護

`restrict_to_workspace: false` の場合でも、`exec` ツールは以下の危険なコマンドをブロックします。

* `rm -rf`、`del /f`、`rmdir /s` — 一括削除
* `format`、`mkfs`、`diskpart` — ディスクフォーマット
* `dd if=` — ディスクイメージング
* `/dev/sd[a-z]` への書き込み — 直接ディスク書き込み
* `shutdown`、`reboot`、`poweroff` — システムシャットダウン
* Fork bomb `:(){ :|:& };:`
* そのほか：`sudo`、`chmod`/`chown`、`kill`/`pkill`、`curl ... | sh`、`git push`、`docker run`、パッケージのインストール、コマンド置換（`$(...)`、バッククォート）など。完全な一覧は `pkg/tools/shell.go` の `defaultDenyPatterns` にあります。

### ファイルアクセス制御

| 設定キー | 型 | デフォルト値 | 説明 |
|----------|------|-------------|------|
| `tools.allow_read_paths` | string[] | `[]` | ワークスペース外で読み取りを許可する追加パス |
| `tools.allow_write_paths` | string[] | `[]` | ワークスペース外で書き込みを許可する追加パス |
| `tools.message.media_enabled` | bool | `false` | `message` ツールがパス指定でローカルメディアファイルを添付できるようにします。`tools.send_file.enabled` とは別の設定です。テキスト/メディア/キャプションを一括で送る運用にする場合にのみ有効にしてください。 |

### Read File モード

`read_file` には、設定で選択される相互排他的な 2 つの実装があります。NeuroClaw は起動時にそのうち 1 つだけを登録します。

| 設定キー | 型 | デフォルト値 | 説明 |
|----------|------|-------------|------|
| `tools.read_file.enabled` | bool | `true` | `read_file` ツールを有効にします |
| `tools.read_file.mode` | string | `bytes` | `read_file` の実装を選択します：`bytes` または `lines` |
| `tools.read_file.max_read_file_size` | int | `65536` | `read_file` が返す最大バイト数 |

#### モード：`bytes`

任意のファイルとバイナリセーフなページネーションに最適化されています。

パラメータ：

* `path`（必須）：ファイルパス
* `offset`（任意）：開始バイトオフセット。デフォルトは `0`
* `length`（任意）：読み取る最大バイト数。デフォルトは `max_read_file_size`

`bytes` を使う場面：

* バイナリファイルを読む可能性がある
* バイト範囲による決定的なページネーションが必要

#### モード：`lines`

ソースファイル、markdown、ログ、設定ファイル向けのテキスト指向の動作です。行単位で順に読み、設定されたバイト数の上限に達すると停止します。

パラメータ：

* `path`（必須）：ファイルパス
* `start_line`（任意）：開始行番号。1 始まりで、その行を含みます。デフォルトは `1`
* `max_lines`（任意）：読み取る最大行数。デフォルトは、EOF またはバイト数の上限までの残りすべての行

動作に関する注意：

* バイナリと思われるファイルは拒否され、`read_file` を `mode = bytes` に切り替えるよう案内されます
* 極端に長い 1 行はスキップされず、切り詰められます

`mode = lines` を使う場面：

* Agent が主にテキストファイルを読む
* プロンプトやツール呼び出しで行ベースのページネーションを使いたい
* コードレビュー、ログ、ドキュメント向けに、よりきれいなチャンクが欲しい

#### 例

```json
{
  "tools": {
    "read_file": {
      "enabled": true,
      "mode": "lines",
      "max_read_file_size": 65536
    }
  }
}
```

### Exec セキュリティ

| 設定キー | 型 | デフォルト値 | 説明 |
|----------|------|-------------|------|
| `tools.exec.allow_remote` | bool | `true` | リモートチャネル（Telegram/Discord など）からの exec ツール実行を許可します。内部チャネルのみに制限するには `false` に設定します |
| `tools.exec.enable_deny_patterns` | bool | `true` | 危険なコマンドのインターセプトを有効化 |
| `tools.exec.custom_deny_patterns` | string[] | `[]` | カスタムブロック正規表現パターン |
| `tools.exec.custom_allow_patterns` | string[] | `[]` | カスタム許可正規表現パターン |

> **リスクラベル：** `exec` と cron のシェルコマンドはデフォルトで有効で、起動時にそれぞれ名前付きで表示されます（`NeuroClaw risk=shell`、`NeuroClaw risk=cron_shell`）。スキルのインストールとハードウェアバスのツールにもラベルがあります。一覧、各機能でモデルに何ができるか、無効化の方法は [Capabilities §2](../reference/capabilities.md) を参照してください。`exec` を使う場合は、すべてのチャネルで空でない `allow_from` を併用してください（[チャットアプリ](chat-apps.md)）。

> **セキュリティ注意:** Symlink 保護はデフォルトで有効です。すべてのファイルパスはホワイトリストマッチング前に `filepath.EvalSymlinks` で解決され、シンボリックリンクエスケープ攻撃を防止します。

#### 既知の制限：ビルドツールの子プロセス

exec セキュリティガードは NeuroClaw が直接起動するコマンドラインのみを検査します。`make`、`go run`、`cargo`、`npm run`、またはカスタムビルドスクリプトなどの許可された開発ツールが生成する子プロセスは再帰的に検査しません。

つまり、トップレベルのコマンドが初期ガードチェックを通過した後、他のバイナリをコンパイルまたは起動できます。実際には、ビルドスクリプト、Makefile、パッケージスクリプト、生成されたバイナリを、直接のシェルコマンドと同等レベルのレビューが必要な実行可能コードとして扱ってください。

高リスク環境の場合：

* 実行前にビルドスクリプトをレビューしてください。
* コンパイル・実行ワークフローには承認/手動レビューを優先してください。
* ビルトインガードより強力な分離が必要な場合は、コンテナまたは VM 内で NeuroClaw を実行してください。

#### エラー例

```
[ERROR] tool: Tool execution failed
{tool=exec, error=Command blocked by safety guard (path outside working dir)}
```

```
[ERROR] tool: Tool execution failed
{tool=exec, error=Command blocked by safety guard (dangerous pattern detected)}
```

#### 制限の無効化（セキュリティリスク）

Agent がワークスペース外のパスにアクセスする必要がある場合：

**方法 1: 設定ファイル**

```json
{
  "agents": {
    "defaults": {
      "restrict_to_workspace": false
    }
  }
}
```

**方法 2: 環境変数**

```bash
export NEUROCLAW_AGENTS_DEFAULTS_RESTRICT_TO_WORKSPACE=false
```

> ⚠️ **警告**: この制限を無効にすると、Agent がシステム上の任意のパスにアクセスできるようになります。管理された環境でのみ慎重に使用してください。

#### セキュリティ境界の一貫性

`restrict_to_workspace` 設定はすべての実行パスで一貫して適用されます。

| 実行パス         | セキュリティ境界             |
| ---------------- | ---------------------------- |
| メイン Agent     | `restrict_to_workspace` ✅   |
| サブ Agent / Spawn | 同じ制限を継承 ✅           |
| ハートビートタスク | 同じ制限を継承 ✅           |

すべてのパスは同じワークスペース制限を共有しており、サブ Agent やスケジュールタスクを通じてセキュリティ境界を回避することはできません。

### ハートビート（定期タスク）

NeuroClaw は定期タスクを自動実行できます。ワークスペースに `HEARTBEAT.md` ファイルを作成してください。

```markdown
# Periodic Tasks

- Check my email for important messages
- Review my calendar for upcoming events
- Check the weather forecast
```

Agent は 30 分ごと（設定可能）にこのファイルを読み取り、利用可能なツールを使用してタスクを実行します。

#### Spawn を使用した非同期タスク

長時間実行タスク（Web 検索、API 呼び出し）には、`spawn` ツールを使用して**サブ Agent (subagent)** を作成します。

```markdown
# Periodic Tasks

## Quick Tasks (respond directly)

- Report current time

## Long Tasks (use spawn for async)

- Search the web for AI news and summarize
- Check email and report important messages
```

**主な動作：**

| 特性             | 説明                                         |
| ---------------- | -------------------------------------------- |
| **spawn**        | 非同期サブ Agent を作成、メインハートビートをブロックしない |
| **独立コンテキスト** | サブ Agent は独自のコンテキストを持ち、セッション履歴なし |
| **message tool** | サブ Agent は message ツールでユーザーと直接通信 |
| **ノンブロッキング** | spawn 後、ハートビートは次のタスクに進む     |

#### サブ Agent の通信フロー

```
Heartbeat triggers
    ↓
Agent reads HEARTBEAT.md
    ↓
For long task: spawn subagent
    ↓                           ↓
Continue to next task      Subagent works independently
    ↓                           ↓
All tasks done            Subagent uses "message" tool
    ↓                           ↓
Respond HEARTBEAT_OK      User receives result directly
```

サブ Agent はツール（message、web_search など）にアクセスでき、メイン Agent を経由せずに独立してユーザーと通信できます。

**設定：**

```json
{
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

| オプション | デフォルト値 | 説明                           |
| ---------- | ------------ | ------------------------------ |
| `enabled`  | `true`       | ハートビートの有効/無効        |
| `interval` | `30`         | チェック間隔（分単位、最小: 5）|

**環境変数:**

* `NEUROCLAW_HEARTBEAT_ENABLED=false` で無効化
* `NEUROCLAW_HEARTBEAT_INTERVAL=60` で間隔を変更

### Providers

> [!NOTE]
> Groq は Whisper による無料音声文字起こしを提供します。設定すると、任意のチャンネルの音声メッセージが Agent レベルで自動的に文字起こしされます。

| Provider     | 用途                                    | API キー取得                                                 |
| ------------ | --------------------------------------- | ------------------------------------------------------------ |
| `gemini`     | LLM（Gemini 直接）                      | [aistudio.google.com](https://aistudio.google.com)           |
| `zhipu`      | LLM（Zhipu 直接）                       | [bigmodel.cn](https://bigmodel.cn)                           |
| `volcengine` | LLM（Volcengine 直接）                  | [volcengine.com](https://www.volcengine.com) |
| `openrouter` | LLM（推奨、全モデルにアクセス可能）     | [openrouter.ai](https://openrouter.ai)                       |
| `anthropic`  | LLM（Claude 直接）                      | [console.anthropic.com](https://console.anthropic.com)       |
| `openai`     | LLM（GPT 直接）                         | [platform.openai.com](https://platform.openai.com)           |
| `deepseek`   | LLM（DeepSeek 直接）                    | [platform.deepseek.com](https://platform.deepseek.com)       |
| `qwen`       | LLM（Qwen 直接）                        | [dashscope.console.aliyun.com](https://dashscope.console.aliyun.com) |
| `groq`       | LLM + **音声文字起こし**（Whisper）     | [console.groq.com](https://console.groq.com)                 |
| `cerebras`   | LLM（Cerebras 直接）                    | [cerebras.ai](https://cerebras.ai)                           |
| `vivgrid`    | LLM（Vivgrid 直接）                     | [vivgrid.com](https://vivgrid.com)                           |

### モデル設定 (model_list)

> NeuroClaw は、明示的な `provider` とネイティブの `model` による設定を推奨します（例：`"provider": "zhipu", "model": "glm-4.7"`）。`provider` を省略した場合に限り、従来の単一フィールドの `provider/model` 形式も互換性のためサポートされます。

この設計は、柔軟な Provider 選択による**マルチ Agent サポート**も可能にします。

- **Agent ごとに異なる Provider**: 各 Agent が独自の LLM Provider を使えます
- **モデルのフォールバック**: 耐障害性のために、プライマリとフォールバックのモデルを設定できます
- **ロードバランシング**: 複数のエンドポイントやキーにリクエストを分散できます
- **一元的な設定**: すべての Provider を 1 か所で管理できます
- **モデルの有効/無効**: `enabled` フィールドで、設定を削除せずにモデルを一時的に無効にできます

#### 🔒 セキュリティ設定（推奨）

NeuroClaw は、機密データ（API キー、トークン、シークレット）を `.security.yml` ファイルに保存して、メインの設定から分離できます。

**主な利点：**
- **セキュリティ**: 機密データがメインの設定ファイルに含まれません
- **共有しやすい**: API キーを公開せずに config.json を共有できます
- **バージョン管理**: `.security.yml` を `.gitignore` に追加できます
- **柔軟なデプロイ**: 環境ごとに異なるセキュリティファイルを使えます

**クイックセットアップ：**

1. API キーを入れた `~/.neuroclaw/.security.yml` を作成します：
```yaml
model_list:
  gpt-5.4:
    api_keys:
      - "sk-proj-your-actual-openai-key"
  claude-sonnet-4.6:
    api_keys:
      - "sk-ant-your-actual-anthropic-key"
channel_list:
  telegram:
    token: "your-telegram-bot-token"
web:
  brave:
    api_keys:
      - "BSAyour-brave-api-key"
  glm_search:
    api_key: "your-glm-search-api-key"
```

2. 適切なパーミッションを設定します：
```bash
chmod 600 ~/.neuroclaw/.security.yml
```

3. `config.json` から機密フィールドを削除します（推奨）：
```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4"
      // api_key loaded from .security.yml
    }
  ],
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      // token loaded from .security.yml
    }
  }
}
```

**仕組み：**
- `.security.yml` の値は設定フィールドに自動的にマッピングされます
- 特別な構文は不要です — 機密フィールドを config.json から省くだけです
- 両方のファイルにフィールドがある場合は、`.security.yml` の値が優先されます
- config.json の直接の値とセキュリティファイルの値を混在させることもできます

詳細は [`../security/security_configuration.md`](../security/security_configuration.md) を参照してください。

#### サポートされている全 Vendor

| Vendor                  | `provider` の値   | デフォルト API Base                                 | プロトコル | API Key                                                          |
| ----------------------- | ----------------- | --------------------------------------------------- | ---------- | ---------------------------------------------------------------- |
| **OpenAI**              | `openai`          | `https://api.openai.com/v1`                         | OpenAI     | [取得](https://platform.openai.com)                              |
| **Anthropic**           | `anthropic`       | `https://api.anthropic.com/v1`                      | Anthropic  | [取得](https://console.anthropic.com)                            |
| **智谱 AI (GLM)**       | `zhipu`           | `https://open.bigmodel.cn/api/paas/v4`              | OpenAI     | [取得](https://open.bigmodel.cn/usercenter/proj-mgmt/apikeys)    |
| **DeepSeek**            | `deepseek`        | `https://api.deepseek.com/v1`                       | OpenAI     | [取得](https://platform.deepseek.com)                            |
| **Google Gemini**       | `gemini`          | `https://generativelanguage.googleapis.com/v1beta`  | Gemini     | [取得](https://aistudio.google.com/api-keys)                     |
| **Groq**                | `groq`            | `https://api.groq.com/openai/v1`                    | OpenAI     | [取得](https://console.groq.com)                                 |
| **Moonshot**            | `moonshot`        | `https://api.moonshot.cn/v1`                        | OpenAI     | [取得](https://platform.moonshot.cn)                             |
| **通義千問 (Qwen)**     | `qwen`            | `https://dashscope.aliyuncs.com/compatible-mode/v1` | OpenAI     | [取得](https://dashscope.console.aliyun.com)                     |
| **NVIDIA**              | `nvidia`          | `https://integrate.api.nvidia.com/v1`               | OpenAI     | [取得](https://build.nvidia.com)                                 |
| **Ollama**              | `ollama`          | `http://localhost:11434/v1`                         | OpenAI     | ローカル（キー不要）                                             |
| **LM Studio**           | `lmstudio`        | `http://localhost:1234/v1`                          | OpenAI     | 任意（ローカルのデフォルト：キー不要）                           |
| **OpenRouter**          | `openrouter`      | `https://openrouter.ai/api/v1`                      | OpenAI     | [取得](https://openrouter.ai/keys)                               |
| **LiteLLM Proxy**       | `litellm`         | `http://localhost:4000/v1`                          | OpenAI     | LiteLLM プロキシのキー                                           |
| **VLLM**                | `vllm`            | `http://localhost:8000/v1`                          | OpenAI     | ローカル                                                         |
| **Cerebras**            | `cerebras`        | `https://api.cerebras.ai/v1`                        | OpenAI     | [取得](https://cerebras.ai)                                      |
| **VolcEngine (Doubao)** | `volcengine`      | `https://ark.cn-beijing.volces.com/api/v3`          | OpenAI     | [取得](https://www.volcengine.com) |
| **神算云**              | `shengsuanyun`    | `https://router.shengsuanyun.com/api/v1`            | OpenAI     | —                                                                |
| **BytePlus**            | `byteplus`        | `https://ark.ap-southeast.bytepluses.com/api/v3`    | OpenAI     | [取得](https://www.byteplus.com)                                 |
| **Vivgrid**             | `vivgrid`         | `https://api.vivgrid.com/v1`                        | OpenAI     | [取得](https://vivgrid.com)                                      |
| **LongCat**             | `longcat`         | `https://api.longcat.chat/openai`                   | OpenAI     | [取得](https://longcat.chat/platform)                            |
| **ModelScope (魔搭)**   | `modelscope`      | `https://api-inference.modelscope.cn/v1`            | OpenAI     | [トークン取得](https://modelscope.cn/my/tokens)                  |
| **Antigravity**         | `antigravity`     | Google Cloud                                        | Custom     | OAuth のみ                                                       |
| **GitHub Copilot**      | `github-copilot`  | `localhost:4321`                                    | gRPC       | —                                                                |

#### 基本設定

```json
{
  "model_list": [
    {
      "model_name": "ark-code-latest",
      "provider": "volcengine",
      "model": "ark-code-latest",
      "api_keys": ["sk-your-api-key"]
    },
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_keys": ["sk-your-openai-key"]
    },
    {
      "model_name": "claude-sonnet-4.6",
      "provider": "anthropic",
      "model": "claude-sonnet-4.6",
      "api_keys": ["sk-ant-your-key"]
    },
    {
      "model_name": "glm-4.7",
      "provider": "zhipu",
      "model": "glm-4.7",
      "api_keys": ["your-zhipu-key"]
    }
  ],
  "agents": {
    "defaults": {
      "model_name": "gpt-5.4"
    }
  }
}
```

> **セキュリティ注意**: 設定から `api_keys` フィールドを削除し、代わりに `.security.yml` に保存できます。詳細は上の [セキュリティ設定](#-セキュリティ設定推奨) を参照してください。
>
> **注意**: `enabled` フィールドを `false` にすると、モデルエントリを削除せずに無効にできます。省略した場合、API キーを持つモデルでは移行時に `true` がデフォルトになります。

解決ルール：

- 明示的な `"provider": "openai", "model": "gpt-5.4"` を推奨します。
- `provider` を指定した場合、NeuroClaw は `model` をそのまま送ります。
- `provider` を省略した場合、NeuroClaw は `model` の最初の `/` までをプロバイダー、最初の `/` 以降をランタイムのモデル ID として扱います。
- したがって `"model": "openrouter/openai/gpt-5.4"` も互換形式として引き続き動作し、OpenRouter には `openai/gpt-5.4` が送られます。

#### ストリーミング設定

Provider ストリーミングは二重の opt-in 方式で、デフォルトでは無効です。現在の channel に `settings.streaming.enabled: true` があり、アクティブなモデルエントリに `streaming.enabled: true` があり、さらに provider と channel の両方がストリーミングをサポートしている場合にのみ、agent はストリーミングリクエストを試行します。いずれかの条件が欠ける場合、NeuroClaw は通常の非ストリーミングリクエスト経路を使います。

Pico WebUI が最初に完全対応した channel です。Pico は既存の `message.create` wire message で最初の assistant メッセージを作成し、その後 `message.update` で同じメッセージを更新します。新しい Pico wire message type は追加されません。

ストリーミングを使わない場合は `streaming` を省略してください。`streaming` ブロックの省略は無効を意味するため、`"streaming": {"enabled": false}` を書く必要はありません。

有効化例：

```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_keys": ["sk-your-openai-key"],
      "streaming": {
        "enabled": true
      }
    }
  ],
  "channel_list": {
    "pico": {
      "enabled": true,
      "type": "pico",
      "settings": {
        "token": "YOUR_PICO_TOKEN",
        "streaming": {
          "enabled": true
        }
      }
    }
  }
}
```

| フィールド | 型 | デフォルト | 説明 |
| ---------- | -- | ---------- | ---- |
| `channel_list.<name>.settings.streaming.enabled` | bool | `false` | この channel で provider のストリーミング出力を表示できるようにします |
| `channel_list.<name>.settings.streaming.throttle_seconds` | int | Pico で有効化後のデフォルト：`0` | 中間更新の最小間隔。最終内容は常に flush されます |
| `channel_list.<name>.settings.streaming.min_growth_chars` | int | Pico で有効化後のデフォルト：`1` | 次の中間更新を送るために必要な最小文字増加数。最終内容は常に flush されます |
| `model_list[].streaming.enabled` | bool | `false` | このモデルエントリで provider ストリーミングリクエストを試行できるようにします |

既存の Telegram 環境変数 `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_ENABLED`、`NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_THROTTLE_SECONDS`、`NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_MIN_GROWTH_CHARS` は互換性のため引き続き使えます。これらは Telegram settings にのみ適用され、Pico の `settings.streaming` を有効化または変更しません。

失敗時の動作は保守的です。可視 chunk が送信される前にストリーミングが失敗した場合、NeuroClaw は通常の `Chat()` 経路で一度だけ再試行します。すでに chunk がユーザーに表示されている場合は、表示済み出力の重複を避けるため、二つ目の非ストリーミング回答は送信しません。

モデル固有の TTS リクエストフィールド（カスタムの音声 `voice` 名や `response_format: "mp3"` など）には、`model_list[].extra_body` を使います。

#### Vendor 別の設定例

> **ヒント**: `api_key` フィールドを省略し、`.security.yml` に保存すると、より安全です。[セキュリティ設定](#-セキュリティ設定推奨) を参照してください。

<details>
<summary><b>OpenAI</b></summary>

```json
{
  "model_name": "gpt-5.4",
  "provider": "openai",
  "model": "gpt-5.4"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>OpenRouter TTS (MAI Voice 2)</b></summary>

```json
{
  "model_name": "mai-voice-2",
  "provider": "openrouter",
  "model": "microsoft/mai-voice-2",
  "api_base": "https://openrouter.ai/api/v1",
  "extra_body": {
    "voice": "en-US-Harper:MAI-Voice-2",
    "response_format": "mp3"
  }
  // api_key: set in .security.yml
}
```

次の設定と組み合わせます：

```json
{
  "voice": {
    "tts_model_name": "mai-voice-2"
  }
}
```

</details>

<details>
<summary><b>VolcEngine (Doubao)</b></summary>

```json
{
  "model_name": "ark-code-latest",
  "provider": "volcengine",
  "model": "ark-code-latest"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>智谱 AI (GLM)</b></summary>

```json
{
  "model_name": "glm-4.7",
  "provider": "zhipu",
  "model": "glm-4.7"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>DeepSeek</b></summary>

```json
{
  "model_name": "deepseek-chat",
  "provider": "deepseek",
  "model": "deepseek-chat"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>Anthropic</b></summary>

```json
{
  "model_name": "claude-sonnet-4.6",
  "provider": "anthropic",
  "model": "claude-sonnet-4.6"
  // api_key: set in .security.yml
}
```

> `neuroclaw auth login --provider anthropic` を実行して API トークンを貼り付けます。

Anthropic のネイティブなメッセージ形式のみをサポートする、Anthropic API への直接アクセスやカスタムエンドポイントの場合：

```json
{
  "model_name": "claude-opus-4-6",
  "provider": "anthropic-messages",
  "model": "claude-opus-4-6",
  "api_keys": ["sk-ant-your-key"],
  "api_base": "https://api.anthropic.com"
}
```

> エンドポイントが OpenAI 互換の `/v1/chat/completions` ではなく、Anthropic ネイティブの `/v1/messages` 形式を必要とする場合は、`anthropic-messages` を使います。

</details>

<details>
<summary><b>Ollama (local)</b></summary>

```json
{
  "model_name": "llama3",
  "provider": "ollama",
  "model": "llama3"
}
```

</details>

<details>
<summary><b>LM Studio (local)</b></summary>

```json
{
  "model_name": "lmstudio-local",
  "provider": "lmstudio",
  "model": "openai/gpt-oss-20b"
}
```

`api_base` のデフォルトは `http://localhost:1234/v1` です。LM Studio サーバーで認証を有効にしていない限り、API キーは任意です。<br/>
`provider` を明示した場合、NeuroClaw は `openai/gpt-oss-20b` をそのまま LM Studio に送ります。`provider` を省略した場合も、従来の互換形式 `"model": "lmstudio/openai/gpt-oss-20b"` は同じアップストリームのモデル ID に解決されます。

</details>

<details>
<summary><b>Custom Proxy / LiteLLM</b></summary>

```json
{
  "model_name": "my-custom-model",
  "provider": "openai",
  "model": "custom-model",
  "api_base": "https://my-proxy.com/v1"
  // api_key: set in .security.yml
}
```

`provider` を明示した場合、NeuroClaw は `model` をそのまま送ります。つまり `"provider": "litellm", "model": "lite-gpt4"` は `lite-gpt4` を送り、`"provider": "litellm", "model": "openai/gpt-4o"` は `openai/gpt-4o` を送ります。`provider` を省略した場合も、従来の互換形式 `litellm/lite-gpt4` と `litellm/openai/gpt-4o` は同じように解決されます。

</details>

#### ロードバランシング

同じモデル名に複数のエンドポイントを設定すると、NeuroClaw が自動的にラウンドロビンします。

**方法 1: .security.yml に複数の API キーを置く（推奨）**

```yaml
# .security.yml
model_list:
  gpt-5.4:
    api_keys:
      - "sk-proj-key-1"
      - "sk-proj-key-2"
```

```json
// config.json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_base": "https://api.openai.com/v1"
      // api_keys loaded from .security.yml
    }
  ]
}
```

**方法 2: 複数のモデルエントリ**

```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_base": "https://api1.example.com/v1",
      "api_keys": ["sk-key1"]
    },
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_base": "https://api2.example.com/v1",
      "api_keys": ["sk-key2"]
    }
  ]
}
```

#### 旧 `providers` 設定からの移行

旧 `providers` 設定は**非推奨**となり、V2 で削除されました。既存の V0/V1 設定は自動的に移行されます。完全なガイドは [docs/migration/model-list-migration.md](../migration/model-list-migration.md) を参照してください。

### Provider アーキテクチャ

NeuroClaw はプロトコルファミリーで Provider をルーティングします。

- **OpenAI 互換**：OpenRouter、Groq、Zhipu、vLLM スタイルのエンドポイントなど、ほとんどが該当します。
- **Gemini ネイティブ**：Google Gemini のネイティブ `models/*:generateContent` / `models/*:streamGenerateContent` エンドポイント。
- **Anthropic**：Claude ネイティブ API の動作。
- **Codex/OAuth**：OpenAI OAuth/トークン認証ルート。

これによりランタイムを軽量に保ちつつ、新しい OpenAI 互換バックエンドの追加をほぼ設定操作（`api_base` + `api_keys`）のみで実現します。

<details>
<summary><b>Zhipu (legacy providers format)</b></summary>

```json
{
  "agents": {
    "defaults": {
      "workspace": "~/.neuroclaw/workspace",
      "model": "glm-4.7",
      "max_tokens": 8192,
      "temperature": 0.7,
      "max_tool_iterations": 20,
      "max_parallel_turns": 1
    }
  },
  "providers": {
    "zhipu": {
      "api_key": "Your API Key",
      "api_base": "https://open.bigmodel.cn/api/paas/v4"
    }
  }
}
```

> **注意**: `providers` 形式は非推奨です。より安全な、`.security.yml` を併用する新しい `model_list` 形式を使ってください。
>
> **`max_parallel_turns`**: 異なるセッションからのメッセージの同時処理を制御します。`1`（デフォルト）= 逐次、`>1` = 並列。同じセッションのメッセージは常に直列化されます。詳細は [Steering ドキュメント](../architecture/steering.md) を参照してください。

</details>

<details>
<summary><b>Full config example</b></summary>

```json
{
  "agents": {
    "defaults": {
      "model_name": "claude-opus-4-5"
    }
  },
  "session": {
    "dm_scope": "per-channel-peer",
    "backlog_limit": 20
  },
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      // token: set in .security.yml
      "allow_from": ["123456789"]
    }
  },
  "tools": {
    "web": {
      "duckduckgo": {
        "enabled": true,
        "max_results": 5
      }
    }
  },
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

> **注意**: 機密フィールド（`api_key`、`token` など）は省略して `.security.yml` に保存できます。

</details>

### スケジュールタスク / リマインダー

NeuroClaw は `cron` ツールを通じて cron スタイルのスケジュールタスクをサポートします。Agent は、指定した時刻に実行されるリマインダーや定期ジョブを設定、一覧表示、キャンセルできます。

```json
{
  "tools": {
    "cron": {
      "enabled": true,
      "exec_timeout_minutes": 5,
      "allow_command": true,
      "command_allowed_remotes": []
    }
  }
}
```

スケジュールタスクは再起動後も `~/.neuroclaw/workspace/cron/` に保存されます。

コマンド cron ジョブはシェルコマンドを実行できます（`allow_command` はデフォルトで `true` で、起動時に `NeuroClaw risk=cron_shell` として報告されます。[Capabilities §2](../reference/capabilities.md) を参照）。デフォルトでは、リモートチャネルはコマンドジョブをスケジュールできません。特定のリモートチャネルを許可するには、`command_allowed_remotes` に `"telegram"` や `"telegram:1234567890"` のような項目を設定します。空でないすべてのチャネルを許可してよい場合にのみ `"*"` を使ってください。`"*"` ワイルドカードは、NeuroClaw と通信できるあらゆるリモートチャネルがシェルコマンドをスケジュールできるようになるため、危険になり得ます。これは `allow_command` を迂回するものではなく（`false` の場合、ジョブには `command_confirm: true` が必要です）、exec の安全チェックも迂回しません。

### 高度なトピック

| トピック | 説明 |
| -------- | ---- |
| [セキュリティ設定](../security/security_configuration.md) | API キーとシークレットを別の `.security.yml` ファイルに保存する |
| [機密データのフィルタリング](../security/sensitive_data_filtering.md) | LLM に送る前に、ツール結果から API キーとトークンを除去する |
| [Hook システム](../architecture/hooks/README.md) | イベント駆動 Hook：オブザーバー、インターセプター、承認 Hook |
| [Steering](../architecture/steering.md) | ツール呼び出しの合間に、実行中の Agent ループへメッセージを注入 |
| [SubTurn](../architecture/subturn.md) | サブ Agent の調整、並行制御、ライフサイクル |
| [コンテキスト管理](../architecture/agent-refactor/context.md) | コンテキスト境界検出、事前のバジェットチェック、圧縮 |
