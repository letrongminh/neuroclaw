# ⚙️ Guia de Configuração

> Voltar ao [README](../project/README.pt-br.md)

## ⚙️ Configuração

Arquivo de configuração: `~/.neuroclaw/config.json`

> **Configuração de segurança:** Para armazenar chaves de API, tokens e outros dados sensíveis, veja o [Guia de Configuração de Segurança](../security/security_configuration.md).

### Variáveis de Ambiente

Você pode substituir os caminhos padrão usando variáveis de ambiente. Isso é útil para instalações portáteis, implantações em contêineres ou execução do neuroclaw como serviço do sistema. Essas variáveis são independentes e controlam caminhos diferentes.

| Variável          | Descrição                                                                                                                             | Caminho Padrão              |
|-------------------|-----------------------------------------------------------------------------------------------------------------------------------------|---------------------------|
| `NEUROCLAW_CONFIG` | Substitui o caminho para o arquivo de configuração. Isso indica diretamente ao neuroclaw qual `config.json` carregar, ignorando todos os outros locais. | `~/.neuroclaw/config.json` |
| `NEUROCLAW_HOME`   | Substitui o diretório raiz para dados do neuroclaw. Isso altera o local padrão do `workspace` e outros diretórios de dados.          | `~/.neuroclaw`             |

**Exemplos:**

```bash
# Executar neuroclaw usando um arquivo de configuração específico
# O caminho do workspace será lido de dentro desse arquivo de configuração
NEUROCLAW_CONFIG=/etc/neuroclaw/production.json neuroclaw gateway

# Executar neuroclaw com todos os dados armazenados em /opt/neuroclaw
# A configuração será carregada do padrão ~/.neuroclaw/config.json
# O workspace será criado em /opt/neuroclaw/workspace
NEUROCLAW_HOME=/opt/neuroclaw neuroclaw agent

# Usar ambos para uma configuração totalmente personalizada
NEUROCLAW_HOME=/srv/neuroclaw NEUROCLAW_CONFIG=/srv/neuroclaw/main.json neuroclaw gateway
```

### Nível de Log do Gateway

`gateway.log_level` controla a verbosidade dos logs do Gateway, configurável em `config.json`.

```json
{
  "gateway": {
    "log_level": "warn"
  }
}
```

Quando omitido, o padrão é `warn`. Valores suportados: `debug`, `info`, `warn`, `error`, `fatal`.

Também pode ser substituído pela variável de ambiente `NEUROCLAW_LOG_LEVEL`.

### Layout do Workspace

O NeuroClaw armazena dados no seu workspace configurado (padrão: `~/.neuroclaw/workspace`):

```
~/.neuroclaw/workspace/
├── sessions/          # Sessões de conversa e histórico
├── memory/           # Memória de longo prazo (MEMORY.md)
├── state/            # Estado persistente (último canal, etc.)
├── cron/             # Banco de dados de tarefas agendadas
├── skills/           # Skills personalizadas
├── AGENT.md          # Guia de comportamento do agente
├── HEARTBEAT.md      # Prompts de tarefas periódicas (verificados a cada 30 min)
├── IDENTITY.md       # Identidade do agente
├── SOUL.md           # Alma do agente
└── USER.md           # Preferências do usuário
```

> **Nota:** Alterações em `AGENT.md`, `SOUL.md`, `USER.md` e `memory/MEMORY.md` são detectadas automaticamente em tempo de execução via rastreamento de data de modificação (mtime). **Não é necessário reiniciar o gateway** após editar esses arquivos — o agente carrega o novo conteúdo na próxima requisição.

### Autoevolução do Agente

O bloco `evolution` controla o runtime de autoevolução do NeuroClaw. Quando habilitado, o agente registra os turnos concluídos como registros de aprendizado. Em modos superiores, ele pode agrupar padrões bem-sucedidos repetidos, gerar rascunhos de skills e, opcionalmente, aplicar rascunhos aceitos nas skills do workspace.

```json
{
  "evolution": {
    "enabled": false,
    "mode": "observe",
    "state_dir": "",
    "min_task_count": 2,
    "min_success_ratio": 0.7,
    "cold_path_trigger": "after_turn",
    "cold_path_times": []
  }
}
```

| Campo | Padrão | Descrição |
|-------|--------|-----------|
| `enabled` | `false` | Habilita a captura de registros de aprendizado dos turnos concluídos do agente. Turnos de heartbeat são ignorados. |
| `mode` | `observe` | `observe` apenas registra dados. `draft` pode gerar rascunhos candidatos de skills. `apply` pode aplicar rascunhos aceitos nas skills do workspace. |
| `state_dir` | `""` | Diretório opcional para o estado da evolução. Deixe vazio para usar o padrão dentro do workspace. |
| `min_task_count` | `2` | Número mínimo de registros de tarefas relacionadas exigido para que um padrão seja elegível à geração de rascunho. |
| `min_success_ratio` | `0.7` | Taxa mínima de sucesso de um grupo de tarefas. Use um valor maior que `0` e até `1`. |
| `cold_path_trigger` | `after_turn` | Executa a geração de rascunhos `after_turn`, em horários `scheduled`, ou desativa as execuções automáticas do cold path quando definido como `manual`. Ainda não há um gatilho manual para o usuário. Aplica-se apenas aos modos `draft` e `apply`. |
| `cold_path_times` | `[]` | Horários de execução usados quando `cold_path_trigger` é `scheduled`, escritos como strings `HH:MM`. |

Use `observe` primeiro se quiser inspecionar os registros de aprendizado sem gerar alterações em skills. Use `draft` quando quiser que o NeuroClaw prepare melhorias revisáveis. Use `apply` apenas se estiver confortável em deixar que rascunhos aceitos atualizem as skills do workspace.

### Política de contexto da requisição

`turn_profile` é uma política opcional de contexto da requisição em `agents.defaults.turn_profile`. Deixe-a sem definir, ou com `"enabled": false`, para manter o comportamento normal do NeuroClaw. Com `"enabled": true`, a mesma política se aplica a cada novo turno.

Todos os blocos usam os mesmos valores de `mode`:

| Mode | Significado |
| --- | --- |
| `default` | Mantém o comportamento normal do NeuroClaw para esse bloco. Blocos ausentes ou sem `mode` são tratados como `default`. |
| `off` | Desativa esse bloco para o turno. |
| `custom` | Usa uma lista de permissão. Nesta versão, `custom` só é suportado para `skills` e `tools`; usá-lo em `history` ou `system_prompt` gera erro de validação. |

