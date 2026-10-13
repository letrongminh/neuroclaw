# 💬 チャットアプリ設定

> [README](../project/README.ja.md) に戻る

## 💬 チャットアプリ連携

Telegram、Discord、WhatsApp、Matrix、QQ、DingTalk、LINE、WeCom、Feishu、Slack、IRC、OneBot、MQTT、MaixCam、Pico（ネイティブプロトコル）を通じて NeuroClaw と会話できます。

> **注意**: HTTP コールバックに依存するチャネルは、単一の共有 Gateway HTTP サーバー（`gateway.host`:`gateway.port`、デフォルト `127.0.0.1:18790`）を使用します。Feishu、DingTalk、WeCom のようなソケット/ストリームベースのチャネルは、受信に共有 Webhook サーバーを使用しません。

> **すべてのチャネルで `allow_from` を設定してください。** `allow_from` が空の場合、ボットに到達できる誰でもボットと会話できます（Gateway は警告 `SECURITY: Channel allows EVERYONE` をログに出力するだけです）。自分のユーザー ID を列挙するか、公開アクセスが意図したものであることを明示するために `["*"]` を使用してください。以下の例の多くは簡潔さのために `"allow_from": []` と記載していますが、ツールを実行できるボットでは空のままにしないでください。チャットのユーザーがエージェントに実行させられること（シェル、スケジュールされたコマンド、スキルのインストール）は、リスクラベルとともに [Capabilities §2](../reference/capabilities.md) に一覧があります。

