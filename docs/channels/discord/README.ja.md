> [README](../../project/README.ja.md) に戻る

# Discord

Discord はコミュニティ向けに設計された無料の音声・ビデオ・テキストチャットアプリケーションです。NeuroClaw は Discord Bot API を通じて Discord サーバーに接続し、メッセージの受信と送信をサポートします。

## 設定

```json
{
  "agents": {
    "defaults": {
      "tool_feedback": {
        "enabled": true,
        "max_args_length": 300
      }
    }
  },
  "channel_list": {
    "discord": {
      "enabled": true,
      "type": "discord",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"],
      "placeholder": {
        "enabled": true,
        "text": ["Thinking... 💭"]
      },
      "group_trigger": {
        "mention_only": false
      },
      "reasoning_channel_id": ""
    }
  }
}
```

| フィールド    | 型     | 必須   | 説明                                                              |
| ------------- | ------ | ------ | ----------------------------------------------------------------- |
| enabled       | bool   | はい   | Discord チャンネルを有効にするかどうか                            |
| token         | string | はい   | Discord ボットトークン                                            |
| allow_from    | array  | いいえ | 許可するユーザーIDのリスト。空の場合は誰でも許可されます（Gateway が警告をログに出力します）。設定するか、全員を意図的に許可する場合は `["*"]` を使用してください |
| placeholder   | object | いいえ | エージェントの処理中に表示するプレースホルダーメッセージの設定    |
| group_trigger | object | いいえ | グループトリガー設定（例: { "mention_only": false }）             |
| reasoning_channel_id | string | いいえ | 推論（thinking）出力の送信先チャンネル ID（任意）          |

## 実行中フィードバックの表示

Discord では、3 種類の「処理中」フィードバックを表示できます：

1. 入力中インジケーター：自動で表示され、追加の設定は不要です。
2. プレースホルダーメッセージ：`channel_list.discord.placeholder.enabled` を有効にすると、目に見える `Thinking...` メッセージが送信され、後で最終的な返信に編集されます。
3. ツール実行フィードバック：`agents.defaults.tool_feedback.enabled` を有効にすると、各ツール呼び出しの前に短いメッセージが送信されます。例：

```text
🔧 `web_search`
Checking the latest NeuroClaw release notes before I answer.
```

`Bot is typing` しか表示されない場合は、実行時の設定で `placeholder.enabled` または `tool_feedback.enabled` が実際に設定されているか確認してください。

## セットアップ手順

1. [Discord 開発者ポータル](https://discord.com/developers/applications) にアクセスして新しいアプリケーションを作成する
2. Intents を有効にする:
   - Message Content Intent
   - Server Members Intent
3. Bot トークンを取得する
4. 設定ファイルに Bot トークンを入力する
5. ボットをサーバーに招待し、必要な権限を付与する（例: メッセージの送信、メッセージ履歴の読み取りなど）
