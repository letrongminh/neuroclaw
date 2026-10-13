# 🔌 Provedores e Configuração de Modelos

> Voltar ao [README](../project/README.pt-br.md)

### Provedores

> [!NOTE]
> A transcrição de voz pode usar um modelo multimodal configurado em `voice.model_name`. O Groq Whisper continua disponível como fallback quando nenhum modelo de voz está configurado.

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

### Configuração de Modelos (model_list)

> O NeuroClaw prefere a configuração explícita com `provider` + `model` nativo (por exemplo `"provider": "zhipu", "model": "glm-4.7"`). A forma legada de campo único `provider/model` continua suportada por compatibilidade quando `provider` é omitido.

Para exemplos de despacho de agentes e roteamento de modelo leve, veja o [Guia de Roteamento](routing-guide.md).

Este design também permite **suporte multi-agente** com seleção flexível de provedores:

- **Agentes diferentes, provedores diferentes**: Cada agente pode usar seu próprio provedor LLM
- **Fallback de modelos**: Configure modelos primários e de fallback para resiliência
- **Balanceamento de carga**: Distribua requisições entre múltiplos endpoints
- **Configuração centralizada**: Gerencie todos os provedores em um só lugar

#### 📋 Todos os Vendors Suportados

| Vendor              | Valor de `provider`  | Default API Base                                    | Protocol  | API Key                                                          |
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

#### Configuração Básica

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

#### Campos de entrada `model_list`

| Campo | Tipo | Obrigatório | Descrição |
|-------|------|-------------|-----------|
| `model_name` | string | Sim | Nome único para referenciar este modelo na config do agent |
| `provider` | string | Não | Identificador de provedor preferido. Quando presente, o NeuroClaw envia `model` sem alteração a esse provedor |
| `model` | string | Sim | ID nativo do modelo quando `provider` está definido. Se `provider` for omitido, a forma legada `provider/model` continua suportada |
| `api_keys` | string[] | Sim* | Chave(s) API para autenticação. Múltiplas chaves permitem rotação por requisição. Não necessário para providers locais (Ollama, LM Studio, VLLM) |
| `api_base` | string | Não | Substitui a URL base da API padrão |
| `proxy` | string | Não | URL do proxy HTTP para esta entrada de modelo |
| `user_agent` | string | Não | Cabeçalho `User-Agent` personalizado enviado com requisições API (suportado por providers OpenAI-compatible, Gemini, Anthropic e Azure) |
| `request_timeout` | int | Não | Timeout de requisição em segundos (o padrão varia por provider) |
| `max_tokens_field` | string | Não | Substitui o nome do campo max tokens no corpo da requisição (ex: `max_completion_tokens` para modelos o1) |
| `thinking_level` | string | Não | Nível de pensamento estendido: `off`, `low`, `medium`, `high`, `xhigh` ou `adaptive` |
| `tool_schema_transform` | string | Não | Transformação opcional de compatibilidade para os schemas de parâmetros das ferramentas. Padrão: desativada. Valores suportados: `simple`. |
| `extra_body` | object | Não | Campos adicionais para injetar em cada corpo de requisição |
| `custom_headers` | object | Não | Cabeçalhos HTTP adicionais para injetar em cada requisição (ex: `{"X-Source":"coding-plan"}`). Se uma chave coincidir com um cabeçalho embutido, o valor personalizado substitui o embutido (ex: `Authorization`, `User-Agent`, `Content-Type`, `Accept`). |
| `streaming.enabled` | bool | Não | Opt-in para provider streaming nesta entrada de modelo. O padrão é `false` e o canal ativo também precisa de `settings.streaming.enabled` como `true` |
| `rpm` | int | Não | Limite de requisições por minuto |
| `fallbacks` | string[] | Não | Nomes dos modelos de fallback para failover automático |
| `enabled` | bool | Não | Ativar ou desativar esta entrada de modelo (padrão: `true`) |

Quando streaming estiver desativado, omita o bloco `streaming`. Escrever `"streaming": {"enabled": false}` é opcional e não é necessário em configurações geradas ou escritas à mão.

`extra_body` é especialmente útil para campos de TTS específicos de cada modelo em rotas de fala compatíveis com OpenAI, por exemplo nomes de `voice` personalizados ou `response_format: "mp3"`.

#### Compatibilidade de Schema de Ferramentas

