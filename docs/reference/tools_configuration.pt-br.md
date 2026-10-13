# 🔧 Configuração de Ferramentas

> Voltar ao [README](../project/README.pt-br.md)

A configuração de ferramentas do NeuroClaw está localizada no campo `tools` do `config.json`.

## Estrutura de diretórios

```json
{
  "tools": {
    "web": {
      ...
    },
    "mcp": {
      ...
    },
    "exec": {
      ...
    },
    "cron": {
      ...
    },
    "skills": {
      ...
    }
  }
}
```

## Filtragem de dados sensíveis

Antes de os resultados das ferramentas serem enviados ao LLM, o NeuroClaw pode filtrar valores sensíveis (chaves de API, tokens, segredos) da saída. Isso impede que o LLM veja as próprias credenciais.

Veja [Sensitive Data Filtering](../security/sensitive_data_filtering.md) para a documentação completa.

| Config | Tipo | Padrão | Descrição |
|--------|------|--------|-----------|
| `filter_sensitive_data` | bool | `true` | Habilitar/desabilitar a filtragem |
| `filter_min_length` | int | `8` | Comprimento mínimo do conteúdo para acionar a filtragem |

## Ferramentas Web

As ferramentas web são usadas para pesquisa e busca de páginas web.

### Web Fetcher
Configurações gerais para busca e processamento de conteúdo de páginas web.

| Config              | Tipo   | Padrão        | Descrição                                                                                     |
|---------------------|--------|---------------|-----------------------------------------------------------------------------------------------|
| `enabled`           | bool   | true          | Habilitar a capacidade de busca de páginas web.                                               |
| `fetch_limit_bytes` | int    | 10485760      | Tamanho máximo do payload da página web a ser buscado, em bytes (padrão é 10MB).              |
| `format`            | string | "plaintext"   | Formato de saída do conteúdo buscado. Opções: `plaintext` ou `markdown` (recomendado).        |

### DuckDuckGo

| Config        | Tipo | Padrão | Descrição                      |
|---------------|------|--------|--------------------------------|
| `enabled`     | bool | false  | Habilitar pesquisa DuckDuckGo  |
| `max_results` | int  | 5      | Número máximo de resultados    |

### Sogou

| Config        | Tipo | Padrão | Descrição                      |
|---------------|------|--------|--------------------------------|
| `enabled`     | bool | true   | Habilitar pesquisa Sogou       |
| `max_results` | int  | 5      | Número máximo de resultados    |

### Gemini Google Search

A pesquisa Gemini usa o Gemini com grounding do Google Search. Ela retorna uma resposta sintetizada por IA, com citações do Google Search.

| Config        | Tipo   | Padrão               | Descrição                         |
|---------------|--------|----------------------|-----------------------------------|
| `enabled`     | bool   | false                | Habilitar Gemini Google Search    |
| `api_key`     | string | -                    | Chave API do Google Gemini        |
| `model`       | string | `gemini-2.5-flash`   | Modelo Gemini usado na pesquisa   |
| `max_results` | int    | 5                    | Número máximo de citações         |

```json
{
  "tools": {
    "web": {
      "gemini": {
        "enabled": true,
        "api_key": "YOUR_GEMINI_API_KEY",
        "model": "gemini-2.5-flash",
        "max_results": 5
      }
    }
  }
}
```

### Baidu Search

| Config        | Tipo   | Padrão                                                          | Descrição                          |
|---------------|--------|-----------------------------------------------------------------|------------------------------------|
| `enabled`     | bool   | false                                                           | Habilitar pesquisa Baidu           |
| `api_key`     | string | -                                                               | Chave API Qianfan                  |
| `base_url`    | string | `https://qianfan.baidubce.com/v2/ai_search/web_search`         | URL da API Baidu Search            |
| `max_results` | int    | 10                                                              | Número máximo de resultados        |

```json
{
  "tools": {
    "web": {
      "baidu_search": {
        "enabled": true,
        "api_key": "YOUR_BAIDU_QIANFAN_API_KEY",
        "max_results": 10
      }
    }
  }
}
```

