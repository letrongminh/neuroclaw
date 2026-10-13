# NeuroClaw のデバッグ

> [README](../project/README.ja.md) に戻る

NeuroClaw は、受信するすべてのリクエストに対して、メッセージのルーティングや複雑度の評価、ツールの実行、モデル障害への適応など、多くの複雑な処理をバックグラウンドで実行しています。何が起きているかを正確に把握できることは、潜在的な問題のトラブルシューティングだけでなく、エージェントの動作を真に理解するためにも非常に重要です。

## デバッグモードで NeuroClaw を起動する

エージェントの動作に関する詳細情報（LLM リクエスト、ツール呼び出し、メッセージルーティング）を取得するには、デバッグフラグを付けて NeuroClaw ゲートウェイを起動します：

```bash
neuroclaw gateway --debug
# or
neuroclaw gateway -d
```

このモードでは、システムがログを詳細にフォーマットし、システムプロンプトやツール実行結果のプレビューを表示します。

## 起動時のリスクラベル

`--debug` の有無にかかわらず、`gateway` と `agent` は、有効になっているリスクのある機能ごとに警告ログ `NeuroClaw risk=<name>` を 1 行出力します（例：`risk=shell`）。このマシンでモデルに何が許可されているかを知りたいときは、まずこれを確認してください。名前と意味：[capabilities.md セクション 2](../reference/capabilities.md#2-risk-labels)。

## ログの切り詰めを無効にする（完全なログ）

デフォルトでは、NeuroClaw はコンソールの可読性を保つために、デバッグログ内の非常に長い文字列（*システムプロンプト*や大きな JSON 出力結果など）を切り詰めます。

コマンドの完全な出力や、LLM モデルに送信された正確なペイロードを確認する必要がある場合は、`--no-truncate` フラグを使用できます。

**注意：** このフラグは `--debug` モードと組み合わせた場合に*のみ*機能します。

```bash
neuroclaw gateway --debug --no-truncate

```

このフラグが有効な場合、グローバルな切り詰め機能が無効になります。これは以下の場合に非常に便利です：

* プロバイダーに送信されるメッセージの正確な構文を確認する。
* `exec`、`web_fetch`、`read_file` などのツールの完全な出力を読む。
* メモリに保存されたセッション履歴をデバッグする。

## デバッグログでのツール呼び出しの可視化

デバッグモードが有効な場合、エージェントはツール実行ライフサイクルの各段階で構造化ログを出力します。これらのエントリには `component=agent` ラベルが付き、詳細度に応じて `INFO` または `DEBUG` レベルになります：

| ログメッセージ | レベル | 主なフィールド | 説明 |
|---|---|---|---|
| `LLM requested tool calls` | INFO | `tools`, `count`, `iteration` | モデルが呼び出すと決めたツール名の一覧 |
| `Tool call: <name>(<args>)` | INFO | `tool`, `iteration` | ツール名と引数のプレビュー（200 文字に切り詰め） |
| `Sent tool result to user` | DEBUG | `tool`, `content_len` | ツール結果がチャットチャンネルに転送されたときに出力 |
| `TTL tick after tool execution` | DEBUG | `agent_id`, `iteration` | 各ツールラウンドの後に行われる MCP ツール検出 TTL の減算 |
| `Async tool completed, publishing result` | INFO | `tool`, `content_len`, `channel` | バックグラウンドで非同期に実行されるツールのみ |

### ツール呼び出しログの読み方

同期ツールの典型的な呼び出しでは、コンソールに連続する 2 行が出力されます：

```
[...] [INFO] agent: LLM requested tool calls {tools=[web_search], count=1, iteration=1}
[...] [INFO] agent: Tool call: web_search({"query":"neuroclaw release notes"}) {tool=web_search, iteration=1}
```

引数のプレビューは `INFO` レベルの経路に属するため、`--no-truncate` フラグの有無にかかわらず、ログ上で **200 文字**に制限されます。モデルに送信されたすべてのツール定義を含む、`Full LLM request` DEBUG エントリの `tools_json` フィールド全体を見るには、`--debug` と一緒に `--no-truncate` を使用してください。

## チャットでのリアルタイムのツールフィードバック（tool_feedback）

デバッグログはサーバー側にのみ出力されます。エージェントがツールを実行するたびに、チャットチャンネルへ直接通知を送らせたい場合（ボットを他のユーザーと共有するときや、透明性のために便利です）は、`config.json` で `tool_feedback` 機能を有効にします：

```json
{
  "agents": {
    "defaults": {
      "tool_feedback": {
        "enabled": true,
        "max_args_length": 300,
        "separate_messages": true
      }
    }
  }
}
```

`enabled` が `true` の場合、ツール結果がモデルに返される前に、ツール呼び出しごとに短いメッセージがチャットへ送信されます。メッセージは次のようになります：

```bash
🔧 `web_search`
{"query": "neuroclaw release notes"}
```


### オプション

| フィールド | 型 | デフォルト | 説明 |
|---|---|---|---|
| `enabled` | bool | `false` | ツール呼び出しごとにチャット通知を送信する |
| `separate_messages` | bool | `false` | 1 つのプレースホルダー／進捗メッセージを再利用せず、ツールフィードバックの更新ごとに別々のチャットメッセージにする |
| `max_args_length` | int | `300` | 通知に含めるシリアライズ済み引数の最大文字数 |

### 環境変数

どちらのフィールドも環境変数で設定できます：

```bash
NEUROCLAW_AGENTS_DEFAULTS_TOOL_FEEDBACK_ENABLED=true
NEUROCLAW_AGENTS_DEFAULTS_TOOL_FEEDBACK_MAX_ARGS_LENGTH=300
```

> **注意：** `tool_feedback` は `--debug` モードとは独立しています。本番環境でも動作し、ゲートウェイを特別なフラグ付きで起動する必要はありません。