Por padrão, o NeuroClaw encaminha os JSON Schemas das ferramentas sem alteração.

Alguns provedores rejeitam recursos avançados de JSON Schema como `$ref`, `$defs`, `anyOf`, `oneOf`, `allOf`, `pattern` ou restrições numéricas/de string dentro das declarações de ferramentas. Para esses modelos, você pode optar por uma transformação de compatibilidade por entrada de modelo com `tool_schema_transform`.

Use `simple` quando o provedor upstream espera o subconjunto conservador de schema de função:

```json
{
  "model_name": "gemini-2.5-flash-safe-tools",
  "provider": "gemini",
  "model": "gemini-2.5-flash",
  "api_keys": ["your-gemini-key"],
  "tool_schema_transform": "simple"
}
```

Notas:

- O comportamento padrão é desativado. Se você omitir `tool_schema_transform`, o NeuroClaw envia o schema original da ferramenta.
- A configuração é por entrada de modelo, então você pode ativá-la apenas para os provedores que precisam.

#### Resolução de Provider / Modelo

O NeuroClaw resolve `provider` e o ID de modelo em tempo de execução com estas regras:

- Se `provider` está definido, `model` é usado como está.
- Se `provider` é omitido, o NeuroClaw trata o primeiro segmento antes de `/` em `model` como o provider e tudo o que vem depois dessa primeira `/` como o ID do modelo em tempo de execução.

Exemplos:

| Config | Provider resolvido | Modelo enviado ao upstream |
| --- | --- | --- |
| `"provider": "openai", "model": "gpt-5.4"` | `openai` | `gpt-5.4` |
| `"model": "openai/gpt-5.4"` | `openai` | `gpt-5.4` |
| `"provider": "openrouter", "model": "openai/gpt-5.4"` | `openrouter` | `openai/gpt-5.4` |
| `"model": "openrouter/openai/gpt-5.4"` | `openrouter` | `openai/gpt-5.4` |

#### Transcrição de Voz

Você pode configurar um modelo dedicado à transcrição de áudio com `voice.model_name`. Isso permite reutilizar provedores multimodais existentes que aceitam entrada de áudio, em vez de depender apenas do Groq.

Se `voice.model_name` não estiver configurado, o NeuroClaw continua usando a transcrição do Groq como fallback quando há uma chave de API do Groq disponível.

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

#### Síntese de Voz

Você pode configurar um modelo dedicado de text-to-speech com `voice.tts_model_name`. Quando o provedor precisa de campos de requisição de TTS específicos do modelo, coloque-os em `model_list[].extra_body`.

Exemplo com o OpenRouter `microsoft/mai-voice-2`:

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

Pontos importantes:

- O NeuroClaw ainda usa a rota `/audio/speech` compatível com OpenAI nesta configuração.
- A requisição de TTS padrão usa `voice: alloy` e `response_format: opus`.
- Substitua esses padrões com `extra_body` quando o modelo de TTS escolhido exigir outros valores.

#### Exemplos por Vendor

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
> Z.AI e 智谱 AI são duas marcas do mesmo provedor. Para o Z.AI Coding Plan, use o `provider` `openai` e a api base abaixo, em vez da config do zhipu
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

**Anthropic (com chave de API)**

```json
{
  "model_name": "claude-sonnet-4.6",
  "provider": "anthropic",
  "model": "claude-sonnet-4.6",
  "api_keys": ["sk-ant-your-key"]
}
```

> Execute `neuroclaw auth login --provider anthropic` para colar seu token de API.

**Anthropic Messages API (formato nativo)**

Para acesso direto à API Anthropic ou endpoints personalizados que suportam apenas o formato de mensagem nativo da Anthropic:

```json
{
  "model_name": "claude-opus-4-6",
  "provider": "anthropic-messages",
  "model": "claude-opus-4-6",
  "api_keys": ["sk-ant-your-key"],
  "api_base": "https://api.anthropic.com"
}
```

> Use o protocolo `anthropic-messages` quando:
> - Usar proxies de terceiros que suportam apenas o endpoint nativo `/v1/messages` da Anthropic (não o compatível com OpenAI `/v1/chat/completions`)
> - Conectar a serviços como MiniMax, Synthetic que requerem o formato de mensagem nativo da Anthropic
> - O protocolo `anthropic` existente retorna erros 404 (indicando que o endpoint não suporta formato compatível com OpenAI)
>
> **Nota:** O protocolo `anthropic` usa formato compatível com OpenAI (`/v1/chat/completions`), enquanto `anthropic-messages` usa o formato nativo da Anthropic (`/v1/messages`). Escolha com base no formato suportado pelo seu endpoint.

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