### Perplexity

| Config        | Tipo   | Padrão | Descrição                      |
|---------------|--------|--------|--------------------------------|
| `enabled`     | bool     | false  | Habilitar pesquisa Perplexity                                    |
| `api_key`     | string   | -      | Chave API do Perplexity                                          |
| `api_keys`    | string[] | -      | Várias chaves API do Perplexity para rotação (prioridade sobre `api_key`) |
| `max_results` | int      | 5      | Número máximo de resultados                                      |

### Kagi Search

O Kagi Search usa o cliente OpenAPI oficial do Kagi para `POST /search` e retorna resultados web normais de `data.search`.

| Config        | Tipo     | Padrão                                | Descrição                                      |
|---------------|----------|---------------------------------------|------------------------------------------------|
| `enabled`     | bool     | false                                 | Habilitar Kagi Search                          |
| `api_key`     | string   | -                                     | Chave API do Kagi                              |
| `api_keys`    | string[] | -                                     | Várias chaves API para rotação (prioridade sobre `api_key`) |
| `base_url`    | string   | `https://kagi.com/api/v1/search`      | Endpoint da API do Kagi Search                 |
| `max_results` | int      | 5                                     | Número máximo de resultados                    |

```json
{
  "tools": {
    "web": {
      "provider": "kagi",
      "kagi": {
        "enabled": true,
        "max_results": 5,
        "base_url": "https://kagi.com/api/v1/search"
      }
    }
  }
}
```

Guarde as chaves API do Kagi em `.security.yml`:

```yaml
web:
  kagi:
    api_keys:
      - "YOUR_KAGI_API_KEY"
```

O uso da API do Kagi pode ser cobrado ou limitado separadamente de uma assinatura Kagi normal, dependendo da sua conta e da configuração da API.

### Brave

| Config        | Tipo   | Padrão | Descrição                  |
|---------------|--------|--------|----------------------------|
| `enabled`     | bool     | false  | Habilitar pesquisa Brave                                         |
| `api_key`     | string   | -      | Chave API única do Brave Search                                  |
| `api_keys`    | string[] | -      | Várias chaves API do Brave para rotação (prioridade sobre `api_key`) |
| `max_results` | int      | 5      | Número máximo de resultados                                      |

### Tavily

| Config        | Tipo   | Padrão | Descrição                          |
|---------------|--------|--------|------------------------------------|
| `enabled`     | bool   | false  | Habilitar pesquisa Tavily          |
| `api_key`     | string | -      | Chave API do Tavily                |
| `base_url`    | string | -      | URL base personalizada do Tavily   |
| `max_results` | int    | 0      | Número máximo de resultados (0 = padrão) |

### SearXNG

| Config        | Tipo   | Padrão                   | Descrição                      |
|---------------|--------|--------------------------|--------------------------------|
| `enabled`     | bool   | false                    | Habilitar pesquisa SearXNG     |
| `base_url`    | string | -                        | URL da instância SearXNG (por exemplo `http://localhost:8888`) |
| `max_results` | int    | 5                        | Número máximo de resultados    |

### GLM Search

| Config          | Tipo   | Padrão                                               | Descrição                  |
|-----------------|--------|------------------------------------------------------|----------------------------|
| `enabled`       | bool   | false                                                | Habilitar GLM Search       |
| `api_key`       | string | -                                                    | Chave API GLM              |
| `base_url`      | string | `https://open.bigmodel.cn/api/paas/v4/web_search`   | URL da API GLM Search      |
| `search_engine` | string | `search_std`                                         | Tipo de motor de busca     |
| `max_results`   | int    | 5                                                    | Número máximo de resultados |

### Configurações web adicionais

| Config                   | Tipo     | Padrão | Descrição                                                                     |
|--------------------------|----------|--------|--------------------------------------------------------------------------------|
| `provider`               | string   | `auto` | Provedor de pesquisa a usar; `auto` escolhe entre os mecanismos habilitados    |
| `proxy`                  | string   | `""`   | Proxy para as ferramentas web                                                  |
| `prefer_native`          | bool     | true   | Preferir a pesquisa nativa do provedor aos mecanismos de pesquisa configurados |
| `private_host_whitelist` | string[] | `[]`   | Hosts privados/internos permitidos para busca web                              |

