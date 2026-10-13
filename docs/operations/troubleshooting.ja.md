# 🐛 トラブルシューティング

> [README](../project/README.ja.md) に戻る

## "model ... not found in model_list" または OpenRouter "free is not a valid model ID"

**症状：** 以下のいずれかのエラーが表示されます：

- `Error creating provider: model "openrouter/free" not found in model_list`
- OpenRouter が 400 を返す：`"free is not a valid model ID"`

**原因：** NeuroClaw は provider/model を 2 段階で解決します：

- `provider` が設定されている場合、`model` フィールドはそのまま当該プロバイダーに送信されます。
- `provider` が省略された場合、NeuroClaw は最初の `/` までの部分からプロバイダーを推測し、最初の `/` 以降をすべてランタイムのモデル ID として送信します。

OpenRouter の無料枠ルーティングでは、`provider` を明示する設定が推奨されます。

- **誤り：** `"model": "free"` → OpenRouter プロバイダーが選択されないため、`free` は有効な OpenRouter のモデルルートになりません。
- **正しい：** `"provider": "openrouter", "model": "free"` → OpenRouter は `free` を受け取ります。
- **これも可：** `"model": "openrouter/free"` → プロバイダーは `openrouter`、ランタイムのモデル ID は `free` と解決されます。

**修正方法：** `~/.neuroclaw/config.json`（またはお使いの設定パス）で：

1. **agents.defaults.model_name** は `model_list` 内の `model_name` と一致する必要があります（例：`"openrouter-free"`）。
2. そのエントリでは **provider** を `openrouter` に設定することを推奨し、**model** は有効な OpenRouter モデル ID にします。例：
   - `"free"` – 自動無料枠
   - `"google/gemini-2.0-flash-exp:free"`
   - `"meta-llama/llama-3.1-8b-instruct:free"`

設定例：

```json
{
  "agents": {
    "defaults": {
      "model_name": "openrouter-free"
    }
  },
  "model_list": [
    {
      "model_name": "openrouter-free",
      "provider": "openrouter",
      "model": "free",
      "api_keys": ["sk-or-v1-YOUR_OPENROUTER_KEY"],
      "api_base": "https://openrouter.ai/api/v1"
    }
  ]
}
```

キーは [OpenRouter Keys](https://openrouter.ai/keys) で取得できます。