| チャネル             | セットアップ難易度 | 特徴                                      | ドキュメント                                                                                                    |
| -------------------- | ------------------ | ----------------------------------------- | --------------------------------------------------------------------------------------------------------------- |
| **Telegram**         | ⭐ 簡単            | 推奨、音声テキスト変換対応、ロングポーリング（公開 IP 不要） | [ドキュメント](../channels/telegram/README.ja.md)                                                             |
| **Discord**          | ⭐ 簡単            | Gateway WebSocket（公開 IP 不要）、グループ/DM 対応 | [ドキュメント](../channels/discord/README.ja.md)                                                              |
| **WhatsApp**         | ⭐ 簡単            | ネイティブ (QR スキャン) または Bridge URL | [ドキュメント](#whatsapp)                                                             |
| **微信 (Weixin)**    | ⭐ 簡単            | ネイティブ QR スキャン（Tencent iLink API）| [ドキュメント](#weixin)                                                               |
| **Slack**            | ⭐ 簡単            | **Socket Mode** (公開 IP 不要)、エンタープライズ対応 | [ドキュメント](../channels/slack/README.ja.md)                                                                |
| **Matrix**           | ⭐⭐ 中程度        | フェデレーションプロトコル、セルフホスト対応 | [ドキュメント](../channels/matrix/README.ja.md)                                                              |
| **QQ**               | ⭐⭐ 中程度        | 公式ボット API、中国コミュニティ向け       | [ドキュメント](../channels/qq/README.ja.md)                                                                   |
| **DingTalk**         | ⭐⭐ 中程度        | Stream モード（公開 IP 不要）、企業向け    | [ドキュメント](../channels/dingtalk/README.ja.md)                                                             |
| **LINE**             | ⭐⭐⭐ やや難      | HTTPS Webhook が必要                       | [ドキュメント](../channels/line/README.ja.md)                                                                 |
| **WeCom (企業微信)** | ⭐⭐⭐ やや難      | 公式 AI Bot（WebSocket）、ストリーミング + メディア対応 | [ドキュメント](../channels/wecom/README.ja.md) |
| **Feishu (飛書)**    | ⭐⭐⭐ やや難      | エンタープライズコラボレーション、機能豊富 | [ドキュメント](../channels/feishu/README.ja.md)                                                               |
| **IRC**              | ⭐⭐ 中程度        | サーバー + TLS 設定                        | [ドキュメント](#irc) |
| **OneBot**           | ⭐⭐ 中程度        | NapCat/Go-CQHTTP 互換、コミュニティエコシステム充実 | [ドキュメント](../channels/onebot/README.ja.md)                                                               |
| **MQTT**             | ⭐ 簡単            | ブローカー経由で任意の MQTT クライアントと通信 | [ドキュメント](../channels/mqtt/README.ja.md)                                                                 |
| **MaixCam**          | ⭐ 簡単            | Sipeed AI カメラハードウェア統合チャネル   | [ドキュメント](../channels/maixcam/README.ja.md)                                                              |
| **Pico**             | ⭐ 簡単            | NeuroClaw ネイティブプロトコルチャネル     |                                                                                                               |

<a id="telegram"></a>
<details>
<summary><b>Telegram</b>（推奨）</summary>

**1. Bot を作成**

* Telegram を開き、`@BotFather` を検索
* `/newbot` を送信し、プロンプトに従う
* Token をコピー

**2. 設定**

```json
{
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"]
    }
  }
}
```

> Telegram の `@userinfobot` から User ID を取得できます。

**3. 実行**

```bash
neuroclaw gateway
```

**4. Telegram コマンドメニュー（起動時に自動登録）**

NeuroClaw は統一されたコマンド定義を使用します。起動時に Telegram がサポートするコマンド（例: `/start`、`/help`、`/show`、`/list`、`/use`、`/btw`）を Bot コマンドメニューに自動登録し、メニュー表示と実際の動作を一致させます。
Telegram 側はコマンドメニュー登録機能を保持し、汎用コマンドの実行は Agent Loop 内の commands executor で統一的に処理されます。

ネットワークや API の一時的なエラーで登録に失敗しても、チャネルの起動はブロックされません。システムがバックグラウンドで自動リトライします。

Telegram からスキルや MCP サーバーを直接確認することもできます：

- `/list skills`
- `/list mcp`
- `/show mcp <server>`
- `/use <skill> <message>`
- `/use <skill>` の後、次のメッセージで実際のリクエストを送信
- `/use clear`
- `/btw <question>`：アクティブなセッション履歴を変更せずに、すぐに脇道の質問をします。`/btw` はツールを使わないクエリとして処理され、通常のツール実行フローには入りません

**5. 高度なフォーマット**
`use_markdown_v2: true` を設定すると、拡張フォーマットオプションが有効になります。これにより、ボットは Telegram MarkdownV2 の全機能（ネストされたスタイル、スポイラー、カスタム固定幅ブロックなど）を利用できます。

</details>

<a id="discord"></a>
<details>
<summary><b>Discord</b></summary>

**1. Bot を作成**

* <https://discord.com/developers/applications> にアクセス
* アプリケーションを作成 → Bot → Bot を追加
* Bot Token をコピー

**2. Intents を有効化**

* Bot 設定で **MESSAGE CONTENT INTENT** を有効化
* （オプション）メンバーデータに基づくホワイトリストが必要な場合は **SERVER MEMBERS INTENT** を有効化

**3. User ID を取得**

* Discord 設定 → 詳細設定 → **開発者モード** を有効化
* アバターを右クリック → **ユーザー ID をコピー**

**4. 設定**

```json
{
  "channel_list": {
    "discord": {
      "enabled": true,
      "type": "discord",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"]
    }
  }
}
```

**5. Bot を招待**

* OAuth2 → URL Generator
* Scopes: `bot`
* Bot Permissions: `Send Messages`, `Read Message History`
* 生成された招待リンクを開き、Bot をサーバーに追加

**オプション：グループトリガーモード**

デフォルトでは Bot はサーバーチャネル内のすべてのメッセージに応答します。@メンション時のみ応答するには：

```json
{
  "channel_list": {
    "discord": {
      "group_trigger": { "mention_only": true }
    }
  }
}
```

キーワードプレフィックスでトリガーすることもできます（例: `!bot`）：

```json
{
  "channel_list": {
    "discord": {
      "group_trigger": { "prefixes": ["!bot"] }
    }
  }
}
```

**6. 実行**

```bash
neuroclaw gateway
```

</details>

<a id="whatsapp"></a>
<details>
<summary><b>WhatsApp</b>（ネイティブ whatsmeow）</summary>

NeuroClaw は 2 つの WhatsApp 接続方式をサポートしています：

- **ネイティブ（推奨）：** プロセス内で [whatsmeow](https://github.com/tulir/whatsmeow) を使用。独立した Bridge は不要です。`"use_native": true` に設定し、`bridge_url` を空にします。初回実行時に WhatsApp で QR コードをスキャン（リンクデバイス）。セッションはワークスペース配下（例: `workspace/whatsapp/`）に保存されます。ネイティブチャネルは**オプション**ビルドで、`-tags whatsapp_native` でコンパイルします（例: `make build-whatsapp-native` または `go build -tags whatsapp_native ./cmd/...`）。
- **Bridge：** 外部 WebSocket Bridge に接続。`bridge_url`（例: `ws://localhost:3001`）を設定し、`use_native` を false のままにします。

**設定（ネイティブ）**

```json
{
  "channel_list": {
    "whatsapp": {
      "enabled": true,
      "type": "whatsapp",
      "use_native": true,
      "session_store_path": "",
      "allow_from": []
    }
  }
}
```

`session_store_path` が空の場合、セッションは `<workspace>/whatsapp/` に保存されます。`neuroclaw gateway` を実行し、初回実行時にターミナルに表示される QR コードをスキャンしてください（WhatsApp → リンクデバイス）。

</details>

<a id="weixin"></a>
<details>
<summary><b>微信 (Weixin)</b></summary>

NeuroClaw は Tencent iLink 公式 API を使用して WeChat 個人アカウントへの接続をサポートしています。

**1. ログイン**

インタラクティブな QR ログインフローを実行します：
```bash
neuroclaw auth weixin
```
WeChat モバイルアプリで表示された QR コードをスキャンしてください。ログイン成功後、トークンが設定ファイルに保存されます。

**2. 設定**

（オプション）ボットと会話できるユーザーを制限するために `allow_from` に WeChat ユーザー ID を追加します：
```json
{
  "channel_list": {
    "weixin": {
      "enabled": true,
      "type": "weixin",
      "token": "YOUR_TOKEN",
      "allow_from": ["YOUR_USER_ID"]
    }
  }
}
```

**3. 実行**
```bash
neuroclaw gateway
```

</details>

<a id="matrix"></a>
<details>
<summary><b>Matrix</b></summary>

**1. Bot アカウントを準備**

* お好みの homeserver（例: `https://matrix.org` またはセルフホスト）を使用
* Bot ユーザーを作成し、access token を取得

**2. 設定**

```json
{
  "channel_list": {
    "matrix": {
      "enabled": true,
      "type": "matrix",
      "homeserver": "https://matrix.org",
      "user_id": "@your-bot:matrix.org",
      "access_token": "YOUR_MATRIX_ACCESS_TOKEN",
      "allow_from": []
    }
  }
}
```

**3. 実行**

```bash
neuroclaw gateway
```

すべてのオプション（`device_id`、`join_on_invite`、`group_trigger`、`placeholder`、`reasoning_channel_id`）については [Matrix チャネル設定ガイド](../channels/matrix/README.md) を参照してください。

</details>

<a id="qq"></a>
<details>
<summary><b>QQ</b></summary>

**クイックセットアップ（推奨）**

QQ 開放プラットフォームでは、OpenClaw 互換ボットのワンクリックセットアップページが提供されています：

1. [QQ Bot クイックスタート](https://q.qq.com/qqbot/openclaw/index.html) を開き、QR コードをスキャンしてログイン
2. ボットが自動的に作成されます — **App ID** と **App Secret** をコピー
3. NeuroClaw を設定：

```json
{
  "channel_list": {
    "qq": {
      "enabled": true,
      "type": "qq",
      "app_id": "YOUR_APP_ID",
      "app_secret": "YOUR_APP_SECRET",
      "allow_from": []
    }
  }
}
```

4. `neuroclaw gateway` を実行し、QQ を開いてボットとチャット

> App Secret は一度しか表示されません。すぐに保存してください — 再度表示するとリセットされます。
>
> クイックセットアップで作成されたボットは、最初は作成者のみが使用でき、グループチャットには対応していません。グループアクセスを有効にするには、[QQ 開放プラットフォーム](https://q.qq.com/) でサンドボックスモードを設定してください。

**手動セットアップ**

ボットを手動で作成する場合：

* [QQ 開放プラットフォーム](https://q.qq.com/) にログインして開発者登録
* QQ ボットを作成 — アバターと名前をカスタマイズ
* ボット設定から **App ID** と **App Secret** をコピー
* 上記の設定を行い、`neuroclaw gateway` を実行

</details>

<a id="slack"></a>
<details>
<summary><b>Slack</b></summary>

**1. Slack App を作成**

* [Slack API](https://api.slack.com/apps) にアクセスして新しいアプリを作成
* **OAuth & Permissions** で Bot スコープを追加：`chat:write`、`app_mentions:read`、`im:history`、`im:read`、`im:write`
* アプリをワークスペースにインストール
* **Bot Token**（`xoxb-...`）と **App-Level Token**（`xapp-...`、Socket Mode を有効にして取得）をコピー

**2. 設定**

```json
{
  "channel_list": {
    "slack": {
      "enabled": true,
      "type": "slack",
      "bot_token": "xoxb-YOUR-BOT-TOKEN",
      "app_token": "xapp-YOUR-APP-TOKEN",
      "allow_from": []
    }
  }
}
```

**3. 実行**

```bash
neuroclaw gateway
```

</details>

<a id="irc"></a>
<details>
<summary><b>IRC</b></summary>

**1. 設定**

```json
{
  "channel_list": {
    "irc": {
      "enabled": true,
      "type": "irc",
      "server": "irc.libera.chat:6697",
      "tls": true,
      "nick": "neuroclaw-bot",
      "channels": ["#your-channel"],
      "password": "",
      "allow_from": []
    }
  }
}
```

オプション：NickServ 認証用の `nickserv_password`、SASL 認証用の `sasl_user`/`sasl_password`。

**2. 実行**

```bash
neuroclaw gateway
```

ボットは IRC サーバーに接続し、指定されたチャネルに参加します。

</details>

<a id="dingtalk"></a>
<details>
<summary><b>DingTalk</b></summary>

**1. Bot を作成**

* [開放プラットフォーム](https://open.dingtalk.com/) にアクセス
* 内部アプリを作成
* Client ID と Client Secret をコピー

**2. 設定**

```json
{
  "channel_list": {
    "dingtalk": {
      "enabled": true,
      "type": "dingtalk",
      "client_id": "YOUR_CLIENT_ID",
      "client_secret": "YOUR_CLIENT_SECRET",
      "allow_from": []
    }
  }
}
```

> アクセスを制限するには、`allow_from` に DingTalk ユーザー ID を指定してください。空の場合は全ユーザーを許可します。明示するには `["*"]` を使用してください。

**3. 実行**

```bash
neuroclaw gateway
```

</details>

<a id="line"></a>
<details>
<summary><b>LINE</b></summary>

**1. LINE 公式アカウントを作成**

- [LINE Developers Console](https://developers.line.biz/) にアクセス
- Provider を作成 → Messaging API チャネルを作成
- **Channel Secret** と **Channel Access Token** をコピー

**2. 設定**

```json
{
  "channel_list": {
    "line": {
      "enabled": true,
      "type": "line",
      "channel_secret": "YOUR_CHANNEL_SECRET",
      "channel_access_token": "YOUR_CHANNEL_ACCESS_TOKEN",
      "webhook_path": "/webhook/line",
      "allow_from": []
    }
  }
}
```

> LINE Webhook は共有 Gateway サーバー（`gateway.host`:`gateway.port`、デフォルト `127.0.0.1:18790`）上で提供されます。

**3. Webhook URL を設定**

LINE は HTTPS Webhook が必要です。リバースプロキシまたはトンネルを使用してください：

```bash
# 例：ngrok を使用（Gateway デフォルトポートは 18790）
ngrok http 18790
```

LINE Developers Console で Webhook URL を `https://your-domain/webhook/line` に設定し、**Use webhook** を有効にしてください。

**4. 実行**

```bash
neuroclaw gateway
```

> グループチャットでは、Bot は @メンション時のみ応答します。返信は元のメッセージを引用します。

</details>

<a id="feishu"></a>
<details>
<summary><b>Feishu (飛書)</b></summary>

NeuroClaw は WebSocket/SDK モードで飛書に接続します — 公開 Webhook URL やコールバックサーバーは不要です。

**1. アプリを作成**

* [飛書開放プラットフォーム](https://open.feishu.cn/) にアクセスしてアプリケーションを作成
* アプリ設定で **ボット** 機能を有効化
* バージョンを作成してアプリを公開（アプリは公開しないと有効になりません）
* **App ID**（`cli_` で始まる）と **App Secret** をコピー

**2. 設定**

```json
{
  "channel_list": {
    "feishu": {
      "enabled": true,
      "type": "feishu",
      "app_id": "cli_xxx",
      "app_secret": "YOUR_APP_SECRET",
      "allow_from": []
    }
  }
}
```

オプション：`encrypt_key` と `verification_token` でイベント暗号化（本番環境推奨）。

**3. 実行してチャット**

```bash
neuroclaw gateway
```

飛書を開き、ボット名を検索してチャットを開始できます。ボットをグループに追加することもできます — `group_trigger.mention_only: true` を設定すると @メンション時のみ応答します。

詳細なオプションについては [飛書チャネル設定ガイド](../channels/feishu/README.ja.md) を参照してください。

</details>

<a id="wecom"></a>
<details>
<summary><b>WeCom (企业微信)</b></summary>

NeuroClaw は WeCom を、WebSocket 経由の単一の AI Bot チャネルとして提供します。
公開 Webhook コールバック URL は不要です。

設定の詳細リファレンスと移行に関する注意は [WeCom 設定ガイド](../channels/wecom/README.ja.md) を参照してください。

**クイックセットアップ（推奨）**

**1. 認証**

```bash
neuroclaw auth wecom
```

このコマンドは QR コードを表示し、WeCom での承認を待って、`bot_id` と `secret` を `channel_list` の `wecom` エントリに書き込みます。

**2. 必要に応じて手動で設定**

```json
{
  "channel_list": {
    "wecom": {
      "enabled": true,
      "type": "wecom",
      "bot_id": "YOUR_BOT_ID",
      "secret": "YOUR_SECRET",
      "websocket_url": "wss://openws.work.weixin.qq.com",
      "send_thinking_message": true,
      "allow_from": [],
      "reasoning_channel_id": ""
    }
  }
}
```

**3. 実行**

```bash
neuroclaw gateway
```

> 旧 `wecom_app` と `wecom_aibot` のエントリは、`channel_list` 配下の統合された `wecom` エントリに置き換えられました。

</details>

<a id="onebot"></a>
<details>
<summary><b>OneBot（OneBot プロトコル経由の QQ）</b></summary>

OneBot は QQ ボット向けのオープンプロトコルです。NeuroClaw は OneBot v11 互換の実装（例：[Lagrange](https://github.com/LagrangeDev/Lagrange.Core)、[NapCat](https://github.com/NapNeko/NapCatQQ)）に WebSocket で接続します。

**1. OneBot 実装をセットアップ**

OneBot v11 互換の QQ ボットフレームワークをインストールして実行します。WebSocket サーバーを有効にしてください。

**2. 設定**

```json
{
  "channel_list": {
    "onebot": {
      "enabled": true,
      "type": "onebot",
      "ws_url": "ws://127.0.0.1:8080",
      "access_token": "",
      "allow_from": []
    }
  }
}
```

| フィールド | 説明 |
|-------|-------------|
| `ws_url` | OneBot 実装の WebSocket URL |
| `access_token` | 認証用アクセストークン（OneBot 側で設定している場合） |
| `reconnect_interval` | 再接続間隔（秒）（デフォルト：5） |

**3. 実行**

```bash
neuroclaw gateway
```

</details>

<a id="mqtt"></a>
<details>
<summary><b>MQTT</b></summary>

任意の MQTT クライアントがブローカーを介して NeuroClaw と通信できます。デバイスやサービスがブローカーにリクエストをパブリッシュし、NeuroClaw がサブスクライブして処理し、レスポンスをパブリッシュして返します。

**1. 設定**

```json
{
  "channel_list": {
    "mqtt": {
      "enabled": true,
      "type": "mqtt",
      "settings": {
        "broker": "ssl://your-broker:8883",
        "agent_id": "assistant",
        "topic_prefix": "/neuroclaw",
        "keep_alive": 60,
        "qos": 0
      }
    }
  }
}
```

ユーザー名とパスワードは `~/.neuroclaw/.security.yml` に記載します：

```yaml
channel_list:
  mqtt:
    settings:
      username: your_username
      password: your_password
```

**トピック形式**

```
{prefix}/{agent_id}/{client_id}/request    # クライアント → NeuroClaw
{prefix}/{agent_id}/{client_id}/response   # NeuroClaw → クライアント
```

`client_id` はクライアントアプリケーションがデバイスやセッションを識別するために設定します。

**2. 起動**

```bash
neuroclaw gateway
```

**3. テスト**

```bash
mosquitto_pub -t "/neuroclaw/assistant/device1/request" \
  -m '{"text": "こんにちは"}'

mosquitto_sub -t "/neuroclaw/assistant/device1/response"
```

完全な設定オプションは [MQTT チャンネルドキュメント](../channels/mqtt/README.ja.md) を参照してください。

</details>