### Parâmetros da ferramenta `web_search`

Em tempo de execução, a ferramenta `web_search` aceita os seguintes parâmetros:

| Campo   | Tipo    | Obrigatório | Descrição |
|---------|---------|-------------|-----------|
| `query` | string  | sim         | String de consulta de pesquisa |
| `count` | integer | não         | Número de resultados a retornar. Padrão: `10`, máximo: `10` |
| `range` | string  | não         | Filtro de tempo opcional: `d` (dia), `w` (semana), `m` (mês), `y` (ano) |

Se `range` for omitido, o NeuroClaw faz uma pesquisa sem restrição.
Para o Kagi, `d`, `w` e `m` correspondem ao `time_relative` da lens do Kagi; `y` corresponde a uma data `time_after` da lens um ano antes do dia atual.

### Exemplo de chamada `web_search`

```json
{
  "query": "ai agent news",
  "count": 10,
  "range": "w"
}
```

## Ferramenta Exec

A ferramenta exec é usada para executar comandos shell.

| Config                 | Tipo  | Padrão | Descrição                                      |
|------------------------|-------|--------|-------------------------------------------------|
| `enabled`               | bool  | true   | Habilitar a ferramenta exec. Ligada, é reportada na inicialização como `risk=shell` |
| `enable_deny_patterns`  | bool  | true   | Habilitar bloqueio padrão de comandos perigosos |
| `custom_deny_patterns`  | array | []     | Padrões de negação personalizados (expressões regulares) |
| `custom_allow_patterns` | array | []     | Padrões de permissão personalizados (expressões regulares) |
| `allow_remote`          | bool  | true   | Permitir chamadas exec originadas de canais remotos (aplicativos de chat); `false` limita o exec aos canais internos |
| `timeout_seconds`       | int   | 60     | Tempo limite por comando em segundos; `0` usa o padrão (60) |

### Desabilitando a Ferramenta Exec

Para desabilitar completamente a ferramenta `exec`, defina `enabled` como `false`:

**Via arquivo de configuração:**
```json
{
  "tools": {
    "exec": {
      "enabled": false
    }
  }
}
```

**Via variável de ambiente:**
```bash
NEUROCLAW_TOOLS_EXEC_ENABLED=false
```

> **Nota:** Quando desabilitada, o agent não poderá executar comandos shell. Isso também afeta a capacidade da ferramenta Cron de executar comandos shell agendados.

### Funcionalidade

- **`enable_deny_patterns`**: Defina como `false` para desabilitar completamente os padrões de bloqueio de comandos perigosos padrão
- **`custom_deny_patterns`**: Adicione padrões regex de negação personalizados; comandos correspondentes serão bloqueados
- **`custom_allow_patterns`**: Adicione padrões regex de permissão; comandos correspondentes ficam isentos dos padrões de negação
- **`allow_remote`**: Defina como `false` para recusar chamadas exec vindas de canais remotos

### Padrões de comandos bloqueados por padrão

Por padrão, o NeuroClaw bloqueia os seguintes comandos perigosos:

- Comandos de exclusão: `rm -rf`, `del /f/q`, `rmdir /s`
- Operações de disco: `format`, `mkfs`, `diskpart`, `dd if=`, escrita em `/dev/sd*`
- Operações do sistema: `shutdown`, `reboot`, `poweroff`
- Substituição de comandos: `$()`, `${}`, crases
- Pipe para shell: `| sh`, `| bash`
- Escalação de privilégios: `sudo`, `chmod`, `chown`
- Controle de processos: `pkill`, `killall`, `kill -9`
- Operações remotas: `curl | sh`, `wget | sh`, `ssh`
- Gerenciamento de pacotes: `apt`, `yum`, `dnf`, `npm install -g`, `pip install --user`
- Contêineres: `docker run`, `docker exec`
- Git: `git push`, `git force`
- Outros: `eval`, `source *.sh`

