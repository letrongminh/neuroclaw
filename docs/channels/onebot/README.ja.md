> [README](../../project/README.ja.md) に戻る

# OneBot

OneBot は QQ ボット向けのオープンプロトコル標準で、複数の QQ ボット実装（例: go-cqhttp、Mirai）に統一されたインターフェースを提供します。通信には WebSocket を使用します。

## 設定

```json
{
  "channel_list": {
    "onebot": {
      "enabled": true,
      "type": "onebot",
      "ws_url": "ws://localhost:8080",
      "access_token": "",
      "allow_from": []
    }
  }
}
```

| フィールド   | 型     | 必須   | 説明                                                             |
| ------------ | ------ | ------ | ---------------------------------------------------------------- |
| enabled      | bool   | はい   | OneBot チャンネルを有効にするかどうか                            |
| ws_url       | string | はい   | OneBot サーバーの WebSocket URL                                  |
| access_token | string | いいえ | OneBot サーバーへの接続に使用するアクセストークン                |
| allow_from   | array  | いいえ | ユーザーIDのホワイトリスト。空の場合は誰でも許可されます（Gateway が警告をログに出力します）。設定するか、全員を意図的に許可する場合は `["*"]` を使用してください |

## セットアップ手順

1. OneBot 互換の実装（例: napcat）をデプロイする
2. OneBot 実装で WebSocket サービスを有効にし、アクセストークンを設定する（必要な場合）
3. WebSocket URL とアクセストークンを設定ファイルに入力する