Blocos disponíveis:

| Bloco | O que controla |
| --- | --- |
| `history` | Se o turno lê o histórico e o resumo anteriores da sessão, grava mensagens de usuário/assistente/ferramenta, ingere contexto e executa compactação ou resumo. |
| `system_prompt` | Se o NeuroClaw injeta sua identidade padrão, instruções do workspace, memória, contexto de execução e resumo. Prompts de sistema externos da requisição ainda são permitidos quando este bloco está `off`. |
| `skills` | Se o catálogo de skills e o conteúdo do prompt de skills ativas são carregados. `custom.allow` mantém no contexto do prompt apenas os nomes de skills listados. |
| `tools` | Quais ferramentas chamáveis são expostas ao modelo e permitidas na execução. `custom.allow` mantém apenas os nomes de ferramentas registradas listados. |

Quando `system_prompt.mode` é `off`, as ferramentas continuam visíveis e nenhum prompt de sistema externo é fornecido, o NeuroClaw usa sua regra existente de uso de ferramentas como prompt mínimo de fallback. Se `tools.mode` é `off`, nenhum prompt de fallback é adicionado.

Exemplo de política web limpa:

```json
{
  "agents": {
    "defaults": {
      "turn_profile": {
        "enabled": true,
        "history": { "mode": "off" },
        "system_prompt": { "mode": "off" },
        "skills": { "mode": "off" },
        "tools": {
          "mode": "custom",
          "allow": ["web_search", "web_fetch"]
        }
      }
    }
  }
}
```

### Painel web do launcher

O **neuroclaw-launcher** serve uma interface de navegador que exige primeiro um login com senha. Na primeira execução, abra `/launcher-setup` para criar a senha do painel. Os logins manuais seguintes usam `/launcher-login`.

- **Arquivo de configuração**: No mesmo diretório do `config.json` (ou no arquivo apontado por `NEUROCLAW_CONFIG`). O arquivo específico do launcher é `launcher-config.json`.
- **Armazenamento da senha**: Nas plataformas suportadas, a senha é armazenada como hash bcrypt em `launcher-auth.db`. Nas plataformas em que o armazenamento SQLite de senhas não está disponível, o hash bcrypt fica em `launcher-config.json`.
- **Migração legada**: Valores antigos de `launcher_token` são migrados uma vez para o login com senha e removidos da configuração salva do launcher.
- **Login automático local**: Quando o launcher abre automaticamente um navegador local após a inicialização, ele usa um endpoint de bootstrap de uso único, restrito ao loopback, para definir o cookie de sessão automaticamente.
- **Caminhos de autenticação não suportados**: Login por token na URL (`?token=...`), `NEUROCLAW_LAUNCHER_TOKEN` e autenticação do painel por `Authorization: Bearer` não são mais suportados.
- **Sair**: Use **`POST /api/auth/logout`** com **`Content-Type: application/json`** (o corpo pode ser `{}`). Não dependa de uma URL GET para o logout (padrão seguro contra CSRF).
- **Força bruta**: **`POST /api/auth/login`** tem **limite de taxa por IP de cliente por minuto** (HTTP 429 quando excedido).
- **Duração da sessão**: O cookie de sessão HttpOnly dura cerca de **31 dias** por padrão, mas as sessões são invalidadas quando o processo do launcher é reiniciado.

### Fontes de Skills

Por padrão, as skills são carregadas de:

1. `~/.neuroclaw/workspace/skills` (workspace)
2. `~/.neuroclaw/skills` (global)
3. `<caminho-embutido-no-binário>/skills` (builtin, definido na compilação)

Para configurações avançadas/de teste, você pode substituir o diretório raiz das skills builtin com:

```bash
export NEUROCLAW_BUILTIN_SKILLS=/path/to/skills
```

### Usando Skills a partir de Canais de Chat

Depois que as skills estiverem instaladas e os servidores MCP configurados, você pode inspecioná-los e forçá-los diretamente a partir de um canal de chat:

- `/list skills` mostra os nomes das skills instaladas disponíveis para o agente atual.
- `/list mcp` mostra os servidores MCP configurados com o status habilitado/deferred/conectado.
- `/show mcp <server>` mostra as ferramentas ativas expostas por um servidor MCP conectado.
- `/use <skill> <message>` força uma skill específica para uma única requisição.
- `/use <skill>` prepara essa skill para a sua próxima mensagem na mesma sessão de chat.
- `/use clear` cancela uma substituição pendente criada por `/use <skill>`.
- `/btw <question>` faz uma pergunta lateral imediata sem alterar o histórico atual da sessão. `/btw` é tratado como uma consulta sem ferramentas e não entra no fluxo normal de execução de ferramentas.

Exemplos:

```text
/list skills
/list mcp
/show mcp github
/use git explain how to squash the last 3 commits
/btw remind me what we already decided about the deploy plan
/use italiapersonalfinance
dammi le ultime news
```

### Política Unificada de Execução de Comandos

- Comandos slash genéricos são executados através de um único caminho em `pkg/agent/loop.go` via `commands.Executor`.
- Os adaptadores de canal não consomem mais comandos genéricos localmente; eles encaminham o texto de entrada para o caminho bus/agent. O Telegram ainda registra automaticamente na inicialização comandos suportados como `/start`, `/help`, `/show`, `/list`, `/use` e `/btw`.
- Comando slash desconhecido (por exemplo `/foo`) passa para o processamento normal do LLM.
- Comando registrado mas não suportado no canal atual (por exemplo `/show` no WhatsApp) retorna um erro explícito ao usuário e interrompe o processamento.

### Isolamento de Sessão

O escopo da sessão controla quanta memória é compartilhada entre chats, usuários, threads e espaços.

- Use `session.dimensions` para o padrão global.
- Use `session_dimensions` em uma regra de despacho para uma exceção roteada.

Para receitas passo a passo e padrões de isolamento, veja o [Guia de Sessão](session-guide.md).

### Roteamento

O roteamento é configurado por meio de `agents.dispatch.rules`.

Cada regra é comparada com o contexto de entrada normalizado produzido pelos canais.
As regras são avaliadas de cima para baixo. A primeira regra que corresponder vence. Se nenhuma
regra corresponder, o NeuroClaw recorre ao agente padrão configurado.

