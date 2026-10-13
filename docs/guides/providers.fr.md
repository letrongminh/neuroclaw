# 🔌 Fournisseurs et Configuration des Modèles

> Retour au [README](../project/README.fr.md)

### Fournisseurs

> [!NOTE]
> La transcription vocale peut utiliser un modèle multimodal configuré via `voice.model_name`. Groq Whisper reste disponible en repli lorsqu'aucun modèle vocal n'est configuré.

| Provider     | Purpose                                 | Get API Key                                                  |
| ------------ | --------------------------------------- | ------------------------------------------------------------ |
| `gemini`     | LLM (Gemini direct)                     | [aistudio.google.com](https://aistudio.google.com)           |
| `zhipu`      | LLM (Zhipu direct)                      | [bigmodel.cn](https://bigmodel.cn)                           |
| `zai`        | LLM (Z.AI Coding Plan)                  | [z.ai](https://z.ai/manage-apikey/apikey-list)           |
| `volcengine` | LLM(Volcengine direct)                  | [volcengine.com](https://www.volcengine.com)                 |
| `openrouter` | LLM (recommended, access to all models) | [openrouter.ai](https://openrouter.ai)                       |
| `anthropic`  | LLM (Claude direct)                     | [console.anthropic.com](https://console.anthropic.com)       |
| `openai`     | LLM (GPT direct)                        | [platform.openai.com](https://platform.openai.com)           |
| `venice`     | LLM (Venice AI direct)                  | [venice.ai](https://venice.ai)                               |
| `nearai`     | LLM (NEAR AI Cloud TEE inference)       | [near.ai](https://near.ai)                                   |
| `deepseek`   | LLM (DeepSeek direct)                   | [platform.deepseek.com](https://platform.deepseek.com)       |
| `qwen`       | LLM (Qwen direct)                       | [dashscope.console.aliyun.com](https://dashscope.console.aliyun.com) |
| `groq`       | LLM + **Voice transcription** (Whisper) | [console.groq.com](https://console.groq.com)                 |
| `cerebras`   | LLM (Cerebras direct)                   | [cerebras.ai](https://cerebras.ai)                           |
| `vivgrid`    | LLM (Vivgrid direct)                    | [vivgrid.com](https://vivgrid.com)                           |
| `nvidia`     | LLM (NVIDIA NIM)                        | [build.nvidia.com](https://build.nvidia.com)                 |
| `moonshot`   | LLM (Kimi/Moonshot direct)              | [platform.moonshot.cn](https://platform.moonshot.cn)         |
| `minimax`    | LLM (Minimax direct)                    | [platform.minimaxi.com](https://platform.minimaxi.com)      |
| `avian`      | LLM (Avian direct)                      | [avian.io](https://avian.io)                                 |
| `mistral`    | LLM (Mistral direct)                    | [console.mistral.ai](https://console.mistral.ai)            |
| `longcat`    | LLM (Longcat direct)                    | [longcat.ai](https://longcat.ai)                             |
| `modelscope` | LLM (ModelScope direct)                 | [modelscope.cn](https://modelscope.cn)                       |
| `mimo`       | LLM (Xiaomi MiMo direct)                | [platform.xiaomimimo.com](https://platform.xiaomimimo.com)   |

### Configuration des Modèles (model_list)

> NeuroClaw privilégie une configuration explicite `provider` + `model` natif (par exemple `"provider": "zhipu", "model": "glm-4.7"`). L'ancienne forme à champ unique `provider/model` reste prise en charge pour la compatibilité lorsque `provider` est omis.

Pour des exemples de répartition entre agents et de routage vers un modèle léger, voir le [Guide de routage](routing-guide.md).

Cette conception permet également le **support multi-agents** avec une sélection flexible de fournisseurs :

- **Différents agents, différents fournisseurs** : Chaque agent peut utiliser son propre fournisseur LLM
- **Modèles de repli** : Configurez des modèles principaux et de repli pour la résilience
- **Répartition de charge** : Distribuez les requêtes entre plusieurs endpoints
- **Configuration centralisée** : Gérez tous les fournisseurs en un seul endroit

#### 📋 Tous les Vendors Supportés

| Vendor              | Valeur `provider` | Default API Base                                    | Protocol  | API Key                                                          |
| ------------------- | ----------------- |-----------------------------------------------------| --------- | ---------------------------------------------------------------- |
| **OpenAI**          | `openai`          | `https://api.openai.com/v1`                         | OpenAI    | [Get Key](https://platform.openai.com)                           |
| **Venice AI**       | `venice`          | `https://api.venice.ai/api/v1`                      | OpenAI    | [Get Key](https://venice.ai)                                     |
| **NEAR AI Cloud**   | `nearai`          | `https://cloud-api.near.ai/v1`                      | OpenAI    | [Get Key](https://near.ai)                                       |
| **Anthropic**       | `anthropic`       | `https://api.anthropic.com/v1`                      | Anthropic | [Get Key](https://console.anthropic.com)                         |
| **智谱 AI (GLM)**   | `zhipu`           | `https://open.bigmodel.cn/api/paas/v4`              | OpenAI    | [Get Key](https://open.bigmodel.cn/usercenter/proj-mgmt/apikeys) |
| **Z.AI Coding Plan** | `openai`         | `https://api.z.ai/api/coding/paas/v4`               | OpenAI    | [Get Key](https://z.ai/manage-apikey/apikey-list)                |
| **DeepSeek**        | `deepseek`        | `https://api.deepseek.com/v1`                       | OpenAI    | [Get Key](https://platform.deepseek.com)                         |
| **Google Gemini**   | `gemini`          | `https://generativelanguage.googleapis.com/v1beta`  | Gemini    | [Get Key](https://aistudio.google.com/api-keys)                  |
| **Groq**            | `groq`            | `https://api.groq.com/openai/v1`                    | OpenAI    | [Get Key](https://console.groq.com)                              |
| **Moonshot**        | `moonshot`        | `https://api.moonshot.cn/v1`                        | OpenAI    | [Get Key](https://platform.moonshot.cn)                          |
| **通义千问 (Qwen)** | `qwen`            | `https://dashscope.aliyuncs.com/compatible-mode/v1` | OpenAI    | [Get Key](https://dashscope.console.aliyun.com)                  |
| **NVIDIA**          | `nvidia`          | `https://integrate.api.nvidia.com/v1`               | OpenAI    | [Get Key](https://build.nvidia.com)                              |
| **Ollama**          | `ollama`          | `http://localhost:11434/v1`                         | OpenAI    | Local (no key needed)                                            |
| **LM Studio**       | `lmstudio`        | `http://localhost:1234/v1`                          | OpenAI    | Optional (local default: no key)                                 |
| **OpenRouter**      | `openrouter`      | `https://openrouter.ai/api/v1`                      | OpenAI    | [Get Key](https://openrouter.ai/keys)                            |
| **LiteLLM Proxy**   | `litellm`         | `http://localhost:4000/v1`                          | OpenAI    | Your LiteLLM proxy key                                           |
| **VLLM**            | `vllm`            | `http://localhost:8000/v1`                          | OpenAI    | Local                                                            |
| **Cerebras**        | `cerebras`        | `https://api.cerebras.ai/v1`                        | OpenAI    | [Get Key](https://cerebras.ai)                                   |
| **VolcEngine (Doubao)** | `volcengine`  | `https://ark.cn-beijing.volces.com/api/v3`          | OpenAI    | [Get Key](https://www.volcengine.com) |
| **神算云**          | `shengsuanyun`    | `https://router.shengsuanyun.com/api/v1`            | OpenAI    | -                                                                |
| **BytePlus**        | `byteplus`        | `https://ark.ap-southeast.bytepluses.com/api/v3`    | OpenAI    | [Get Key](https://www.byteplus.com)                              |
| **Vivgrid**         | `vivgrid`         | `https://api.vivgrid.com/v1`                        | OpenAI    | [Get Key](https://vivgrid.com)                                   |
| **LongCat**         | `longcat`         | `https://api.longcat.chat/openai`                   | OpenAI    | [Get Key](https://longcat.chat/platform)                         |
| **ModelScope (魔搭)**| `modelscope`     | `https://api-inference.modelscope.cn/v1`            | OpenAI    | [Get Token](https://modelscope.cn/my/tokens)                     |
| **Xiaomi MiMo**     | `mimo`            | `https://api.xiaomimimo.com/v1`                     | OpenAI    | [Get Key](https://platform.xiaomimimo.com)                       |
| **Azure OpenAI**    | `azure`           | `https://{resource}.openai.azure.com`               | Azure     | [Get Key](https://portal.azure.com)                              |
| **Antigravity**     | `antigravity`     | Google Cloud                                        | Custom    | OAuth only                                                       |
| **GitHub Copilot**  | `github-copilot`  | `localhost:4321`                                    | gRPC      | -                                                                |

#### Configuration de Base

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

#### Champs d'entrée `model_list`

| Champ | Type | Requis | Description |
|-------|------|--------|-------------|
| `model_name` | string | Oui | Nom unique pour référencer ce modèle dans la config agent |
| `provider` | string | Non | Identifiant de fournisseur préféré. Lorsqu'il est présent, NeuroClaw envoie `model` tel quel à ce fournisseur |
| `model` | string | Oui | Identifiant natif du modèle lorsque `provider` est défini. Si `provider` est omis, l'ancienne forme `provider/model` reste prise en charge |
| `api_keys` | string[] | Oui* | Clé(s) API pour l'authentification. Plusieurs clés permettent la rotation par requête. Non requis pour les fournisseurs locaux (Ollama, LM Studio, VLLM) |
| `api_base` | string | Non | Remplace l'URL de base API par défaut |
| `proxy` | string | Non | URL du proxy HTTP pour cette entrée de modèle |
| `user_agent` | string | Non | En-tête `User-Agent` personnalisé pour les requêtes API (supporté par les providers compatibles OpenAI, Gemini, Anthropic et Azure) |
| `request_timeout` | int | Non | Délai d'expiration de la requête en secondes (la valeur par défaut varie selon le provider) |
| `max_tokens_field` | string | Non | Remplace le nom du champ max tokens dans le corps de la requête (ex : `max_completion_tokens` pour les modèles o1) |
| `thinking_level` | string | Non | Niveau de pensée étendue : `off`, `low`, `medium`, `high`, `xhigh` ou `adaptive` |
| `tool_schema_transform` | string | Non | Transformation de compatibilité facultative pour les schémas de paramètres d'outils. Par défaut : désactivée. Valeurs prises en charge : `simple`. |
| `extra_body` | object | Non | Champs supplémentaires à injecter dans chaque corps de requête |
| `custom_headers` | object | Non | En-têtes HTTP supplémentaires à injecter dans chaque requête (ex : `{"X-Source":"coding-plan"}`). Si une clé correspond à un en-tête intégré, la valeur personnalisée le remplace (ex : `Authorization`, `User-Agent`, `Content-Type`, `Accept`). |
| `streaming.enabled` | bool | Non | Opt-in pour le streaming provider sur cette entrée de modèle. Par défaut `false`, et le channel actif doit aussi avoir `settings.streaming.enabled` à `true` |
| `rpm` | int | Non | Limite de requêtes par minute |
| `fallbacks` | string[] | Non | Noms des modèles de secours pour le basculement automatique |
| `enabled` | bool | Non | Activer ou désactiver cette entrée de modèle (par défaut : `true`) |

Lorsque le streaming est désactivé, omettez le bloc `streaming`. Écrire `"streaming": {"enabled": false}` est optionnel et n'est pas nécessaire.

`extra_body` est particulièrement utile pour les champs TTS propres à un modèle sur les routes
de synthèse vocale compatibles OpenAI, par exemple des noms de `voice` personnalisés ou `response_format: "mp3"`.

#### Compatibilité des schémas d'outils

Par défaut, NeuroClaw transmet les schémas JSON des outils sans modification.

Certains fournisseurs rejettent les fonctionnalités avancées de JSON Schema telles que `$ref`, `$defs`, `anyOf`, `oneOf`, `allOf`, `pattern` ou les contraintes numériques/chaînes dans les déclarations d'outils. Pour ces modèles, vous pouvez activer une transformation de compatibilité par entrée de modèle avec `tool_schema_transform`.

Utilisez `simple` lorsque le fournisseur en amont attend le sous-ensemble prudent des schémas de fonctions :

```json
{
  "model_name": "gemini-2.5-flash-safe-tools",
  "provider": "gemini",
  "model": "gemini-2.5-flash",
  "api_keys": ["your-gemini-key"],
  "tool_schema_transform": "simple"
}
```

Notes :

- Désactivé par défaut. Si vous omettez `tool_schema_transform`, NeuroClaw envoie le schéma d'outil d'origine.
- Le réglage s'applique par entrée de modèle : vous ne l'activez que pour les fournisseurs qui en ont besoin.

#### Résolution du fournisseur et du modèle

NeuroClaw détermine `provider` et l'identifiant de modèle utilisé à l'exécution selon ces règles :

- Si `provider` est défini, `model` est utilisé tel quel.
- Si `provider` est omis, NeuroClaw traite le premier segment avant `/` dans `model` comme le fournisseur, et tout ce qui suit ce premier `/` comme l'identifiant de modèle d'exécution.

Exemples :

| Config | Fournisseur résolu | Modèle envoyé en amont |
| --- | --- | --- |
| `"provider": "openai", "model": "gpt-5.4"` | `openai` | `gpt-5.4` |
| `"model": "openai/gpt-5.4"` | `openai` | `gpt-5.4` |
| `"provider": "openrouter", "model": "openai/gpt-5.4"` | `openrouter` | `openai/gpt-5.4` |
| `"model": "openrouter/openai/gpt-5.4"` | `openrouter` | `openai/gpt-5.4` |

#### Transcription vocale

Vous pouvez configurer un modèle dédié à la transcription audio avec `voice.model_name`. Cela permet de réutiliser des fournisseurs multimodaux existants qui acceptent l'entrée audio, au lieu de dépendre uniquement de Groq.

Si `voice.model_name` n'est pas configuré, NeuroClaw continue de se replier sur la transcription Groq lorsqu'une clé API Groq est disponible.

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

#### Synthèse vocale

Vous pouvez configurer un modèle de synthèse vocale dédié avec `voice.tts_model_name`.
Lorsque le fournisseur exige des champs de requête TTS propres au modèle, placez-les dans
`model_list[].extra_body`.

Exemple avec OpenRouter `microsoft/mai-voice-2` :

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

Points importants :

- NeuroClaw utilise toujours la route `/audio/speech` compatible OpenAI pour cette configuration.
- La requête TTS par défaut utilise `voice: alloy` et `response_format: opus`.
- Remplacez ces valeurs par défaut via `extra_body` lorsque le modèle TTS choisi exige d'autres valeurs.

#### Exemples par Vendor

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
> Z.AI et 智谱 AI sont deux marques du même fournisseur. Pour le Z.AI Coding Plan, utilisez la clé de modèle `openai` et l'api base ci-dessous, plutôt que la configuration zhipu
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

**Anthropic (avec clé API)**

```json
{
  "model_name": "claude-sonnet-4.6",
  "provider": "anthropic",
  "model": "claude-sonnet-4.6",
  "api_keys": ["sk-ant-your-key"]
}
```

> Exécutez `neuroclaw auth login --provider anthropic` pour coller votre token API.

**API Anthropic Messages (format natif)**

Pour un accès direct à l'API Anthropic ou des endpoints personnalisés qui ne prennent en charge que le format de message natif d'Anthropic :

```json
{
  "model_name": "claude-opus-4-6",
  "provider": "anthropic-messages",
  "model": "claude-opus-4-6",
  "api_keys": ["sk-ant-your-key"],
  "api_base": "https://api.anthropic.com"
}
```

> Utilisez le protocole `anthropic-messages` lorsque :
> - Vous utilisez des proxys tiers qui ne prennent en charge que l'endpoint natif `/v1/messages` d'Anthropic (pas le format compatible OpenAI `/v1/chat/completions`)
> - Vous vous connectez à des services comme MiniMax, Synthetic qui nécessitent le format de message natif d'Anthropic
> - Le protocole `anthropic` existant renvoie des erreurs 404 (indiquant que l'endpoint ne prend pas en charge le format compatible OpenAI)
>
> **Note :** Le protocole `anthropic` utilise le format compatible OpenAI (`/v1/chat/completions`), tandis que `anthropic-messages` utilise le format natif d'Anthropic (`/v1/messages`). Choisissez en fonction du format pris en charge par votre endpoint.

**Ollama (local)**

```json
{
  "model_name": "llama3",
  "provider": "ollama",
  "model": "llama3"
}
```

**LM Studio (local)**

```json
{
  "model_name": "lmstudio-local",
  "provider": "lmstudio",
  "model": "openai/gpt-oss-20b"
}
```

`api_base` vaut par défaut `http://localhost:1234/v1`. La clé API est optionnelle, sauf si votre serveur LM Studio active l'authentification.<br/>
Avec un `provider` explicite, NeuroClaw envoie `openai/gpt-oss-20b` tel quel au serveur LM Studio. L'ancienne forme de compatibilité `"model": "lmstudio/openai/gpt-oss-20b"` donne toujours le même identifiant de modèle en amont lorsque `provider` est omis.

**Proxy/API Personnalisé**

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

Avec un `provider` explicite, NeuroClaw envoie `model` tel quel. Ainsi `"provider": "litellm", "model": "lite-gpt4"` envoie `lite-gpt4`, tandis que `"provider": "litellm", "model": "openai/gpt-4o"` envoie `openai/gpt-4o`. Les anciennes formes de compatibilité `litellm/lite-gpt4` et `litellm/openai/gpt-4o` se résolvent de la même façon lorsque `provider` est omis.

**Z.AI Coding Plan**

Si l'endpoint Zhipu standard (`https://open.bigmodel.cn/api/paas/v4`) renvoie 429 (code 1113 : solde insuffisant), essayez plutôt l'endpoint Z.AI Coding Plan :

```json
{
  "model_name": "glm-4.7",
  "provider": "openai",
  "model": "glm-4.7",
  "api_keys": ["your-zhipu-api-key"],
  "api_base": "https://api.z.ai/api/coding/paas/v4"
}
```

**Note :** L'endpoint Z.AI Coding Plan et l'endpoint Zhipu standard utilisent le même format de clé API mais ont une facturation séparée. Si vous rencontrez des erreurs 429 avec l'endpoint Zhipu standard, l'endpoint Z.AI Coding Plan peut disposer d'un solde.

#### Répartition de Charge

Configurez plusieurs endpoints pour le même nom de modèle — NeuroClaw effectuera automatiquement un round-robin entre eux :

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

#### Basculement automatique de modèle (cascade)

NeuroClaw prend déjà en charge le basculement automatique lorsque vous configurez `primary` + `fallbacks` dans les paramètres de modèle de l'agent.
La chaîne de repli à l'exécution essaie le candidat suivant pour les échecs réessayables, tels que HTTP `429`, les erreurs de quota/limite de débit et les délais d'expiration.
Elle applique aussi un suivi de période de refroidissement par candidat afin d'éviter de réessayer immédiatement une cible qui vient d'échouer.

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

Si vous utilisez le basculement au niveau des clés pour un même modèle, NeuroClaw peut parcourir les candidats supplémentaires adossés à d'autres clés avant de passer aux modèles de secours.

#### Migration depuis l'Ancienne Configuration `providers`

L'ancienne configuration `providers` est **dépréciée** et a été supprimée dans V2. Les configs V0/V1 existantes sont auto-migrées.

**Ancienne configuration (dépréciée) :**

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

**Nouvelle configuration (recommandée) :**

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

Pour un guide de migration détaillé, voir [migration/model-list-migration.md](../migration/model-list-migration.md).

### Architecture des Fournisseurs

NeuroClaw route les fournisseurs par famille de protocoles :

- Protocole compatible OpenAI : OpenRouter, passerelles compatibles OpenAI, Groq, Zhipu et endpoints de type vLLM.
- Protocole Gemini natif : Google Gemini via les endpoints natifs `models/*:generateContent` et `models/*:streamGenerateContent`.
- Protocole Anthropic : Comportement natif de l'API Claude.
- Chemin Codex/OAuth : Route d'authentification OAuth/token OpenAI.

Cela maintient le runtime léger tout en faisant des nouveaux backends compatibles OpenAI principalement une opération de configuration (`api_base` + `api_keys`).

<details>
<summary><b>Zhipu</b></summary>

**1. Obtenir la clé API et l'URL de base**

* Obtenir la [clé API](https://bigmodel.cn/usercenter/proj-mgmt/apikeys)

**2. Configurer**

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

**3. Lancer**

```bash
neuroclaw agent -m "Hello"
```

</details>

<details>
<summary><b>Exemple de configuration complète</b></summary>

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

## 📝 Comparaison des Clés API

| Service          | Pricing                  | Use Case                              |
| ---------------- | ------------------------ | ------------------------------------- |
| **OpenRouter**   | Free: 200K tokens/month  | Multiple models (Claude, GPT-4, etc.) |
| **Volcengine CodingPlan** | ¥9.9/first month | Best for Chinese users, multiple SOTA models (Doubao, DeepSeek, etc.) |
| **Zhipu**        | Free: 200K tokens/month  | Suitable for Chinese users                |
| **Brave Search** | $5/1000 queries          | Web search functionality              |
| **SearXNG**      | Free (self-hosted)       | Privacy-focused metasearch (70+ engines) |
| **Groq**         | Free tier available      | Fast inference (Llama, Mixtral)       |
| **Cerebras**     | Free tier available      | Fast inference (Llama, Qwen, etc.)    |
| **LongCat**      | Free: up to 5M tokens/day | Fast inference                       |
| **ModelScope**   | Free: 2000 requests/day  | Inference (Qwen, GLM, DeepSeek, etc.) |
