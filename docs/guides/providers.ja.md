# 🔌 プロバイダーとモデル設定

> [README](../project/README.ja.md) に戻る

### プロバイダー

> [!NOTE]
> 音声の文字起こしには、`voice.model_name` で設定したマルチモーダルモデルを使えます。音声モデルが未設定の場合は、フォールバックとして Groq Whisper を引き続き使用できます。

| プロバイダー         | 用途                         | API Key の取得                                                       |
| -------------------- | ---------------------------- | -------------------------------------------------------------------- |
| `gemini`             | LLM (Gemini 直接接続)       | [aistudio.google.com](https://aistudio.google.com)                   |
| `zhipu`              | LLM (Zhipu 直接接続)        | [bigmodel.cn](https://bigmodel.cn)                                   |
| `zai`                | LLM (Z.AI Coding Plan)      | [z.ai](https://z.ai/manage-apikey/apikey-list)                       |
| `volcengine`         | LLM (Volcengine 直接接続)   | [volcengine.com](https://www.volcengine.com)                         |
| `openrouter`         | LLM (推奨、全モデルアクセス可) | [openrouter.ai](https://openrouter.ai)                               |
| `anthropic`          | LLM (Claude 直接接続)       | [console.anthropic.com](https://console.anthropic.com)               |
| `openai`             | LLM (GPT 直接接続)          | [platform.openai.com](https://platform.openai.com)                   |
| `venice`             | LLM (Venice AI 直接接続)    | [venice.ai](https://venice.ai)                                       |
| `nearai`             | LLM (NEAR AI Cloud TEE 推論) | [near.ai](https://near.ai)                                          |
| `deepseek`           | LLM (DeepSeek 直接接続)     | [platform.deepseek.com](https://platform.deepseek.com)               |
| `qwen`               | LLM (Qwen 直接接続)         | [dashscope.console.aliyun.com](https://dashscope.console.aliyun.com) |
| `groq`               | LLM + **音声文字起こし** (Whisper) | [console.groq.com](https://console.groq.com)                         |
| `cerebras`           | LLM (Cerebras 直接接続)     | [cerebras.ai](https://cerebras.ai)                                   |
| `vivgrid`            | LLM (Vivgrid 直接接続)      | [vivgrid.com](https://vivgrid.com)                                   |
| `nvidia`             | LLM (NVIDIA NIM)            | [build.nvidia.com](https://build.nvidia.com)                         |
| `moonshot`           | LLM (Kimi/Moonshot 直接接続) | [platform.moonshot.cn](https://platform.moonshot.cn)                 |
| `minimax`            | LLM (Minimax 直接接続)      | [platform.minimaxi.com](https://platform.minimaxi.com)              |
| `avian`              | LLM (Avian 直接接続)        | [avian.io](https://avian.io)                                         |
| `mistral`            | LLM (Mistral 直接接続)      | [console.mistral.ai](https://console.mistral.ai)                    |
| `longcat`            | LLM (Longcat 直接接続)      | [longcat.ai](https://longcat.ai)                                     |
| `modelscope`         | LLM (ModelScope 直接接続)   | [modelscope.cn](https://modelscope.cn)                               |
| `mimo`               | LLM (Xiaomi MiMo 直接接続)  | [platform.xiaomimimo.com](https://platform.xiaomimimo.com)           |

### モデル設定 (model_list)

> NeuroClaw は、明示的な `provider` とネイティブな `model` を指定する設定（例: `"provider": "zhipu", "model": "glm-4.7"`）を推奨します。`provider` を省略した場合は、従来の `provider/model` 形式（1 つのフィールドにまとめる書き方）も互換性のために引き続き使えます。

Agent の振り分けや軽量モデルへのルーティングの例は、[ルーティングガイド](routing-guide.md)を参照してください。

この設計は**マルチ Agent**にも対応し、柔軟な Provider 選択を提供します：

- **Agent ごとに異なる Provider**: 各 Agent が独自の LLM provider を使用可能
- **モデルフォールバック**: プライマリモデルとフォールバックモデルを設定し、信頼性を向上
- **ロードバランシング**: 複数の API エンドポイント間でリクエストを分散
- **一元管理**: すべての provider を一箇所で管理

#### 📋 サポートされている全ベンダー

| ベンダー            | `provider` の値   | デフォルト API Base                                 | プロトコル | API Key の取得                                                    |
| ------------------- | ----------------- | --------------------------------------------------- | ---------- | ----------------------------------------------------------------- |
| **OpenAI**          | `openai`          | `https://api.openai.com/v1`                         | OpenAI     | [キーを取得](https://platform.openai.com)                         |
| **Venice AI**       | `venice`          | `https://api.venice.ai/api/v1`                      | OpenAI     | [キーを取得](https://venice.ai)                                   |
| **NEAR AI Cloud**   | `nearai`          | `https://cloud-api.near.ai/v1`                      | OpenAI     | [キーを取得](https://near.ai)                                     |
| **Anthropic**       | `anthropic`       | `https://api.anthropic.com/v1`                      | Anthropic  | [キーを取得](https://console.anthropic.com)                       |
| **智谱 AI (GLM)**   | `zhipu`           | `https://open.bigmodel.cn/api/paas/v4`              | OpenAI     | [キーを取得](https://open.bigmodel.cn/usercenter/proj-mgmt/apikeys) |
| **Z.AI Coding Plan** | `openai`         | `https://api.z.ai/api/coding/paas/v4`               | OpenAI     | [キーを取得](https://z.ai/manage-apikey/apikey-list)              |
| **DeepSeek**        | `deepseek`        | `https://api.deepseek.com/v1`                       | OpenAI     | [キーを取得](https://platform.deepseek.com)                       |
| **Google Gemini**   | `gemini`          | `https://generativelanguage.googleapis.com/v1beta`  | Gemini     | [キーを取得](https://aistudio.google.com/api-keys)                |
| **Groq**            | `groq`            | `https://api.groq.com/openai/v1`                    | OpenAI     | [キーを取得](https://console.groq.com)                            |
| **Moonshot**        | `moonshot`        | `https://api.moonshot.cn/v1`                        | OpenAI     | [キーを取得](https://platform.moonshot.cn)                        |
| **通義千問 (Qwen)** | `qwen`            | `https://dashscope.aliyuncs.com/compatible-mode/v1` | OpenAI     | [キーを取得](https://dashscope.console.aliyun.com)                |
| **NVIDIA**          | `nvidia`          | `https://integrate.api.nvidia.com/v1`               | OpenAI     | [キーを取得](https://build.nvidia.com)                            |
| **Ollama**          | `ollama`          | `http://localhost:11434/v1`                         | OpenAI     | ローカル（キー不要）                                              |
| **LM Studio**       | `lmstudio`        | `http://localhost:1234/v1`                          | OpenAI     | 任意（ローカルのデフォルトではキー不要）                          |
| **OpenRouter**      | `openrouter`      | `https://openrouter.ai/api/v1`                      | OpenAI     | [キーを取得](https://openrouter.ai/keys)                          |
| **LiteLLM Proxy**   | `litellm`         | `http://localhost:4000/v1`                          | OpenAI     | LiteLLM プロキシキー                                              |
| **VLLM**            | `vllm`            | `http://localhost:8000/v1`                          | OpenAI     | ローカル                                                          |
| **Cerebras**        | `cerebras`        | `https://api.cerebras.ai/v1`                        | OpenAI     | [キーを取得](https://cerebras.ai)                                 |
| **VolcEngine (Doubao)** | `volcengine`  | `https://ark.cn-beijing.volces.com/api/v3`          | OpenAI     | [キーを取得](https://www.volcengine.com)                          |
| **神算云**          | `shengsuanyun`    | `https://router.shengsuanyun.com/api/v1`            | OpenAI     | -                                                                 |
| **BytePlus**        | `byteplus`        | `https://ark.ap-southeast.bytepluses.com/api/v3`    | OpenAI     | [キーを取得](https://www.byteplus.com)                            |
| **Vivgrid**         | `vivgrid`         | `https://api.vivgrid.com/v1`                        | OpenAI     | [キーを取得](https://vivgrid.com)                                 |
| **LongCat**         | `longcat`         | `https://api.longcat.chat/openai`                   | OpenAI     | [キーを取得](https://longcat.chat/platform)                       |
| **ModelScope (魔搭)**| `modelscope`     | `https://api-inference.modelscope.cn/v1`            | OpenAI     | [トークンを取得](https://modelscope.cn/my/tokens)                 |
| **Xiaomi MiMo**     | `mimo`            | `https://api.xiaomimimo.com/v1`                     | OpenAI     | [キーを取得](https://platform.xiaomimimo.com)                     |
| **Azure OpenAI**    | `azure`           | `https://{resource}.openai.azure.com`               | Azure      | [キーを取得](https://portal.azure.com)                            |
| **Antigravity**     | `antigravity`     | Google Cloud                                        | カスタム   | OAuth のみ                                                        |
| **GitHub Copilot**  | `github-copilot`  | `localhost:4321`                                    | gRPC       | -                                                                 |

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

#### `model_list` エントリフィールド

| フィールド | 型 | 必須 | 説明 |
|-----------|------|------|------|
| `model_name` | string | はい | agent 設定でこのモデルを参照するための一意の名前 |
| `provider` | string | いいえ | 推奨される provider 識別子。指定した場合、NeuroClaw は `model` をそのまま provider に送信します |
| `model` | string | はい | `provider` を指定した場合はネイティブなモデル ID。`provider` を省略した場合は、従来の `provider/model` 形式も引き続き使えます |
| `api_keys` | string[] | はい* | 認証キー。複数キーでリクエストごとのローテーションが可能。ローカル provider（Ollama、LM Studio、VLLM）には不要 |
| `api_base` | string | いいえ | デフォルトの API エンドポイント URL を上書き |
| `proxy` | string | いいえ | このモデルエントリの HTTP プロキシ URL |
| `user_agent` | string | いいえ | カスタム `User-Agent` リクエストヘッダー（OpenAI 互換、Gemini、Anthropic、Azure provider で対応） |
| `request_timeout` | int | いいえ | リクエストタイムアウト（秒）。デフォルト値は provider により異なる |
| `max_tokens_field` | string | いいえ | リクエストボディの max tokens フィールド名を上書き（例：o1 モデルでは `max_completion_tokens`） |
| `thinking_level` | string | いいえ | 拡張思考レベル：`off`、`low`、`medium`、`high`、`xhigh`、`adaptive` |
| `tool_schema_transform` | string | いいえ | ツールのパラメータスキーマに対する任意の互換性変換。デフォルト：無効。対応する値：`simple` |
| `extra_body` | object | いいえ | 各リクエストボディに注入する追加フィールド |
| `custom_headers` | object | いいえ | 各リクエストに注入する追加の HTTP ヘッダー（例：`{"X-Source":"coding-plan"}`）。キーが組み込みヘッダー（`Authorization`、`User-Agent`、`Content-Type`、`Accept` など）と一致する場合は、カスタム値が組み込みの値を上書きします |
| `streaming.enabled` | bool | いいえ | このモデルエントリで provider ストリーミングを試行するための opt-in。デフォルトは `false` で、アクティブな channel の `settings.streaming.enabled` も `true` である必要があります |
| `rpm` | int | いいえ | 1 分あたりのリクエストレート制限 |
| `fallbacks` | string[] | いいえ | 自動フェイルオーバーのフォールバックモデル名 |
| `enabled` | bool | いいえ | このモデルエントリを有効にするかどうか（デフォルト：`true`） |

ストリーミングを無効にする場合は `streaming` ブロックを省略してください。`"streaming": {"enabled": false}` を書くことは任意であり、生成された設定や手書きの設定には必要ありません。

`extra_body` は、OpenAI 互換の音声合成ルートで、モデル固有の TTS フィールド（カスタムの `voice` 名や `response_format: "mp3"` など）を指定する場合に特に便利です。

#### ツールスキーマの互換性

デフォルトでは、NeuroClaw はツールの JSON Schema をそのまま転送します。

一部の provider は、ツール宣言内の高度な JSON Schema 機能（`$ref`、`$defs`、`anyOf`、`oneOf`、`allOf`、`pattern`、数値や文字列の制約など）を拒否します。そうしたモデルには、モデルエントリごとに `tool_schema_transform` で互換性変換を有効にできます。

上流の provider が保守的な関数スキーマのサブセットを期待する場合は、`simple` を使います：

```json
{
  "model_name": "gemini-2.5-flash-safe-tools",
  "provider": "gemini",
  "model": "gemini-2.5-flash",
  "api_keys": ["your-gemini-key"],
  "tool_schema_transform": "simple"
}
```

注意事項：

- デフォルトは無効です。`tool_schema_transform` を省略すると、NeuroClaw は元のツールスキーマを送信します。
- この設定はモデルエントリごとなので、必要な provider だけで有効にできます。

#### Provider とモデルの解決

NeuroClaw は次のルールで `provider` とランタイムのモデル ID を解決します：

- `provider` を指定した場合、`model` はそのまま使われます。
- `provider` を省略した場合、NeuroClaw は `model` の最初の `/` までを provider とみなし、最初の `/` 以降をランタイムのモデル ID とします。

例：

| 設定 | 解決された Provider | 上流に送信されるモデル |
| --- | --- | --- |
| `"provider": "openai", "model": "gpt-5.4"` | `openai` | `gpt-5.4` |
| `"model": "openai/gpt-5.4"` | `openai` | `gpt-5.4` |
| `"provider": "openrouter", "model": "openai/gpt-5.4"` | `openrouter` | `openai/gpt-5.4` |
| `"model": "openrouter/openai/gpt-5.4"` | `openrouter` | `openai/gpt-5.4` |

#### 音声の文字起こし

`voice.model_name` で、音声の文字起こし専用のモデルを設定できます。これにより、Groq だけに頼らず、音声入力に対応した既存のマルチモーダル provider を再利用できます。

`voice.model_name` が未設定の場合、Groq の API キーがあれば、NeuroClaw は引き続き Groq の文字起こしにフォールバックします。

```json
{
  "model_list": [
    {
      "model_name": "voice-gemini",
      "provider": "gemini",
      "model": "gemini-2.5-flash",
      "api_keys": ["your-gemini-key"]
    }
  ],
  "voice": {
    "model_name": "voice-gemini",
    "echo_transcription": false
  },
  "providers": {
    "groq": {
      "api_key": "gsk_xxx"
    }
  }
}
```

#### 音声合成

`voice.tts_model_name` で、専用のテキスト読み上げ (TTS) モデルを設定できます。provider がモデル固有の TTS リクエストフィールドを必要とする場合は、`model_list[].extra_body` に記述します。

OpenRouter の `microsoft/mai-voice-2` の例：

```json
{
  "model_list": [
    {
      "model_name": "mai-voice-2",
      "provider": "openrouter",
      "model": "microsoft/mai-voice-2",
      "api_base": "https://openrouter.ai/api/v1",
      "extra_body": {
        "voice": "en-US-Harper:MAI-Voice-2",
        "response_format": "mp3"
      },
      "api_keys": ["sk-or-your-openrouter-key"]
    }
  ],
  "voice": {
    "tts_model_name": "mai-voice-2"
  }
}
```

重要な注意事項：

- この設定でも、NeuroClaw は OpenAI 互換の `/audio/speech` ルートを使用します。
- デフォルトの TTS リクエストは `voice: alloy` と `response_format: opus` を使用します。
- 選択した TTS モデルが別の値を必要とする場合は、`extra_body` でこれらのデフォルトを上書きしてください。

#### ベンダー別設定例

**OpenAI**

```json
{
  "model_name": "gpt-5.4",
  "provider": "openai",
  "model": "gpt-5.4",
  "api_keys": ["sk-..."]
}
```

**NEAR AI Cloud**

```json
{
  "model_name": "nearai-glm",
  "provider": "nearai",
  "model": "zai-org/GLM-5.1-FP8",
  "api_keys": ["your-nearai-api-key"]
}
```

**VolcEngine (Doubao)**

```json
{
  "model_name": "ark-code-latest",
  "provider": "volcengine",
  "model": "ark-code-latest",
  "api_keys": ["sk-..."]
}
```

**智谱 AI (GLM)**

```json
{
  "model_name": "glm-4.7",
  "provider": "zhipu",
  "model": "glm-4.7",
  "api_keys": ["your-key"]
}
```

**Z.AI Coding Plan (GLM)**
> Z.AI と智谱 AI は、同じ provider の 2 つのブランドです。Z.AI Coding Plan では、zhipu の設定ではなく、次のように `openai` の provider キーと api base を使用してください。
```json
{
  "model_name": "glm-4.7",
  "provider": "openai",
  "model": "glm-4.7",
  "api_keys": ["your-z.ai-key"],
  "api_base": "https://api.z.ai/api/coding/paas/v4"
}
```

**DeepSeek**

```json
{
  "model_name": "deepseek-chat",
  "provider": "deepseek",
  "model": "deepseek-chat",
  "api_keys": ["sk-..."]
}
```

**Anthropic (API キー使用)**

```json
{
  "model_name": "claude-sonnet-4.6",
  "provider": "anthropic",
  "model": "claude-sonnet-4.6",
  "api_keys": ["sk-ant-your-key"]
}
```

> `neuroclaw auth login --provider anthropic` を実行して API トークンを設定してください。

**Anthropic Messages API（ネイティブ形式）**

Anthropic API への直接アクセスや、Anthropic のネイティブメッセージ形式のみをサポートするカスタムエンドポイント向け：

```json
{
  "model_name": "claude-opus-4-6",
  "provider": "anthropic-messages",
  "model": "claude-opus-4-6",
  "api_keys": ["sk-ant-your-key"],
  "api_base": "https://api.anthropic.com"
}
```

> `anthropic-messages` プロトコルを使用するケース：
> - Anthropic のネイティブ `/v1/messages` エンドポイントのみをサポートするサードパーティプロキシを使用する場合（OpenAI 互換の `/v1/chat/completions` 非対応）
> - MiniMax、Synthetic など Anthropic のネイティブメッセージ形式を必要とするサービスに接続する場合
> - 既存の `anthropic` プロトコルが 404 エラーを返す場合（エンドポイントが OpenAI 互換形式をサポートしていないことを示す）
>
> **注意:** `anthropic` プロトコルは OpenAI 互換形式（`/v1/chat/completions`）を使用し、`anthropic-messages` は Anthropic のネイティブ形式（`/v1/messages`）を使用します。エンドポイントがサポートする形式に応じて選択してください。

**Ollama (ローカル)**

```json
{
  "model_name": "llama3",
  "provider": "ollama",
  "model": "llama3"
}
```

**LM Studio (ローカル)**

```json
{
  "model_name": "lmstudio-local",
  "provider": "lmstudio",
  "model": "openai/gpt-oss-20b"
}
```

`api_base` のデフォルトは `http://localhost:1234/v1` です。LM Studio サーバーで認証を有効にしていない限り、API キーは任意です。<br/>
`provider` を明示した場合、NeuroClaw は `openai/gpt-oss-20b` をそのまま LM Studio サーバーに送信します。`provider` を省略した場合も、従来の互換形式 `"model": "lmstudio/openai/gpt-oss-20b"` は同じ上流モデル ID に解決されます。

**カスタムプロキシ/API**

```json
{
  "model_name": "my-custom-model",
  "provider": "openai",
  "model": "custom-model",
  "api_base": "https://my-proxy.com/v1",
  "api_keys": ["sk-..."],
  "user_agent": "MyApp/1.0",
  "request_timeout": 300
}
```

**LiteLLM Proxy**

```json
{
  "model_name": "lite-gpt4",
  "provider": "litellm",
  "model": "lite-gpt4",
  "api_base": "http://localhost:4000/v1",
  "api_keys": ["sk-..."]
}
```

`provider` を明示した場合、NeuroClaw は `model` をそのまま送信します。つまり、`"provider": "litellm", "model": "lite-gpt4"` は `lite-gpt4` を送信し、`"provider": "litellm", "model": "openai/gpt-4o"` は `openai/gpt-4o` を送信します。`provider` を省略した場合も、従来の互換形式 `litellm/lite-gpt4` と `litellm/openai/gpt-4o` は同じように解決されます。

**Z.AI Coding Plan**

標準の Zhipu エンドポイント（`https://open.bigmodel.cn/api/paas/v4`）が 429（コード 1113：残高不足）を返す場合は、代わりに Z.AI Coding Plan のエンドポイントを試してください：

```json
{
  "model_name": "glm-4.7",
  "provider": "openai",
  "model": "glm-4.7",
  "api_keys": ["your-zhipu-api-key"],
  "api_base": "https://api.z.ai/api/coding/paas/v4"
}
```

**注意:** Z.AI Coding Plan のエンドポイントと標準の Zhipu エンドポイントは同じ API キー形式を使いますが、課金は別です。標準の Zhipu エンドポイントで 429 エラーが出る場合、Z.AI Coding Plan のエンドポイントには残高がある可能性があります。

#### ロードバランシング

同じモデル名に複数のエンドポイントを設定すると、NeuroClaw が自動的にラウンドロビンで分散します：

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

#### モデルの自動フェイルオーバー（カスケード）

NeuroClaw は、agent のモデル設定で `primary` と `fallbacks` を設定すると、自動フェイルオーバーを行います。
ランタイムのフォールバックチェーンは、HTTP `429`、クォータ/レート制限エラー、タイムアウトエラーなど、再試行可能な失敗の場合に次の候補を試します。
また、候補ごとにクールダウンを管理し、最近失敗した対象をすぐに再試行しないようにします。

```json
{
  "model_list": [
    {
      "model_name": "qwen-main",
      "provider": "openai",
      "model": "qwen3.5:cloud",
      "api_base": "https://api.example.com/v1",
      "api_keys": ["sk-main"]
    },
    {
      "model_name": "deepseek-backup",
      "provider": "deepseek",
      "model": "deepseek-chat",
      "api_keys": ["sk-backup-1"]
    },
    {
      "model_name": "gemini-backup",
      "provider": "gemini",
      "model": "gemini-2.5-flash",
      "api_keys": ["sk-backup-2"]
    }
  ],
  "agents": {
    "defaults": {
      "model_name": "qwen-main",
      "model_fallbacks": ["deepseek-backup", "gemini-backup"]
    }
  }
}
```

同じモデルでキー単位のフェイルオーバーを使う場合、NeuroClaw はモデル間のバックアップに移る前に、追加のキーを持つ候補を順にたどることができます。

#### レガシー `providers` 設定からの移行

旧 `providers` 設定形式は**非推奨**となり、V2 で削除されました。既存の V0/V1 設定は自動的に移行されます。

**旧設定（非推奨）：**

```json
{
  "providers": {
    "zhipu": {
      "api_key": "your-key",
      "api_base": "https://open.bigmodel.cn/api/paas/v4"
    }
  },
  "agents": {
    "defaults": {
      "provider": "zhipu",
      "model": "glm-4.7"
    }
  }
}
```

**新設定（推奨）：**

```json
{
  "version": 3,
  "model_list": [
    {
      "model_name": "glm-4.7",
      "provider": "zhipu",
      "model": "glm-4.7",
      "api_keys": ["your-key"]
    }
  ],
  "agents": {
    "defaults": {
      "model_name": "glm-4.7"
    }
  }
}
```

詳細な移行ガイドは [migration/model-list-migration.md](../migration/model-list-migration.md) を参照してください。

### Provider アーキテクチャ

NeuroClaw はプロトコルファミリーごとに Provider をルーティングします：

- OpenAI 互換プロトコル：OpenRouter、OpenAI 互換ゲートウェイ、Groq、Zhipu、vLLM スタイルのエンドポイント。
- Gemini ネイティブプロトコル：Google Gemini のネイティブ `models/*:generateContent` / `models/*:streamGenerateContent` エンドポイント。
- Anthropic プロトコル：Claude ネイティブ API 動作。
- Codex/OAuth パス：OpenAI OAuth/Token 認証ルート。

これによりランタイムを軽量に保ちつつ、新しい OpenAI 互換バックエンドの追加をほぼ設定操作（`api_base` + `api_keys`）のみで実現しています。

<details>
<summary><b>Zhipu</b></summary>

**1. API key と base URL を取得**

* [API key](https://bigmodel.cn/usercenter/proj-mgmt/apikeys) を取得

**2. 設定**

```json
{
  "agents": {
    "defaults": {
      "workspace": "~/.neuroclaw/workspace",
      "model_name": "glm-4.7",
      "max_tokens": 8192,
      "temperature": 0.7,
      "max_tool_iterations": 20
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

**3. 実行**

```bash
neuroclaw agent -m "Hello"
```

</details>

<details>
<summary><b>完全な設定例</b></summary>

```json
{
  "agents": {
    "defaults": {
      "model_name": "claude-opus-4-5"
    }
  },
  "session": {
    "dm_scope": "per-channel-peer"
  },
  "providers": {
    "openrouter": {
      "api_key": "sk-or-v1-xxx"
    },
    "groq": {
      "api_key": "gsk_xxx"
    }
  },
  "voice": {
    "model_name": "voice-gemini",
    "echo_transcription": false
  },
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      "token": "123456:ABC...",
      "allow_from": ["123456789"]
    },
    "discord": {
      "enabled": true,
      "type": "discord",
      "token": "",
      "allow_from": [""]
    },
    "whatsapp": {
      "enabled": false,
      "type": "whatsapp",
      "bridge_url": "ws://localhost:3001",
      "use_native": false,
      "session_store_path": "",
      "allow_from": []
    },
    "feishu": {
      "enabled": false,
      "type": "feishu",
      "app_id": "cli_xxx",
      "app_secret": "xxx",
      "encrypt_key": "",
      "verification_token": "",
      "allow_from": []
    },
    "qq": {
      "enabled": false,
      "type": "qq",
      "app_id": "",
      "app_secret": "",
      "allow_from": []
    }
  },
  "tools": {
    "web": {
      "brave": {
        "enabled": false,
        "api_key": "BSA...",
        "max_results": 5
      },
      "duckduckgo": {
        "enabled": true,
        "max_results": 5
      },
      "perplexity": {
        "enabled": false,
        "api_key": "",
        "max_results": 5
      },
      "searxng": {
        "enabled": false,
        "base_url": "http://localhost:8888",
        "max_results": 5
      }
    },
    "cron": {
      "exec_timeout_minutes": 5,
      "allow_command": true,
      "command_allowed_remotes": []
    }
  },
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

</details>

---

## 📝 API Key 比較表

| サービス         | Pricing                  | ユースケース                          |
| ---------------- | ------------------------ | ------------------------------------- |
| **OpenRouter**   | Free: 200K tokens/month  | マルチモデル (Claude, GPT-4 など)     |
| **Volcengine CodingPlan** | ¥9.9/first month | 中国ユーザー向け、複数の SOTA モデル (Doubao, DeepSeek など) |
| **Zhipu**        | Free: 200K tokens/month  | 中国ユーザー向け                      |
| **Brave Search** | $5/1000 queries          | Web 検索機能                          |
| **SearXNG**      | Free (self-hosted)       | プライバシー重視のメタ検索 (70+ engines) |
| **Groq**         | Free tier available      | 高速推論 (Llama, Mixtral)             |
| **Cerebras**     | Free tier available      | 高速推論 (Llama, Qwen など)           |
| **LongCat**      | Free: up to 5M tokens/day | 高速推論                             |
| **ModelScope**   | Free: 2000 requests/day  | 推論 (Qwen, GLM, DeepSeek など)       |