Campos de correspondência suportados:

* `channel`
* `account`
* `space`
* `chat`
* `topic`
* `sender`
* `mentioned`

Os valores de correspondência usam o mesmo vocabulário de escopo do sistema de sessões:

* `space`: `workspace:t001`, `guild:123456`
* `chat`: `direct:user123`, `group:-100123`, `channel:c123`
* `topic`: `topic:42`
* `sender`: um identificador normalizado do remetente na plataforma

As regras podem, opcionalmente, substituir o valor global de `session.dimensions`
por meio de `session_dimensions`. Isso permite manter o roteamento e a alocação de sessões alinhados
sem reintroduzir os antigos formatos `bindings` ou `dm_scope`.

Exemplo:

```json
{
  "agents": {
    "list": [
      { "id": "main", "default": true },
      { "id": "support" },
      { "id": "sales" }
    ],
    "dispatch": {
      "rules": [
        {
          "name": "vip in support group",
          "agent": "sales",
          "when": {
            "channel": "telegram",
            "chat": "group:-1001234567890",
            "sender": "12345"
          },
          "session_dimensions": ["chat", "sender"]
        },
        {
          "name": "telegram support group",
          "agent": "support",
          "when": {
            "channel": "telegram",
            "chat": "group:-1001234567890"
          },
          "session_dimensions": ["chat"]
        }
      ]
    }
  },
  "session": {
    "dimensions": ["chat"]
  }
}
```

No exemplo acima, a regra VIP deve aparecer antes da regra mais ampla do grupo.
Como o roteamento é estritamente ordenado, as regras mais específicas devem ser colocadas
antes e as regras de fallback mais amplas depois.

Para exemplos mais completos de roteamento e de camadas de modelos, veja o [Guia de Roteamento](routing-guide.md).

### Lista de Ferramentas Permitidas do Agente

As declarações de ferramentas por agente ficam no frontmatter do `AGENT.md`, não no `config.json`.

Se `tools` for omitido do frontmatter, o agente recebe o conjunto normal de ferramentas habilitadas globalmente. Se `tools` estiver presente, o NeuroClaw registra para esse agente apenas as ferramentas de runtime listadas.

```md
---
name: Research Agent
description: Specialist for web research and in-depth analysis.
tools: [read_file, write_file, web_search, web_fetch, message]
skills: [deep-research]
mcpServers: [web-index]
---

You are the research agent.
```

Notas:

- Isto é uma lista de permissão, não uma dica de preferência.
- Os nomes das ferramentas são comparados 1:1 com o nome da ferramenta em runtime.
- Use nomes de ferramentas de runtime como `web_search`, `web_fetch`, `spawn`, `subagent`, `send_file`.
- As declarações de ferramentas no `AGENT.md` são usadas pelo runtime/ferramental, mas não são injetadas no prompt de descoberta.

### Descoberta de Agentes (Automática)

Quando um agente tem pares que pode criar e pode chamar `spawn`, o NeuroClaw injeta um registro estruturado de agentes no prompt de sistema desse agente a cada turno. Nenhuma chamada extra à ferramenta `list_agents` é necessária.

Esse registro serve para tornar a delegação concreta e confiável, especialmente ao usar `spawn` com um `agent_id` de destino.

Cada entrada inclui:

| Campo | Significado |
|-------|-------------|
| `id` | ID estável do agente |
| `name` | Nome de identidade do agente, do frontmatter do `AGENT.md` |
| `description` | Descrição de identidade do agente, do frontmatter do `AGENT.md` |

Comportamento importante:

- A seção de descoberta aparece apenas quando o agente atual tem a ferramenta `spawn` e inclui somente os agentes pares que ele tem permissão de criar via `subagents.allow_agents`.
- O agente atual e os pares que não podem ser criados são omitidos, para que o modelo não planeje com base em agentes indisponíveis.
- A descoberta é propositalmente leve. Ela dá ao modelo apenas a identidade de que precisa para escolher um par: `id`, `name` e `description`.
- O `config.json` continua sendo a camada de infraestrutura: workspace, seleção do agente padrão, roteamento e permissões de subagentes. Essas permissões também controlam a visibilidade na descoberta.
- O `AGENT.md` continua sendo a camada de identidade. O código de runtime/ferramentas ainda pode usar seus campos `tools`, `skills`, `mcpServers` e `model` quando a delegação acontece.

Exemplo do formato injetado:

```json
{
  "agents": [
    {
      "id": "research",
      "name": "Research Agent",
      "description": "Specialist for long-form investigation and web work."
    }
  ]
}
```

Na prática, isso significa que um agente generalista pode escolher um par com base na descrição do seu papel e depois chamar `spawn` com o `agent_id` do par. O runtime resolve o resto.

### 🔒 Sandbox de Segurança

O NeuroClaw é executado em um ambiente sandbox por padrão. O agente só pode acessar arquivos e executar comandos dentro do workspace configurado.

#### Configuração Padrão

```json
{
  "agents": {
    "defaults": {
      "workspace": "~/.neuroclaw/workspace",
      "restrict_to_workspace": true
    }
  }
}
```

| Opção                   | Padrão                  | Descrição                                 |
| ----------------------- | ----------------------- | ----------------------------------------- |
| `workspace`             | `~/.neuroclaw/workspace` | Diretório de trabalho do agente           |
| `restrict_to_workspace` | `true`                  | Restringir acesso a arquivos/comandos ao workspace |

#### Ferramentas Protegidas

Quando `restrict_to_workspace: true`, as seguintes ferramentas são isoladas:

| Ferramenta    | Função           | Restrição                              |
| ------------- | ---------------- | -------------------------------------- |
| `read_file`   | Ler arquivos     | Apenas arquivos dentro do workspace    |
| `write_file`  | Escrever arquivos| Apenas arquivos dentro do workspace    |
| `list_dir`    | Listar diretórios| Apenas diretórios dentro do workspace  |
| `edit_file`   | Editar arquivos  | Apenas arquivos dentro do workspace    |
| `append_file` | Anexar a arquivos| Apenas arquivos dentro do workspace    |
| `exec`        | Executar comandos| Caminhos de comando devem estar dentro do workspace |

#### Proteção Adicional do Exec

Mesmo com `restrict_to_workspace: false`, a ferramenta `exec` bloqueia estes comandos perigosos:

