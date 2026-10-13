> [README](../../project/README.ja.md) に戻る

# Telegram

Telegram チャンネルは、Telegram Bot API を使用したロングポーリングによるボットベースの通信を実装しています。テキストメッセージ、メディア添付ファイル（写真、音声、オーディオ、ドキュメント）、音声文字起こし（[設定方法](../../guides/providers.ja.md)）、および組み込みコマンドハンドラーをサポートしています。

## 設定

```json
{
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      "token": "123456789:ABCdefGHIjklMNOpqrsTUVwxyz",
      "allow_from": ["123456789"],
      "proxy": "",
      "use_markdown_v2": false,
      "media_group_delay_ms": 500
    }
  }
}
```

| フィールド      | 型     | 必須 | 説明                                                              |
| --------------- | ------ | ---- | ----------------------------------------------------------------- |
| enabled         | bool   | はい | Telegram チャンネルを有効にするかどうか                           |
| token           | string | はい | Telegram Bot API トークン                                         |
| allow_from      | array  | いいえ | 許可するユーザーIDのリスト。空の場合は誰でも許可されます（Gateway が警告をログに出力します）。設定するか、全員を意図的に許可する場合は `["*"]` を使用してください |
| proxy           | string | いいえ | Telegram API への接続に使用するプロキシ URL (例: http://127.0.0.1:7890) |
| use_markdown_v2 | bool   | いいえ | Telegram MarkdownV2 フォーマットを有効にする                      |
| media_group_delay_ms | int | いいえ | Telegram のメディアグループ／アルバムを処理するまでのアイドル待機時間。デフォルトは 500 ms |

## セットアップ手順

1. Telegram で `@BotFather` を検索する
2. `/newbot` コマンドを送信し、指示に従って新しいボットを作成する
3. HTTP API トークンを取得する
4. 設定ファイルにトークンを入力する
5. (任意) `allow_from` を設定して、対話を許可するユーザー ID を制限する（ID は `@userinfobot` で取得可能）

## 組み込みコマンド

Telegram は起動時に NeuroClaw のトップレベルのボットコマンドを自動登録します。`/start`、`/help`、`/show`、`/list`、`/use` などが含まれます。

スキル関連のコマンド：

- `/list skills` は、現在のエージェントから見えるインストール済みスキルを一覧表示します。
- `/list mcp` は、設定済みの MCP サーバーと、それらが遅延（deferred）／接続済みかどうかを一覧表示します。
- `/show mcp <server>` は、接続済みの MCP サーバーで有効なツールを一覧表示します。
- `/use <skill> <message>` は、1 回のリクエストに対してスキルを強制します。
- `/use <skill>` は、同じチャットでの次のメッセージ用にスキルを待機状態にします。
- `/use clear` は、保留中のスキル指定を解除します。

例：

```text
/list skills
/list mcp
/show mcp github
/use git explain how to squash the last 3 commits
/use git
explain how to squash the last 3 commits
```

## 高度なフォーマット

`use_markdown_v2: true` を設定することで、強化されたフォーマットオプションを有効にできます。これにより、ボットは Telegram MarkdownV2 の全機能（ネストされたスタイル、スポイラー、カスタム固定幅ブロックなど）を利用できます。

```json
{
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"],
      "use_markdown_v2": true
    }
  }
}
```