### Limitação arquitetural conhecida

O guarda exec apenas valida o comando de nível superior enviado ao NeuroClaw. Ele **não** inspeciona recursivamente processos filhos gerados por ferramentas de build ou scripts após o início desse comando.

Exemplos de fluxos de trabalho que podem contornar o guarda de comando direto uma vez que o comando inicial é permitido:

- `make run`
- `go run ./cmd/...`
- `cargo run`
- `npm run build`

Isso significa que o guarda é útil para bloquear comandos diretos obviamente perigosos, mas **não** é um sandbox completo para pipelines de build não revisados. Se seu modelo de ameaça inclui código não confiável no workspace, use isolamento mais forte, como contêineres, VMs ou um fluxo de aprovação em torno de comandos de build e execução.

### Exemplo de configuração

```json
{
  "tools": {
    "exec": {
      "enable_deny_patterns": true,
      "custom_deny_patterns": [
        "\\brm\\s+-r\\b",
        "\\bkillall\\s+python"
      ]
    }
  }
}
```

## Ferramenta Cron

A ferramenta cron é usada para agendar tarefas periódicas.

| Config                 | Tipo | Padrão | Descrição                                          |
|------------------------|------|--------|-----------------------------------------------------|
| `enabled`                 | bool     | true   | Registrar a ferramenta cron voltada ao agente |
| `allow_command`           | bool     | true   | Permitir jobs de comando sem confirmação extra. Com `exec` ligado, é reportado na inicialização como `risk=cron_shell` |
| `command_allowed_remotes` | string[] | []     | Canais remotos ou valores `channel:chat_id` permitidos para jobs de comando; `*` permite todos os canais e é perigoso, a menos que todos os canais remotos sejam confiáveis |
| `exec_timeout_minutes` | int  | 5      | Tempo limite de execução em minutos, 0 significa sem limite |

Para tipos de agendamento, modos de execução (`deliver`, turno do agente e jobs de comando), persistência e as barreiras de segurança atuais para comandos, veja [Tarefas Agendadas e Cron Jobs](cron.md).

## Chaves por ferramenta

Cada uma das outras ferramentas tem seu próprio interruptor `enabled` em `tools.<name>` (variável de ambiente `NEUROCLAW_TOOLS_<NAME>_ENABLED`). Os padrões são os de `pkg/config/defaults.go`:

| Ferramenta | Padrão | Notas |
|------------|--------|-------|
| `read_file`, `write_file`, `append_file`, `edit_file`, `list_dir` | ligada | `read_file` também tem `mode` (`bytes` ou `lines`, padrão `bytes`) e `max_read_file_size` (padrão 65536) |
| `load_image`, `send_file`, `web_fetch` | ligada | |
| `message` | ligada | `media_enabled` vem desligado por padrão |
| `spawn`, `subagent` | ligada | |
| `spawn_status` | desligada | |
| `get_diagnostics` | ligada | Veja [Ferramenta de diagnóstico](#diagnostics-tool) |
| `find_skills`, `install_skill` | ligada | Só têm efeito com `tools.skills.enabled`; reportadas como `risk=skill_install` |
| `send_tts` | desligada | |
| `i2c`, `spi`, `serial` | desligada | Ferramentas de hardware; `i2c` e `spi` são só Linux. Reportadas como `risk=hardware_bus` quando ligadas |
| `media_cleanup` | ligada | `max_age_minutes` 30, `interval_minutes` 5 |

Os rótulos de risco e o que cada um significa estão em [O que o NeuroClaw pode fazer, seção 2](capabilities.md#2-risk-labels).

<a id="diagnostics-tool"></a>
## Ferramenta de diagnóstico

`get_diagnostics` permite que o modelo informe a saúde do próprio NeuroClaw. É somente leitura, não recebe argumentos e tem o mesmo nome da ferramenta do NeuroClaw-ESP. Ela devolve um JSON com:

- `version`, `git_commit`, `go_version`, `os`, `arch`, `num_cpu`;
- `started_at` e `uptime_sec` do processo;
- `memory` (heap do Go e bytes totais segundo o runtime, número de GCs) e `goroutines`;
- `workspace_disk`: bytes livres e totais do sistema de arquivos onde fica o workspace do agente (Linux, macOS, FreeBSD, Windows; nos demais, uma entrada `error` diz que o valor não é informado);
- `channels_enabled`: nomes dos canais habilitados;
- `models`: o modelo padrão, seus fallbacks e `cooldowns`, uma entrada por modelo que falhou desde o início (`available: false` enquanto a cadeia de fallback o pula, `remaining_sec`, `disabled_reason` para erros de cobrança);
- `llm_budget`: limites, chamadas nesta hora e hoje, horários de reinício (veja [orçamento de chamadas LLM](rate-limiting.md#llm-call-budget), em inglês);
- `risk_labels`: as capacidades de risco que estão ligadas (como registradas na inicialização);
- `last_llm_error`: horário e mensagem da última chamada LLM que falhou, ou `null`.

A ferramenta informa apenas nomes, contadores e estados. Chaves de API, tokens e URLs nunca aparecem: no último erro de LLM, credenciais e query strings são removidas de qualquer URL, e o texto ainda passa pela [filtragem de dados sensíveis](../security/sensitive_data_filtering.md).

| Config | Tipo | Padrão | Descrição |
|--------|------|--------|-----------|
| `enabled` | bool | true | Registra a ferramenta `get_diagnostics` |

```json
{
  "tools": {
    "get_diagnostics": {
      "enabled": true
    }
  }
}
```

Variável de ambiente: `NEUROCLAW_TOOLS_GET_DIAGNOSTICS_ENABLED`.

<a id="mcp-tool"></a>
## Ferramenta MCP

A ferramenta MCP permite a integração com servidores Model Context Protocol externos.

### Descoberta de ferramentas (carregamento preguiçoso)

Ao conectar a vários servidores MCP, expor centenas de ferramentas simultaneamente pode esgotar a janela de contexto do LLM e aumentar os custos de API. O recurso **Discovery** resolve isso mantendo as ferramentas MCP *ocultas* por padrão.

Em vez de carregar todas as ferramentas, o LLM recebe uma ferramenta de pesquisa leve (usando correspondência de palavras-chave BM25 ou Regex). Quando o LLM precisa de uma capacidade específica, ele pesquisa a biblioteca oculta. As ferramentas correspondentes são então temporariamente "desbloqueadas" e injetadas no contexto por um número configurado de turnos (`ttl`).

### Configuração global

| Config      | Tipo   | Padrão | Descrição                                    |
|-------------|--------|--------|----------------------------------------------|
| `enabled`   | bool   | false  | Habilitar integração MCP globalmente         |
| `discovery` | object | `{}`   | Configuração de descoberta de ferramentas (veja abaixo) |
| `servers`   | object | `{}`   | Mapa de nome do servidor para configuração do servidor |

### Configuração Discovery (`discovery`)

| Config               | Tipo | Padrão | Descrição                                                                                                                         |
|----------------------|------|--------|-----------------------------------------------------------------------------------------------------------------------------------|
| `enabled`            | bool | false  | Se true, as ferramentas MCP ficam ocultas e são carregadas sob demanda via pesquisa. Se false, todas as ferramentas são carregadas |
| `ttl`                | int  | 5      | Número de turnos de conversa que uma ferramenta descoberta permanece desbloqueada                                                 |
| `max_search_results` | int  | 5      | Número máximo de ferramentas retornadas por consulta de pesquisa                                                                  |
| `use_bm25`           | bool | true   | Habilitar a ferramenta de pesquisa por linguagem natural/palavras-chave (`tool_search_tool_bm25`). **Aviso**: consome mais recursos que a pesquisa regex |
| `use_regex`          | bool | false  | Habilitar a ferramenta de pesquisa por padrão regex (`tool_search_tool_regex`)                                                    |

> **Nota:** Se `discovery.enabled` for `true`, você **deve** habilitar pelo menos um mecanismo de pesquisa (`use_bm25` ou `use_regex`),
> caso contrário a aplicação falhará ao iniciar.

### Configuração por servidor

| Config     | Tipo   | Obrigatório | Descrição                                  |
|------------|--------|-------------|--------------------------------------------|
| `enabled`  | bool   | sim         | Habilitar este servidor MCP                |
| `deferred` | bool   | não         | Substitui o modo deferred apenas para este servidor. `true` = as ferramentas ficam ocultas e são descobertas via pesquisa; `false` = as ferramentas ficam sempre visíveis no contexto. Se omitido, vale o valor global de `discovery.enabled`. |
| `type`     | string | não         | Tipo de transporte: `stdio`, `sse`, `http` (`streamable-http` é aceito como alias de `http`) |
| `command`  | string | stdio       | Comando executável para transporte stdio   |
| `args`     | array  | não         | Argumentos do comando para transporte stdio |
| `env`      | object | não         | Variáveis de ambiente para processo stdio  |
| `env_file` | string | não         | Caminho para arquivo de ambiente para processo stdio |
| `url`      | string | sse/http    | URL do endpoint para transporte `sse`/`http` |
| `headers`  | object | não         | Cabeçalhos HTTP para transporte `sse`/`http` |
| `tls`      | object | não         | NeuroClaw: TLS mútuo para transporte `sse`/`http` (veja abaixo) |

### TLS mútuo (`tls`)

Acréscimo do NeuroClaw. Uma caixa doméstica NeuroEdge iniciada com `mcp serve --http` recusa qualquer cliente sem um certificado assinado por ela. Defina `tls` em um servidor `http` ou `sse` para que o cérebro apresente um certificado de cliente e confie apenas na CA da caixa, sem uma ponte TLS ao lado.

| Config        | Tipo   | Descrição |
|---------------|--------|-----------|
| `ca_file`     | string | Arquivo PEM da CA que assinou o certificado do servidor. Vazio: raízes do sistema |
| `cert_file`   | string | Arquivo PEM do certificado deste cliente. Defina junto com `key_file`, ou nenhum dos dois |
| `key_file`    | string | Arquivo PEM da chave privada deste cliente |
| `server_name` | string | Substitui o nome verificado no certificado do servidor |

Os caminhos podem começar com `~`. Regras, de `pkg/mcp/tls.go`:

- `url` deve ser `https://`; caso contrário o servidor falha ao conectar com `tls is set but url ... is not https`.
- A conexão usa no mínimo TLS 1.3.
- Sem `tls`, usa-se o transporte HTTP padrão, como no PicoClaw.
- `tls` é ignorado por servidores `stdio`.
- `neuroclaw mcp add` não tem flag para `tls`; adicione-o editando o `config.json` (`neuroclaw mcp edit`). Executar `mcp add` de novo com o mesmo nome substitui a entrada e descarta o bloco `tls`.

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "servers": {
        "home": {
          "enabled": true,
          "type": "http",
          "url": "https://neuroedge-box.local:8443/mcp",
          "tls": {
            "ca_file": "~/.neuroclaw/pki/ca.pem",
            "cert_file": "~/.neuroclaw/pki/brain.pem",
            "key_file": "~/.neuroclaw/pki/brain.key",
            "server_name": "neuroedge-box.local"
          }
        }
      }
    }
  }
}
```

Veja o [guia NeuroEdge](../guides/neuroedge.pt-br.md) para o lado da caixa doméstica.

### Comportamento do transporte

- Se `type` for omitido, o transporte é detectado automaticamente:
    - `url` está definido → `sse`
    - `command` está definido → `stdio`
- `http` e `sse` ambos usam `url` + `headers` opcionais + `tls` opcional.
- `neuroclaw mcp add` usa `stdio` por padrão (veja [CLI do servidor MCP](mcp-cli.md)); essa detecção automática vale apenas para entradas escritas à mão.
- Um servidor `stdio` habilitado é reportado na inicialização como `risk=stdio_mcp`.
- `env` e `env_file` são aplicados apenas a servidores `stdio`.

### Exemplos de configuração

#### 1) Servidor MCP Stdio

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "servers": {
        "filesystem": {
          "enabled": true,
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-filesystem",
            "/tmp"
          ]
        }
      }
    }
  }
}
```

#### 2) Servidor MCP remoto SSE/HTTP

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "servers": {
        "remote-mcp": {
          "enabled": true,
          "type": "sse",
          "url": "https://example.com/mcp",
          "headers": {
            "Authorization": "Bearer YOUR_TOKEN"
          }
        }
      }
    }
  }
}
```

#### 3) Configuração MCP massiva com descoberta de ferramentas habilitada

*Neste exemplo, o LLM verá apenas o `tool_search_tool_bm25`. Ele pesquisará e desbloqueará ferramentas do Github ou Postgres dinamicamente apenas quando solicitado pelo usuário.*

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "discovery": {
        "enabled": true,
        "ttl": 5,
        "max_search_results": 5,
        "use_bm25": true,
        "use_regex": false
      },
      "servers": {
        "github": {
          "enabled": true,
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-github"
          ],
          "env": {
            "GITHUB_PERSONAL_ACCESS_TOKEN": "YOUR_GITHUB_TOKEN"
          }
        },
        "postgres": {
          "enabled": true,
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-postgres",
            "postgresql://user:password@localhost/dbname"
          ]
        },
        "slack": {
          "enabled": true,
          "type": "slack",
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-slack"
          ],
          "env": {
            "SLACK_BOT_TOKEN": "YOUR_SLACK_BOT_TOKEN",
            "SLACK_TEAM_ID": "YOUR_SLACK_TEAM_ID"
          }
        }
      }
    }
  }
}
```