* `rm -rf`, `del /f`, `rmdir /s` — Exclusão em massa
* `format`, `mkfs`, `diskpart` — Formatação de disco
* `dd if=` — Imagem de disco
* Escrita em `/dev/sd[a-z]` — Escritas diretas em disco
* `shutdown`, `reboot`, `poweroff` — Desligamento do sistema
* Fork bomb `:(){ :|:& };:`
* Entre outros: `sudo`, `chmod`/`chown`, `kill`/`pkill`, `curl ... | sh`, `git push`, `docker run`, instalação de pacotes e substituição de comandos (`$(...)`, crases). A lista completa é `defaultDenyPatterns` em `pkg/tools/shell.go`.

### Controle de Acesso a Arquivos

| Chave de config | Tipo | Padrão | Descrição |
|-----------------|------|--------|-----------|
| `tools.allow_read_paths` | string[] | `[]` | Caminhos adicionais permitidos para leitura fora do workspace |
| `tools.allow_write_paths` | string[] | `[]` | Caminhos adicionais permitidos para escrita fora do workspace |
| `tools.message.media_enabled` | bool | `false` | Permite que a ferramenta `message` anexe arquivos de mídia locais por caminho. É separado de `tools.send_file.enabled`; habilite apenas quando a entrega unificada de texto/mídia/legenda for pretendida. |

### Modo do Read File

`read_file` tem duas implementações mutuamente exclusivas, selecionadas por configuração. O NeuroClaw registra exatamente uma delas na inicialização:

| Chave de config | Tipo | Padrão | Descrição |
|-----------------|------|--------|-----------|
| `tools.read_file.enabled` | bool | `true` | Habilita a ferramenta `read_file` |
| `tools.read_file.mode` | string | `bytes` | Seleciona a implementação de `read_file`: `bytes` ou `lines` |
| `tools.read_file.max_read_file_size` | int | `65536` | Máximo de bytes retornados por `read_file` |

#### Modo: `bytes`

Otimizado para arquivos arbitrários e paginação segura para binários.

Parâmetros:

* `path` (obrigatório): Caminho do arquivo
* `offset` (opcional): Offset inicial em bytes, padrão `0`
* `length` (opcional): Número máximo de bytes a ler, padrão `max_read_file_size`

Use `bytes` quando:

* Você pode ler arquivos binários
* Você quer paginação determinística por intervalo de bytes

#### Modo: `lines`

Comportamento orientado a texto, otimizado para arquivos-fonte, markdown, logs e configs. A ferramenta lê sequencialmente por linha e para quando o orçamento de bytes configurado é atingido.

Parâmetros:

* `path` (obrigatório): Caminho do arquivo
* `start_line` (opcional): Número da linha inicial, indexado a partir de 1 e inclusivo, padrão `1`
* `max_lines` (opcional): Número máximo de linhas a ler, padrão = todas as linhas restantes até o EOF ou o orçamento de bytes

Notas de comportamento:

* Arquivos com aparência binária são rejeitados, com a orientação de mudar `read_file` para `mode = bytes`
* Linhas únicas extremamente longas são truncadas em vez de ignoradas

Use `mode = lines` quando:

* O agente lê principalmente arquivos de texto
* Você quer paginação por linhas nos prompts e nas chamadas de ferramentas
* Você quer trechos mais limpos para revisão de código, logs e documentação

#### Exemplo

```json
{
  "tools": {
    "read_file": {
      "enabled": true,
      "mode": "lines",
      "max_read_file_size": 65536
    }
  }
}
```

### Segurança do Exec

| Chave de config | Tipo | Padrão | Descrição |
|-----------------|------|--------|-----------|
| `tools.exec.allow_remote` | bool | `true` | Permite a ferramenta exec a partir de canais remotos (Telegram/Discord etc.). Defina `false` para limitá-la aos canais internos |
| `tools.exec.enable_deny_patterns` | bool | `true` | Habilita a interceptação de comandos perigosos |
| `tools.exec.custom_deny_patterns` | string[] | `[]` | Padrões regex personalizados a bloquear |
| `tools.exec.custom_allow_patterns` | string[] | `[]` | Padrões regex personalizados a permitir |

> **Rótulos de risco:** `exec` e os comandos shell do cron vêm ativados por padrão e cada um é nomeado na inicialização (`NeuroClaw risk=shell`, `NeuroClaw risk=cron_shell`). A instalação de skills e as ferramentas de barramento de hardware também têm rótulos. Veja [Capacidades §2](../reference/capabilities.md) para a lista, o que cada um permite ao modelo e como desativá-lo. Combine `exec` com um `allow_from` não vazio em cada canal ([Aplicativos de Chat](chat-apps.pt-br.md)).

> **Nota de Segurança:** A proteção contra symlinks é habilitada por padrão — todos os caminhos de arquivo são resolvidos através de `filepath.EvalSymlinks` antes da correspondência com a lista de permissão, prevenindo ataques de escape via symlink.

#### Limitação Conhecida: Processos Filhos de Ferramentas de Build

O guard de segurança do exec inspeciona apenas a linha de comando que o NeuroClaw executa diretamente. Ele não inspeciona recursivamente processos filhos gerados por ferramentas de desenvolvimento permitidas como `make`, `go run`, `cargo`, `npm run` ou scripts de build personalizados.

Isso significa que um comando de nível superior ainda pode compilar ou executar outros binários após passar pela verificação inicial do guard. Na prática, trate scripts de build, Makefiles, scripts de pacotes e binários gerados como código executável que precisa do mesmo nível de revisão que um comando shell direto.

Para ambientes de maior risco:

* Revise scripts de build antes da execução.
* Prefira aprovação/revisão manual para fluxos de trabalho de compilação e execução.
* Execute o NeuroClaw dentro de um contêiner ou VM se precisar de isolamento mais forte do que o guard integrado oferece.

#### Exemplos de Erro

```
[ERROR] tool: Tool execution failed
{tool=exec, error=Command blocked by safety guard (path outside working dir)}
```

```
[ERROR] tool: Tool execution failed
{tool=exec, error=Command blocked by safety guard (dangerous pattern detected)}
```

#### Desabilitando Restrições (Risco de Segurança)

Se você precisar que o agente acesse caminhos fora do workspace:

**Método 1: Arquivo de configuração**

```json
{
  "agents": {
    "defaults": {
      "restrict_to_workspace": false
    }
  }
}
```