`api_base` tem como padrão `http://localhost:1234/v1`. A chave de API é opcional, a menos que seu servidor LM Studio habilite autenticação.<br/>
Com `provider` explícito, o NeuroClaw envia `openai/gpt-oss-20b` sem alteração ao servidor LM Studio. A forma legada de compatibilidade `"model": "lmstudio/openai/gpt-oss-20b"` ainda resolve para o mesmo ID de modelo upstream quando `provider` é omitido.

**Proxy/API Personalizado**

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

Com `provider` explícito, o NeuroClaw envia `model` sem alteração. Ou seja, `"provider": "litellm", "model": "lite-gpt4"` envia `lite-gpt4`, enquanto `"provider": "litellm", "model": "openai/gpt-4o"` envia `openai/gpt-4o`. As formas legadas de compatibilidade `litellm/lite-gpt4` e `litellm/openai/gpt-4o` continuam resolvendo da mesma forma quando `provider` é omitido.

**Z.AI Coding Plan**

Se o endpoint padrão da Zhipu (`https://open.bigmodel.cn/api/paas/v4`) retornar 429 (código 1113: saldo insuficiente), tente usar o endpoint do Z.AI Coding Plan:

```json
{
  "model_name": "glm-4.7",
  "provider": "openai",
  "model": "glm-4.7",
  "api_keys": ["your-zhipu-api-key"],
  "api_base": "https://api.z.ai/api/coding/paas/v4"
}
```

**Nota:** O endpoint do Z.AI Coding Plan e o endpoint padrão da Zhipu usam o mesmo formato de chave de API, mas têm cobrança separada. Se você receber erros 429 no endpoint padrão da Zhipu, o endpoint do Z.AI Coding Plan pode ter saldo disponível.

#### Balanceamento de Carga

Configure múltiplos endpoints para o mesmo nome de modelo — o NeuroClaw fará automaticamente round-robin entre eles:

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

#### Failover Automático de Modelos (Cascata)

O NeuroClaw já suporta failover automático quando você configura `primary` + `fallbacks` nas configurações de modelo do agente. A cadeia de fallback em tempo de execução tenta o próximo candidato em falhas recuperáveis, como HTTP `429`, erros de cota/limite de taxa e erros de timeout. Ela também aplica um controle de cooldown por candidato para evitar tentar de novo imediatamente um alvo que acabou de falhar.

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

Se você usa failover por chave para o mesmo modelo, o NeuroClaw pode encadear candidatos adicionais baseados em chaves antes de passar para os backups entre modelos.

#### Migração da Configuração Legacy `providers`

A configuração antiga `providers` está **descontinuada** e foi removida no V2. Configs V0/V1 existentes são auto-migradas.

**Configuração Antiga (descontinuada):**

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

**Configuração Nova (recomendada):**

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

Para guia de migração detalhado, veja [migration/model-list-migration.md](../migration/model-list-migration.md).

### Arquitetura de Provedores

O NeuroClaw roteia provedores por família de protocolo:

- Protocolo compatível com OpenAI: OpenRouter, gateways compatíveis com OpenAI, Groq, Zhipu e endpoints estilo vLLM.
- Protocolo Gemini nativo: Google Gemini via endpoints nativos `models/*:generateContent` e `models/*:streamGenerateContent`.
- Protocolo Anthropic: Comportamento nativo da API Claude.
- Caminho Codex/OAuth: Rota de autenticação OAuth/token da OpenAI.

Isso mantém o runtime leve enquanto torna novos backends compatíveis com OpenAI basicamente uma operação de configuração (`api_base` + `api_keys`).

<details>
<summary><b>Zhipu</b></summary>

**1. Obter chave de API e URL base**

* Obtenha a [chave de API](https://bigmodel.cn/usercenter/proj-mgmt/apikeys)

**2. Configurar**

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

**3. Executar**

```bash
neuroclaw agent -m "Hello"
```

</details>

<details>
<summary><b>Exemplo de configuração completa</b></summary>

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

## 📝 Comparação de Chaves de API

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