#### 4) Configuração mista: substituição de deferred por servidor

*A descoberta está habilitada globalmente, mas `filesystem` é fixado como sempre visível, enquanto `context7` segue o padrão global (deferred). `aws` opta explicitamente pelo modo deferred, embora isso seja igual ao padrão global.*

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "discovery": {
        "enabled": true,
        "ttl": 5,
        "max_search_results": 5,
        "use_bm25": true
      },
      "servers": {
        "filesystem": {
          "enabled": true,
          "command": "npx",
          "args": ["-y", "@modelcontextprotocol/server-filesystem", "/workspace"],
          "deferred": false
        },
        "context7": {
          "enabled": true,
          "command": "npx",
          "args": ["-y", "@upstash/context7-mcp"]
        },
        "aws": {
          "enabled": true,
          "command": "npx",
          "args": ["-y", "aws-mcp-server"],
          "deferred": true
        }
      }
    }
  }
}
```

> **Dica:** `deferred` por servidor é independente de `discovery.enabled`. Você pode manter `discovery.enabled: false` globalmente (todas as ferramentas visíveis por padrão) e ainda marcar servidores individuais de alto volume com `"deferred": true` para não poluir o contexto com as ferramentas deles.

<a id="skills-tool"></a>
## Ferramenta Skills

A ferramenta skills configura a descoberta e a instalação de skills por meio de registros como o ClawHub e o GitHub.

### Registros

| Config                             | Tipo   | Padrão               | Descrição                                    |
|------------------------------------|--------|----------------------|----------------------------------------------|
| `registries.clawhub.enabled`       | bool   | true                 | Habilitar registro ClawHub                   |
| `registries.clawhub.base_url`      | string | `https://clawhub.ai` | URL base do ClawHub                          |
| `registries.clawhub.auth_token`    | string | `""`                 | Token Bearer opcional para limites de taxa mais altos |
| `registries.clawhub.search_path`   | string | `""`                 | Caminho da API de pesquisa                   |
| `registries.clawhub.skills_path`   | string | `""`                 | Caminho da API de skills                     |
| `registries.clawhub.download_path` | string | `""`                 | Caminho da API de download                   |
| `registries.clawhub.timeout`       | int    | 0                    | Tempo limite da requisição em segundos (0 = padrão) |
| `registries.clawhub.max_zip_size`  | int    | 0                    | Tamanho máximo do zip da skill em bytes (0 = padrão) |
| `registries.clawhub.max_response_size` | int | 0                   | Tamanho máximo da resposta da API em bytes (0 = padrão) |
| `registries.github.enabled`        | bool   | true                 | Habilitar instalações do GitHub via configuração de registro |
| `registries.github.base_url`       | string | `https://github.com` | URL base do GitHub ou GitHub Enterprise      |
| `registries.github.auth_token`     | string | `""`                 | Token de acesso pessoal do GitHub            |
| `registries.github.proxy`          | string | `""`                 | Proxy HTTP para requisições à API do GitHub  |