**Método 2: Variável de ambiente**

```bash
export NEUROCLAW_AGENTS_DEFAULTS_RESTRICT_TO_WORKSPACE=false
```

> ⚠️ **Aviso**: Desabilitar esta restrição permite que o agente acesse qualquer caminho no seu sistema. Use com cautela apenas em ambientes controlados.

#### Consistência do Limite de Segurança

A configuração `restrict_to_workspace` se aplica consistentemente em todos os caminhos de execução:

| Caminho de Execução | Limite de Segurança          |
| -------------------- | ---------------------------- |
| Agente principal     | `restrict_to_workspace` ✅   |
| Subagente / Spawn    | Herda a mesma restrição ✅   |
| Tarefas de heartbeat | Herda a mesma restrição ✅   |

Todos os caminhos compartilham a mesma restrição de workspace — não há como contornar o limite de segurança através de subagentes ou tarefas agendadas.

### Heartbeat (Tarefas Periódicas)

O NeuroClaw pode executar tarefas periódicas automaticamente. Crie um arquivo `HEARTBEAT.md` no seu workspace:

```markdown
# Tarefas Periódicas

- Verificar meu e-mail para mensagens importantes
- Revisar meu calendário para eventos próximos
- Verificar a previsão do tempo
```

O agente lerá este arquivo a cada 30 minutos (configurável) e executará quaisquer tarefas usando as ferramentas disponíveis.

#### Tarefas Assíncronas com Spawn

Para tarefas de longa duração (busca na web, chamadas de API), use a ferramenta `spawn` para criar um **subagente**:

```markdown
# Tarefas Periódicas

## Tarefas Rápidas (responder diretamente)

- Informar a hora atual

## Tarefas Longas (usar spawn para assíncrono)

- Pesquisar notícias de IA na web e resumir
- Verificar e-mails e reportar mensagens importantes
```

**Comportamentos principais:**

| Funcionalidade   | Descrição                                                          |
| ---------------- | ------------------------------------------------------------------ |
| **spawn**        | Cria subagente assíncrono, não bloqueia o heartbeat                |
| **Contexto independente** | Subagente tem seu próprio contexto, sem histórico de sessão |
| **message tool** | Subagente comunica diretamente com o usuário via message tool      |
| **Não-bloqueante** | Após o spawn, o heartbeat continua para a próxima tarefa         |

#### Fluxo de Comunicação do Subagente

```
Heartbeat disparado
    ↓
Agent lê HEARTBEAT.md
    ↓
Tarefa longa: spawn subagente
    ↓                           ↓
Continua próxima tarefa    Subagente trabalha independentemente
    ↓                           ↓
Todas tarefas concluídas   Subagente usa ferramenta "message"
    ↓                           ↓
Responde HEARTBEAT_OK      Usuário recebe resultado diretamente
```

O subagente tem acesso às ferramentas (message, web_search etc.) e pode se comunicar com o usuário de forma independente, sem passar pelo agente principal.

**Configuração:**

```json
{
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

| Opção      | Padrão | Descrição                              |
| ---------- | ------ | -------------------------------------- |
| `enabled`  | `true` | Ativar/desativar heartbeat             |
| `interval` | `30`   | Intervalo de verificação em minutos (mínimo: 5)       |

**Variáveis de ambiente:**

* `NEUROCLAW_HEARTBEAT_ENABLED=false` para desativar
* `NEUROCLAW_HEARTBEAT_INTERVAL=60` para alterar o intervalo

### Providers

> [!NOTE]
> O Groq fornece transcrição de voz gratuita via Whisper. Se configurado, mensagens de áudio de qualquer canal serão automaticamente transcritas no nível do agente.

| Provider     | Finalidade                              | Obter API Key                                                |
| ------------ | --------------------------------------- | ------------------------------------------------------------ |
| `gemini`     | LLM (Gemini direto)                     | [aistudio.google.com](https://aistudio.google.com)           |
| `zhipu`      | LLM (Zhipu direto)                      | [bigmodel.cn](https://bigmodel.cn)                           |
| `volcengine` | LLM (Volcengine direto)                 | [volcengine.com](https://www.volcengine.com) |
| `openrouter` | LLM (recomendado, acesso a todos os modelos) | [openrouter.ai](https://openrouter.ai)                  |
| `anthropic`  | LLM (Claude direto)                     | [console.anthropic.com](https://console.anthropic.com)       |
| `openai`     | LLM (GPT direto)                        | [platform.openai.com](https://platform.openai.com)           |
| `deepseek`   | LLM (DeepSeek direto)                   | [platform.deepseek.com](https://platform.deepseek.com)       |
| `qwen`       | LLM (Qwen direto)                       | [dashscope.console.aliyun.com](https://dashscope.console.aliyun.com) |
| `groq`       | LLM + **Transcrição de voz** (Whisper)  | [console.groq.com](https://console.groq.com)                 |
| `cerebras`   | LLM (Cerebras direto)                   | [cerebras.ai](https://cerebras.ai)                           |
| `vivgrid`    | LLM (Vivgrid direto)                    | [vivgrid.com](https://vivgrid.com)                           |

### Configuração de Modelos (model_list)

> O NeuroClaw prefere a configuração explícita de `provider` + `model` nativo (por exemplo `"provider": "zhipu", "model": "glm-4.7"`). A forma legada de campo único `provider/model` continua suportada por compatibilidade quando `provider` é omitido.

Este design também permite **suporte multi-agente** com seleção flexível de provider:

- **Agentes diferentes, providers diferentes**: Cada agente pode usar seu próprio provider de LLM
- **Fallbacks de modelo**: Configure modelos primário e de fallback para maior resiliência
- **Balanceamento de carga**: Distribua requisições entre vários endpoints ou chaves
- **Configuração centralizada**: Gerencie todos os providers em um só lugar
- **Habilitar/desabilitar modelo**: Use o campo `enabled` para desabilitar temporariamente um modelo sem remover sua configuração

#### 🔒 Configuração de Segurança (Recomendado)

O NeuroClaw permite separar dados sensíveis (chaves de API, tokens, segredos) da configuração principal, armazenando-os em um arquivo `.security.yml`.

**Principais benefícios:**
- **Segurança**: Dados sensíveis nunca ficam no arquivo de configuração principal
- **Compartilhamento fácil**: Compartilhe o config.json sem expor chaves de API
- **Controle de versão**: Adicione `.security.yml` ao `.gitignore`
- **Implantação flexível**: Ambientes diferentes podem usar arquivos de segurança diferentes

**Configuração rápida:**

1. Crie `~/.neuroclaw/.security.yml` com suas chaves de API:
```yaml
model_list:
  gpt-5.4:
    api_keys:
      - "sk-proj-your-actual-openai-key"
  claude-sonnet-4.6:
    api_keys:
      - "sk-ant-your-actual-anthropic-key"