### Configuração legada do GitHub

`github.*` está obsoleto. Use `registries.github.*`. Os campos legados ainda são suportados por compatibilidade e serão removidos depois.

| Config             | Tipo   | Padrão               | Descrição                      |
|--------------------|--------|----------------------|--------------------------------|
| `github.base_url`  | string | `https://github.com` | URL base do GitHub (obsoleto)  |
| `github.proxy`     | string | `""`                 | Proxy do GitHub (obsoleto)     |
| `github.token`     | string | `""`                 | Token do GitHub (obsoleto)     |

### Configurações de pesquisa

| Config                    | Tipo | Padrão | Descrição                                  |
|---------------------------|------|--------|--------------------------------------------|
| `max_concurrent_searches` | int  | 2      | Máximo de requisições simultâneas de pesquisa de skills |
| `search_cache.max_size`   | int  | 50     | Máximo de resultados de pesquisa em cache  |
| `search_cache.ttl_seconds`| int  | 300    | TTL do cache em segundos                   |

### Exemplo de configuração

```json
{
  "tools": {
    "skills": {
      "registries": {
        "clawhub": {
          "enabled": true,
          "base_url": "https://clawhub.ai",
          "auth_token": "",
          "search_path": "",
          "skills_path": "",
          "download_path": "",
          "timeout": 0,
          "max_zip_size": 0,
          "max_response_size": 0
        },
        "github": {
          "enabled": true,
          "base_url": "https://github.com",
          "auth_token": "",
          "proxy": ""
        }
      },
      "github": {
        "base_url": "https://github.com",
        "proxy": "",
        "token": ""
      },
      "max_concurrent_searches": 2,
      "search_cache": {
        "max_size": 50,
        "ttl_seconds": 300
      }
    }
  }
}
```

## Variáveis de ambiente

Todas as opções de configuração podem ser substituídas via variáveis de ambiente com o formato `NEUROCLAW_TOOLS_<SECTION>_<KEY>`:

Por exemplo:

- `NEUROCLAW_TOOLS_WEB_BRAVE_ENABLED=true`
- `NEUROCLAW_TOOLS_EXEC_ENABLED=false`
- `NEUROCLAW_TOOLS_EXEC_ENABLE_DENY_PATTERNS=false`
- `NEUROCLAW_TOOLS_CRON_EXEC_TIMEOUT_MINUTES=10`
- `NEUROCLAW_TOOLS_MCP_ENABLED=true`
- `NEUROCLAW_TOOLS_MCP_MAX_INLINE_TEXT_CHARS=16384`

Nota: Configuração de tipo mapa aninhado (por exemplo `tools.mcp.servers.<name>.*`) é configurada no `config.json` em vez de variáveis de ambiente.

Para as ferramentas MCP, `tools.mcp.max_inline_text_chars` controla quanto texto de resultado é mantido inline no contexto do modelo. O limite é contado em caracteres Unicode (runes do Go), não em bytes. Por exemplo, `16384` significa até 16.384 caracteres inline, o que pode ocupar mais de 16 KB para texto multibyte, como CJK. Acima desse limite, o NeuroClaw salva o resultado de texto do MCP como um artefato local no workspace do agente e dá ao modelo uma nota curta mais um caminho estruturado de artefato `[file:...]`, em vez de injetar a carga completa no contexto.