channel_list:
  telegram:
    token: "your-telegram-bot-token"
web:
  brave:
    api_keys:
      - "BSAyour-brave-api-key"
  glm_search:
    api_key: "your-glm-search-api-key"
```

2. Defina as permissões adequadas:
```bash
chmod 600 ~/.neuroclaw/.security.yml
```

3. Remova os campos sensíveis do `config.json` (recomendado):
```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4"
      // api_key loaded from .security.yml
    }
  ],
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      // token loaded from .security.yml
    }
  }
}
```

**Como funciona:**
- Os valores de `.security.yml` são mapeados automaticamente para os campos da configuração
- Nenhuma sintaxe especial é necessária — basta omitir os campos sensíveis do config.json
- Se um campo existir nos dois arquivos, o valor de `.security.yml` tem precedência
- Você pode misturar valores diretos no config.json com valores de segurança

Para a documentação completa, veja [`../security/security_configuration.md`](../security/security_configuration.md).

#### Todos os Vendors Suportados

| Vendor                  | Valor de `provider` | API Base padrão                                   | Protocolo | API Key                                                          |
| ----------------------- | ----------------- | --------------------------------------------------- | --------- | ---------------------------------------------------------------- |
| **OpenAI**              | `openai`          | `https://api.openai.com/v1`                         | OpenAI    | [Obter](https://platform.openai.com)                             |
| **Anthropic**           | `anthropic`       | `https://api.anthropic.com/v1`                      | Anthropic | [Obter](https://console.anthropic.com)                           |
| **智谱 AI (GLM)**       | `zhipu`           | `https://open.bigmodel.cn/api/paas/v4`              | OpenAI    | [Obter](https://open.bigmodel.cn/usercenter/proj-mgmt/apikeys)   |
| **DeepSeek**            | `deepseek`        | `https://api.deepseek.com/v1`                       | OpenAI    | [Obter](https://platform.deepseek.com)                           |
| **Google Gemini**       | `gemini`          | `https://generativelanguage.googleapis.com/v1beta`  | Gemini    | [Obter](https://aistudio.google.com/api-keys)                    |
| **Groq**                | `groq`            | `https://api.groq.com/openai/v1`                    | OpenAI    | [Obter](https://console.groq.com)                                |
| **Moonshot**            | `moonshot`        | `https://api.moonshot.cn/v1`                        | OpenAI    | [Obter](https://platform.moonshot.cn)                            |
| **通义千问 (Qwen)**     | `qwen`            | `https://dashscope.aliyuncs.com/compatible-mode/v1` | OpenAI    | [Obter](https://dashscope.console.aliyun.com)                    |
| **NVIDIA**              | `nvidia`          | `https://integrate.api.nvidia.com/v1`               | OpenAI    | [Obter](https://build.nvidia.com)                                |
| **Ollama**              | `ollama`          | `http://localhost:11434/v1`                         | OpenAI    | Local (sem chave)                                                |
| **LM Studio**           | `lmstudio`        | `http://localhost:1234/v1`                          | OpenAI    | Opcional (padrão local: sem chave)                               |
| **OpenRouter**          | `openrouter`      | `https://openrouter.ai/api/v1`                      | OpenAI    | [Obter](https://openrouter.ai/keys)                              |
| **LiteLLM Proxy**       | `litellm`         | `http://localhost:4000/v1`                          | OpenAI    | Sua chave do proxy LiteLLM                                       |
| **VLLM**                | `vllm`            | `http://localhost:8000/v1`                          | OpenAI    | Local                                                            |
| **Cerebras**            | `cerebras`        | `https://api.cerebras.ai/v1`                        | OpenAI    | [Obter](https://cerebras.ai)                                     |
| **VolcEngine (Doubao)** | `volcengine`      | `https://ark.cn-beijing.volces.com/api/v3`          | OpenAI    | [Obter](https://www.volcengine.com) |
| **神算云**              | `shengsuanyun`    | `https://router.shengsuanyun.com/api/v1`            | OpenAI    | —                                                                |
| **BytePlus**            | `byteplus`        | `https://ark.ap-southeast.bytepluses.com/api/v3`    | OpenAI    | [Obter](https://www.byteplus.com)                                |
| **Vivgrid**             | `vivgrid`         | `https://api.vivgrid.com/v1`                        | OpenAI    | [Obter](https://vivgrid.com)                                     |
| **LongCat**             | `longcat`         | `https://api.longcat.chat/openai`                   | OpenAI    | [Obter](https://longcat.chat/platform)                           |
| **ModelScope (魔搭)**   | `modelscope`      | `https://api-inference.modelscope.cn/v1`            | OpenAI    | [Obter token](https://modelscope.cn/my/tokens)                   |
| **Antigravity**         | `antigravity`     | Google Cloud                                        | Custom    | Somente OAuth                                                    |
| **GitHub Copilot**      | `github-copilot`  | `localhost:4321`                                    | gRPC      | —                                                                |

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

> **Nota de segurança**: Você pode remover os campos `api_keys` da sua configuração e armazená-los em `.security.yml`. Veja [Configuração de Segurança](#-configuração-de-segurança-recomendado) acima para detalhes.
>
> **Nota**: O campo `enabled` pode ser definido como `false` para desabilitar uma entrada de modelo sem removê-la. Quando omitido, o padrão é `true` durante a migração para modelos que têm chaves de API.

Regras de resolução:

- Prefira o explícito `"provider": "openai", "model": "gpt-5.4"`.
- Se `provider` estiver definido, o NeuroClaw envia `model` sem alterações.
- Se `provider` for omitido, o NeuroClaw trata o primeiro segmento antes de `/` em `model` como o provider e tudo depois dessa primeira `/` como o ID do modelo em runtime.
- Isso significa que `"model": "openrouter/openai/gpt-5.4"` continua funcionando como forma de compatibilidade e envia `openai/gpt-5.4` ao OpenRouter.

#### Configuração de Streaming

O streaming do provider usa double opt-in e fica desativado por padrão. O agent só tenta streaming quando o canal atual tem `settings.streaming.enabled: true`, a entrada de modelo ativa tem `streaming.enabled: true`, e tanto o provider quanto o canal suportam streaming. Se qualquer condição estiver ausente, o NeuroClaw usa o caminho normal de requisição sem streaming.

O Pico WebUI é o primeiro canal totalmente integrado. O Pico cria a primeira mensagem assistant com o wire message existente `message.create` e depois atualiza a mesma mensagem com `message.update`; nenhum novo tipo de wire message do Pico é introduzido.

Deixe `streaming` ausente quando não quiser streaming. Um bloco `streaming` omitido significa desativado; você não precisa escrever `"streaming": {"enabled": false}`.

Exemplo de ativação:

```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_keys": ["sk-your-openai-key"],
      "streaming": {
        "enabled": true
      }
    }
  ],
  "channel_list": {
    "pico": {
      "enabled": true,
      "type": "pico",
      "settings": {
        "token": "YOUR_PICO_TOKEN",
        "streaming": {
          "enabled": true
        }
      }
    }
  }
}
```

| Campo | Tipo | Padrão | Descrição |
| ----- | ---- | ------ | --------- |
| `channel_list.<name>.settings.streaming.enabled` | bool | `false` | Permite que este canal exiba output streaming do provider |
| `channel_list.<name>.settings.streaming.throttle_seconds` | int | Padrão do Pico após ativar: `0` | Intervalo mínimo entre atualizações intermediárias; o conteúdo final sempre é enviado |
| `channel_list.<name>.settings.streaming.min_growth_chars` | int | Padrão do Pico após ativar: `1` | Crescimento mínimo de texto antes de enviar outra atualização intermediária; o conteúdo final sempre é enviado |
| `model_list[].streaming.enabled` | bool | `false` | Permite que esta entrada de modelo tente requisições de provider streaming |

As variáveis de ambiente legadas do Telegram continuam compatíveis: `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_ENABLED`, `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_THROTTLE_SECONDS` e `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_MIN_GROWTH_CHARS`. Elas se aplicam apenas às settings do Telegram e não ativam nem modificam `settings.streaming` do Pico.

O comportamento de falha é intencionalmente conservador: se o streaming falhar antes de qualquer chunk visível ser enviado, o NeuroClaw tenta novamente uma vez pelo caminho normal `Chat()`. Se um chunk já foi mostrado ao usuário, o NeuroClaw não envia uma segunda resposta sem streaming, evitando output duplicado.

Para campos de requisição TTS específicos de um modelo, como nomes de `voice` personalizados de fala ou
`response_format: "mp3"`, use `model_list[].extra_body`.

#### Exemplos Específicos por Vendor

> **Dica**: Você pode omitir os campos `api_key` e armazená-los em `.security.yml` para maior segurança. Veja [Configuração de Segurança](#-configuração-de-segurança-recomendado).

<details>
<summary><b>OpenAI</b></summary>

```json
{
  "model_name": "gpt-5.4",
  "provider": "openai",
  "model": "gpt-5.4"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>OpenRouter TTS (MAI Voice 2)</b></summary>

```json
{
  "model_name": "mai-voice-2",
  "provider": "openrouter",
  "model": "microsoft/mai-voice-2",
  "api_base": "https://openrouter.ai/api/v1",
  "extra_body": {
    "voice": "en-US-Harper:MAI-Voice-2",
    "response_format": "mp3"
  }
  // api_key: set in .security.yml
}
```

Combine isto com:

```json
{
  "voice": {
    "tts_model_name": "mai-voice-2"
  }
}
```

</details>

<details>
<summary><b>VolcEngine (Doubao)</b></summary>

```json
{
  "model_name": "ark-code-latest",
  "provider": "volcengine",
  "model": "ark-code-latest"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>智谱 AI (GLM)</b></summary>

```json
{
  "model_name": "glm-4.7",
  "provider": "zhipu",
  "model": "glm-4.7"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>DeepSeek</b></summary>

```json
{
  "model_name": "deepseek-chat",
  "provider": "deepseek",
  "model": "deepseek-chat"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>Anthropic</b></summary>

```json
{
  "model_name": "claude-sonnet-4.6",
  "provider": "anthropic",
  "model": "claude-sonnet-4.6"
  // api_key: set in .security.yml
}
```

> Execute `neuroclaw auth login --provider anthropic` para colar o seu token de API.

Para acesso direto à API da Anthropic ou endpoints personalizados que só suportam o formato de mensagem nativo da Anthropic:

```json
{
  "model_name": "claude-opus-4-6",
  "provider": "anthropic-messages",
  "model": "claude-opus-4-6",
  "api_keys": ["sk-ant-your-key"],
  "api_base": "https://api.anthropic.com"
}
```

> Use `anthropic-messages` quando o endpoint exigir o formato nativo `/v1/messages` da Anthropic em vez do `/v1/chat/completions` compatível com OpenAI.

</details>

<details>
<summary><b>Ollama (local)</b></summary>

```json
{
  "model_name": "llama3",
  "provider": "ollama",
  "model": "llama3"
}
```

</details>

<details>
<summary><b>LM Studio (local)</b></summary>

```json
{
  "model_name": "lmstudio-local",
  "provider": "lmstudio",
  "model": "openai/gpt-oss-20b"
}
```

`api_base` tem como padrão `http://localhost:1234/v1`. A chave de API é opcional, a menos que o seu servidor LM Studio habilite autenticação.<br/>
Com `provider` explícito, o NeuroClaw envia `openai/gpt-oss-20b` sem alterações ao LM Studio. A forma legada de compatibilidade `"model": "lmstudio/openai/gpt-oss-20b"` ainda resolve para o mesmo ID de modelo upstream quando `provider` é omitido.

</details>

<details>
<summary><b>Proxy personalizado / LiteLLM</b></summary>

```json
{
  "model_name": "my-custom-model",
  "provider": "openai",
  "model": "custom-model",
  "api_base": "https://my-proxy.com/v1"
  // api_key: set in .security.yml
}
```

Com `provider` explícito, o NeuroClaw envia `model` sem alterações. Isso significa que `"provider": "litellm", "model": "lite-gpt4"` envia `lite-gpt4`, enquanto `"provider": "litellm", "model": "openai/gpt-4o"` envia `openai/gpt-4o`. As formas legadas de compatibilidade `litellm/lite-gpt4` e `litellm/openai/gpt-4o` ainda resolvem da mesma forma quando `provider` é omitido.

</details>

#### Balanceamento de Carga

Configure vários endpoints para o mesmo nome de modelo — o NeuroClaw fará round-robin entre eles automaticamente:

**Opção 1: Várias chaves de API no .security.yml (Recomendado)**

```yaml
# .security.yml
model_list:
  gpt-5.4:
    api_keys:
      - "sk-proj-key-1"
      - "sk-proj-key-2"
```

```json
// config.json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_base": "https://api.openai.com/v1"
      // api_keys loaded from .security.yml
    }
  ]
}
```

**Opção 2: Várias entradas de modelo**

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

#### Migração da Configuração Legada `providers`

A configuração antiga `providers` está **depreciada** e foi removida no V2. Configs V0/V1 existentes são migradas automaticamente. Veja [docs/migration/model-list-migration.md](../migration/model-list-migration.md) para o guia completo.

### Arquitetura de Providers

O NeuroClaw roteia providers por família de protocolo:

- **Compatível com OpenAI**: OpenRouter, Groq, Zhipu, endpoints estilo vLLM e a maioria dos outros.
- **Gemini nativo**: Google Gemini via os endpoints nativos `models/*:generateContent` e `models/*:streamGenerateContent`.
- **Anthropic**: Comportamento nativo da API Claude.
- **Codex/OAuth**: Rota de autenticação OAuth/token da OpenAI.

Isso mantém o runtime leve e faz com que novos backends compatíveis com OpenAI sejam, na maior parte, apenas uma operação de configuração (`api_base` + `api_keys`).

<details>
<summary><b>Zhipu (formato legado de providers)</b></summary>

```json
{
  "agents": {
    "defaults": {
      "workspace": "~/.neuroclaw/workspace",
      "model": "glm-4.7",
      "max_tokens": 8192,
      "temperature": 0.7,
      "max_tool_iterations": 20,
      "max_parallel_turns": 1
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

> **Nota**: O formato `providers` está depreciado. Use o novo formato `model_list` com `.security.yml` para maior segurança.
>
> **`max_parallel_turns`**: Controla o processamento concorrente de mensagens de sessões diferentes. `1` (padrão) = sequencial; `>1` = paralelo. Mensagens da mesma sessão são sempre serializadas. Veja a [documentação de Steering](../architecture/steering.md) para detalhes.

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
    "dm_scope": "per-channel-peer",
    "backlog_limit": 20
  },
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      // token: set in .security.yml
      "allow_from": ["123456789"]
    }
  },
  "tools": {
    "web": {
      "duckduckgo": {
        "enabled": true,
        "max_results": 5
      }
    }
  },
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

> **Nota**: Campos sensíveis (`api_key`, `token` etc.) podem ser omitidos e armazenados em `.security.yml` para maior segurança.

</details>

### Logs de Eventos

O NeuroClaw publica eventos de runtime do agente, dos canais, do gateway, do barramento de mensagens e do MCP. Por padrão, apenas os eventos `agent.*` são impressos; os demais ainda vão para o barramento de eventos de runtime, mas não para o log.

```json
{
  "events": {
    "logging": {
      "enabled": true,
      "include": ["agent.*"],
      "exclude": [],
      "min_severity": "info",
      "include_payload": false
    }
  }
}
```

Uma configuração comum:

```json
{
  "events": {
    "logging": {
      "include": ["*"],
      "exclude": ["agent.llm.delta"],
      "min_severity": "warn"
    }
  }
}
```

`include` / `exclude` aceitam nomes exatos de eventos ou padrões como `gateway.*` e `channel.lifecycle.*`. `include_payload` vem desligado por padrão, para que mensagens completas do usuário e argumentos de ferramentas fiquem fora do log; os eventos do agente registram campos de resumo (tamanhos, contagens, status).

Mais campos e exemplos: [Eventos de runtime e logs de eventos](../architecture/runtime-events.md).

### Tarefas Agendadas / Lembretes

O NeuroClaw suporta tarefas agendadas no estilo cron por meio da ferramenta `cron`. O agente pode criar, listar e cancelar lembretes ou tarefas recorrentes que disparam em horários específicos.

```json
{
  "tools": {
    "cron": {
      "enabled": true,
      "exec_timeout_minutes": 5,
      "allow_command": true,
      "command_allowed_remotes": []
    }
  }
}
```

As tarefas agendadas persistem após reinicializações e ficam armazenadas em `~/.neuroclaw/workspace/cron/`.

Tarefas cron de comando podem executar comandos shell (`allow_command` é `true` por padrão e é reportado na inicialização como `NeuroClaw risk=cron_shell`; veja [Capacidades §2](../reference/capabilities.md)). Por padrão, canais remotos não podem agendar tarefas de comando. Para permitir canais remotos específicos, defina `command_allowed_remotes` com entradas como `"telegram"` ou `"telegram:1234567890"`; use `"*"` apenas se todo canal não vazio deve ser permitido. O curinga `"*"` é potencialmente perigoso, pois qualquer canal remoto que possa falar com o NeuroClaw pode agendar comandos shell. Isso não contorna `allow_command` (quando é `false`, uma tarefa precisa de `command_confirm: true`) nem as verificações de segurança do exec.

### Tópicos Avançados

| Tópico | Descrição |
| ------ | --------- |
| [Configuração de Segurança](../security/security_configuration.md) | Armazene chaves de API e segredos em um arquivo `.security.yml` separado |
| [Filtragem de Dados Sensíveis](../security/sensitive_data_filtering.md) | Filtre chaves de API e tokens dos resultados de ferramentas antes de enviá-los ao LLM |
| [Sistema de Hooks](../architecture/hooks/README.md) | Hooks orientados a eventos: observadores, interceptores, hooks de aprovação |
| [Steering](../architecture/steering.md) | Injete mensagens em um loop de agente em execução entre chamadas de ferramentas |
| [SubTurn](../architecture/subturn.md) | Coordenação de subagentes, controle de concorrência, ciclo de vida |
| [Gerenciamento de Contexto](../architecture/agent-refactor/context.md) | Detecção de limites de contexto, verificação proativa de orçamento, compressão |
