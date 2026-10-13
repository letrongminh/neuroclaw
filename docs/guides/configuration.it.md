# ⚙️ Guida alla Configurazione

> Torna al [README](../README.md)

## ⚙️ Configurazione

File di configurazione: `~/.neuroclaw/config.json`

> **Configurazione di sicurezza:** per salvare API key, token e altri dati sensibili, vedi la [Guida alla Configurazione di Sicurezza](../security/security_configuration.md).

### Variabili d'Ambiente

Puoi sovrascrivere i percorsi predefiniti usando variabili d'ambiente. Questo è utile per installazioni portatili, distribuzioni containerizzate, o per eseguire neuroclaw come servizio di sistema. Queste variabili sono indipendenti e controllano percorsi diversi.

| Variabile         | Descrizione                                                                                                                             | Percorso Predefinito      |
|-------------------|-----------------------------------------------------------------------------------------------------------------------------------------|---------------------------|
| `NEUROCLAW_CONFIG` | Sovrascrive il percorso al file di configurazione. Indica direttamente a neuroclaw quale `config.json` caricare, ignorando tutte le altre posizioni. | `~/.neuroclaw/config.json` |
| `NEUROCLAW_HOME`   | Sovrascrive la directory radice per i dati di neuroclaw. Modifica la posizione predefinita del `workspace` e delle altre directory dati.  | `~/.neuroclaw`             |

**Esempi:**

```bash
# Esegui neuroclaw usando un file di configurazione specifico
# Il percorso del workspace verrà letto da quel file di configurazione
NEUROCLAW_CONFIG=/etc/neuroclaw/production.json neuroclaw gateway

# Esegui neuroclaw con tutti i dati salvati in /opt/neuroclaw
# La configurazione verrà caricata dal percorso predefinito ~/.neuroclaw/config.json
# Il workspace verrà creato in /opt/neuroclaw/workspace
NEUROCLAW_HOME=/opt/neuroclaw neuroclaw agent

# Usa entrambi per un setup completamente personalizzato
NEUROCLAW_HOME=/srv/neuroclaw NEUROCLAW_CONFIG=/srv/neuroclaw/main.json neuroclaw gateway
```

### Livello di Log del Gateway

`gateway.log_level` controlla la verbosità dei log del Gateway ed è configurabile in `config.json`.

```json
{
  "gateway": {
    "log_level": "warn"
  }
}
```

Se omesso, il valore predefinito è `warn`. Valori supportati: `debug`, `info`, `warn`, `error`, `fatal`.

Puoi anche sovrascriverlo con la variabile d'ambiente `NEUROCLAW_LOG_LEVEL`.

### Struttura del Workspace

NeuroClaw salva i dati nel workspace configurato (predefinito: `~/.neuroclaw/workspace`):

```
~/.neuroclaw/workspace/
├── sessions/          # Sessioni di conversazione e cronologia
├── memory/           # Memoria a lungo termine (MEMORY.md)
├── state/            # Stato persistente (ultimo canale, ecc.)
├── cron/             # Database dei job pianificati
├── skills/           # Skill personalizzate
├── AGENT.md          # Guida al comportamento dell'agent
├── HEARTBEAT.md      # Prompt per task periodici (controllato ogni 30 min)
├── SOUL.md           # Anima dell'agent
└── USER.md           # Preferenze dell'utente
```

> **Nota:** Le modifiche a `AGENT.md`, `SOUL.md`, `USER.md` e `memory/MEMORY.md` vengono rilevate automaticamente a runtime tramite il tracciamento della data di modifica (mtime). **Non è necessario riavviare il gateway** dopo aver modificato questi file — l'agent caricherà il nuovo contenuto alla prossima richiesta.

### Auto-Evoluzione dell'Agent

Il blocco `evolution` controlla il runtime di auto-evoluzione di NeuroClaw. Quando è attivo, l'agent registra i turni completati come learning record. Nelle modalità superiori può raggruppare pattern di successo ripetuti, generare bozze di skill e, facoltativamente, applicare le bozze accettate alle skill del workspace.

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

| Campo | Predefinito | Descrizione |
|-------|-------------|-------------|
| `enabled` | `false` | Abilita la cattura dei learning record per i turni completati dell'agent. I turni heartbeat vengono ignorati. |
| `mode` | `observe` | `observe` registra soltanto i dati. `draft` può generare bozze candidate di skill. `apply` può applicare le bozze accettate alle skill del workspace. |
| `state_dir` | `""` | Directory facoltativa per lo stato dell'evoluzione. Lasciala vuota per usare quella predefinita nel workspace. |
| `min_task_count` | `2` | Numero minimo di record di task correlati richiesto prima che un pattern sia idoneo alla generazione di bozze. |
| `min_success_ratio` | `0.7` | Rapporto minimo di successo per un cluster di task. Usa un valore maggiore di `0` e fino a `1`. |
| `cold_path_trigger` | `after_turn` | Esegue la generazione di bozze `after_turn`, con cadenza `scheduled`, oppure disattiva le esecuzioni automatiche del cold path se impostato su `manual`. Non esiste ancora un trigger manuale per l'utente. Si applica solo nelle modalità `draft` e `apply`. |
| `cold_path_times` | `[]` | Orari di esecuzione pianificati usati quando `cold_path_trigger` è `scheduled`, scritti come stringhe `HH:MM`. |

Usa prima `observe` se vuoi ispezionare i learning record senza generare modifiche alle skill. Usa `draft` quando vuoi che NeuroClaw prepari miglioramenti da rivedere. Usa `apply` solo se ti sta bene lasciare che le bozze accettate aggiornino le skill del workspace.

### Politica del Contesto della Richiesta

`turn_profile` è una politica facoltativa del contesto della richiesta sotto `agents.defaults.turn_profile`. Lascialo non impostato o imposta `"enabled": false` per mantenere il comportamento normale di NeuroClaw. Con `"enabled": true`, la stessa politica si applica a ogni nuovo turno.

Ogni blocco usa gli stessi valori di `mode`:

| Modalità | Significato |
| --- | --- |
| `default` | Mantiene il comportamento normale di NeuroClaw per quel blocco. I blocchi mancanti e i campi `mode` mancanti sono trattati come `default`. |
| `off` | Disattiva quel blocco per il turno. |
| `custom` | Usa una allow list. In questa versione `custom` è supportato solo per `skills` e `tools`; usarlo per `history` o `system_prompt` è un errore di validazione. |

Blocchi del profilo:

| Blocco | Cosa controlla |
| --- | --- |
| `history` | Se il turno legge la cronologia e il riepilogo della sessione precedente, scrive i messaggi user/assistant/tool, acquisisce il contesto ed esegue compattazione o riepilogo. |
| `system_prompt` | Se NeuroClaw inietta la sua identità predefinita, le istruzioni del workspace, la memoria, il contesto runtime e il riepilogo. I system prompt esterni della richiesta restano consentiti quando è `off`. |
| `skills` | Se il catalogo delle skill e il contenuto del prompt della skill attiva vengono caricati. `custom.allow` mantiene nel contesto del prompt solo i nomi di skill elencati. |
| `tools` | Quali tool richiamabili sono esposti al modello e consentiti in esecuzione. `custom.allow` mantiene solo i nomi di tool registrati elencati. |

Quando `system_prompt.mode` è `off`, i tool sono ancora visibili e non viene fornito alcun system prompt esterno, NeuroClaw usa la sua regola esistente di uso dei tool come prompt di fallback minimo. Se `tools.mode` è `off`, non viene aggiunto alcun prompt di fallback.

Esempio di politica web pulita:

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

### Dashboard del launcher web

**neuroclaw-launcher** serve un'interfaccia nel browser che richiede prima l'accesso con password. Al primo avvio apri `/launcher-setup` per creare la password della dashboard. Gli accessi manuali successivi usano `/launcher-login`.

- **File di configurazione**: stessa directory di `config.json` (o il file indicato da `NEUROCLAW_CONFIG`). Il file specifico del launcher è `launcher-config.json`.
- **Archiviazione della password**: sulle piattaforme supportate la password è salvata come hash bcrypt in `launcher-auth.db`. Sulle piattaforme dove l'archivio SQLite delle password non è disponibile, l'hash bcrypt è salvato in `launcher-config.json`.
- **Migrazione legacy**: i vecchi valori `launcher_token` vengono migrati una sola volta al login con password e rimossi dalla configurazione salvata del launcher.
- **Login automatico locale**: quando il launcher apre automaticamente un browser locale dopo l'avvio, usa un endpoint di bootstrap monouso, solo loopback, per impostare automaticamente il cookie di sessione.
- **Percorsi di autenticazione non supportati**: il login con token nell'URL (`?token=...`), `NEUROCLAW_LAUNCHER_TOKEN` e l'autenticazione della dashboard con `Authorization: Bearer` non sono più supportati.
- **Disconnessione**: usa **`POST /api/auth/logout`** con **`Content-Type: application/json`** (il body può essere `{}`). Non affidarti a un URL GET per il logout (schema sicuro contro CSRF).
- **Brute force**: **`POST /api/auth/login`** ha un **rate limit per IP del client al minuto** (HTTP 429 al superamento).
- **Durata della sessione**: il cookie di sessione HttpOnly dura circa **31 giorni** per default, ma le sessioni vengono invalidate quando il processo del launcher viene riavviato.

### Sorgenti delle Skill

Per impostazione predefinita, le skill vengono caricate da:

1. `~/.neuroclaw/workspace/skills` (workspace)
2. `~/.neuroclaw/skills` (globale)
3. `<current-working-directory>/skills` (builtin)

Per configurazioni avanzate/di test, puoi sovrascrivere la directory radice delle skill builtin con:

```bash
export NEUROCLAW_BUILTIN_SKILLS=/path/to/skills
```

### Usare le Skill dai Canali di Chat

Dopo aver installato le skill e configurato i server MCP, puoi ispezionarli e forzarne l'uso direttamente da un canale di chat:

- `/list skills` mostra i nomi delle skill installate disponibili per l'agent corrente.
- `/list mcp` mostra i server MCP configurati con lo stato enabled/deferred/connected.
- `/show mcp <server>` mostra i tool attivi esposti da un server MCP connesso.
- `/use <skill> <message>` forza una skill specifica per una singola richiesta.
- `/use <skill>` prepara quella skill per il tuo prossimo messaggio nella stessa sessione di chat.
- `/use clear` annulla un override di skill in sospeso creato da `/use <skill>`.
- `/btw <question>` pone subito una domanda a margine senza modificare la cronologia della sessione corrente. `/btw` è gestito come query senza tool e non entra nel normale flusso di esecuzione dei tool.

Esempi:

```text
/list skills
/list mcp
/show mcp github
/use git explain how to squash the last 3 commits
/btw remind me what we already decided about the deploy plan
/use italiapersonalfinance
dammi le ultime news
```

### Politica Unificata di Esecuzione dei Comandi

- I comandi slash generici vengono eseguiti tramite un unico percorso in `pkg/agent/loop.go` via `commands.Executor`.
- Gli adattatori dei canali non consumano più localmente i comandi generici; inoltrano il testo in entrata al percorso bus/agent. Telegram registra ancora automaticamente all'avvio i comandi supportati come `/start`, `/help`, `/show`, `/list`, `/use` e `/btw`.
- Un comando slash sconosciuto (ad esempio `/foo`) viene passato all'elaborazione LLM come se fosse un messaggio dell'utente.
- Un comando registrato ma non supportato sul canale corrente (ad esempio `/show` su WhatsApp) restituisce un errore esplicito all'utente e interrompe l'elaborazione.

### Isolamento delle Sessioni

L'ambito della sessione controlla quanta memoria è condivisa tra chat, utenti, thread e spazi.

- Usa `session.dimensions` per il valore predefinito globale.
- Usa `session_dimensions` su una regola di dispatch per una singola eccezione instradata.

Per ricette passo-passo e schemi di isolamento, vedi la [Guida alle Sessioni](session-guide.md).

### Routing

Il routing si configura tramite `agents.dispatch.rules`.

Ogni regola viene confrontata con il contesto in entrata normalizzato prodotto dai canali.
Le regole sono valutate dall'alto verso il basso. Vince la prima regola che corrisponde. Se nessuna
regola corrisponde, NeuroClaw ripiega sull'agent predefinito configurato.

Campi di match supportati:

* `channel`
* `account`
* `space`
* `chat`
* `topic`
* `sender`
* `mentioned`

I valori di match usano lo stesso vocabolario di ambito del sistema di sessioni:

* `space`: `workspace:t001`, `guild:123456`
* `chat`: `direct:user123`, `group:-100123`, `channel:c123`
* `topic`: `topic:42`
* `sender`: un identificatore normalizzato del mittente per la piattaforma

Le regole possono facoltativamente sovrascrivere il valore globale `session.dimensions`
tramite `session_dimensions`. Così routing e allocazione delle sessioni restano allineati
senza reintrodurre i vecchi formati `bindings` o `dm_scope`.

Esempio:

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

Nell'esempio sopra, la regola VIP deve comparire prima della regola più ampia del gruppo.
Poiché il routing è rigorosamente ordinato, le regole più specifiche vanno messe
prima e quelle di ripiego più ampie dopo.

Per esempi più completi di routing e di livelli di modello, vedi la [Guida al Routing](routing-guide.md).

### Allowlist dei Tool per Agent

La dichiarazione dei tool per-agent vive nel frontmatter di `AGENT.md`, non in `config.json`.

Se `tools` è omesso nel frontmatter, l'agent riceve il normale set globale dei tool abilitati. Se `tools` è presente, NeuroClaw registra per quell'agent solo i tool runtime elencati.

```md
---
name: Research Agent
description: Specialista per ricerca web e analisi approfondita.
tools: [read_file, write_file, web_search, web_fetch, message]
skills: [deep-research]
mcpServers: [web-index]
---

Sei l'agent di ricerca.
```

Note:

- È una allowlist reale, non un suggerimento per l'LLM.
- I nomi dei tool fanno match 1:1 con il nome runtime del tool.
- Se ti serve controllo preciso, usa i nomi runtime effettivi come `web_search`, `web_fetch`, `spawn`, `subagent`, `send_file`.
- Le dichiarazioni dei tool in `AGENT.md` sono usate dal runtime e dai tool, ma non vengono iniettate nel prompt di discovery.

### Discovery Multi-Agent (Automatica)

Quando un agent ha peer spawnabili, NeuroClaw inietta automaticamente nel suo system prompt un registry strutturato dei peer. Non serve una chiamata aggiuntiva a un tool `list_agents`.

Questa discovery serve soprattutto a rendere affidabile la delega tramite `spawn` con `agent_id` esplicito.

Ogni entry include:

| Campo | Significato |
|-------|-------------|
| `id` | ID stabile dell'agent |
| `name` | Nome identitario da `AGENT.md` frontmatter |
| `description` | Descrizione identitaria da `AGENT.md` frontmatter |

Dettagli importanti:

- La sezione include solo i peer che l'agent corrente può spawnare tramite `subagents.allow_agents`.
- L'agent corrente e i peer non spawnabili vengono omessi, così il modello non pianifica contro agent non disponibili.
- La discovery è volutamente leggera. Fornisce al modello solo l'identità necessaria per scegliere un peer: `id`, `name`, `description`.
- `config.json` resta il layer infrastrutturale: workspace, agent di default, routing e permessi di subagent. Questi permessi controllano anche la visibilità nella discovery.
- `AGENT.md` resta il layer di identità. Il codice runtime e i tool possono comunque usare `tools`, `skills`, `mcpServers` e `model` quando avviene la delega.

Forma dell'oggetto iniettato:

```json
{
  "agents": [
    {
      "id": "research",
      "name": "Research Agent",
      "description": "Specialista per investigazioni e lavoro web."
    }
  ]
}
```

In pratica, un agent generalista sceglie un peer in base alla descrizione del suo ruolo, poi chiama `spawn` con l'`agent_id` del peer. Il runtime risolve il resto.

### 🔒 Sandbox di Sicurezza

NeuroClaw esegue in un ambiente sandboxed per impostazione predefinita. L'agent può accedere solo ai file ed eseguire comandi all'interno del workspace configurato.

#### Configurazione Predefinita

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

| Opzione                 | Predefinito             | Descrizione                                          |
| ----------------------- | ----------------------- | ---------------------------------------------------- |
| `workspace`             | `~/.neuroclaw/workspace` | Directory di lavoro dell'agent                       |
| `restrict_to_workspace` | `true`                  | Limita l'accesso a file/comandi al workspace         |

#### Strumenti Protetti

Quando `restrict_to_workspace: true`, i seguenti strumenti sono in sandbox:

| Strumento     | Funzione                  | Restrizione                                          |
| ------------- | ------------------------- | ---------------------------------------------------- |
| `read_file`   | Legge file                | Solo file all'interno del workspace                  |
| `write_file`  | Scrive file               | Solo file all'interno del workspace                  |
| `list_dir`    | Elenca directory          | Solo directory all'interno del workspace             |
| `edit_file`   | Modifica file             | Solo file all'interno del workspace                  |
| `append_file` | Aggiunge ai file          | Solo file all'interno del workspace                  |
| `exec`        | Esegue comandi            | I percorsi dei comandi devono essere nel workspace   |

#### Protezione Exec Aggiuntiva

Anche con `restrict_to_workspace: false`, lo strumento `exec` blocca questi comandi pericolosi:

* `rm -rf`, `del /f`, `rmdir /s` — Cancellazione di massa
* `format`, `mkfs`, `diskpart` — Formattazione del disco
* `dd if=` — Imaging del disco
* Scrittura su `/dev/sd[a-z]` — Scritture dirette su disco
* `shutdown`, `reboot`, `poweroff` — Spegnimento del sistema
* Fork bomb `:(){ :|:& };:`
* Tra gli altri: `sudo`, `chmod`/`chown`, `kill`/`pkill`, `curl ... | sh`, `git push`, `docker run`, installazioni di pacchetti e command substitution (`$(...)`, backtick). L'elenco completo è `defaultDenyPatterns` in `pkg/tools/shell.go`.

### Controllo Accesso ai File

| Chiave di configurazione | Tipo | Predefinito | Descrizione |
|--------------------------|------|-------------|-------------|
| `tools.allow_read_paths` | string[] | `[]` | Percorsi aggiuntivi consentiti per la lettura al di fuori del workspace |
| `tools.allow_write_paths` | string[] | `[]` | Percorsi aggiuntivi consentiti per la scrittura al di fuori del workspace |
| `tools.message.media_enabled` | bool | `false` | Consente al tool `message` di allegare file multimediali locali tramite percorso. È separato da `tools.send_file.enabled`; abilitalo solo quando si vuole una consegna unificata di testo/media/didascalia. |

### Modalità di Lettura dei File

`read_file` ha due implementazioni mutuamente esclusive selezionate dalla configurazione. NeuroClaw ne registra esattamente una all'avvio:

| Chiave di configurazione | Tipo | Predefinito | Descrizione |
|--------------------------|------|-------------|-------------|
| `tools.read_file.enabled` | bool | `true` | Abilita il tool `read_file` |
| `tools.read_file.mode` | string | `bytes` | Seleziona l'implementazione di `read_file`: `bytes` o `lines` |
| `tools.read_file.max_read_file_size` | int | `65536` | Numero massimo di byte restituiti da `read_file` |

#### Modalità: `bytes`

Ottimizzata per file arbitrari e paginazione sicura per i dati binari.

Parametri:

* `path` (obbligatorio): percorso del file
* `offset` (facoltativo): offset di byte iniziale, predefinito `0`
* `length` (facoltativo): numero massimo di byte da leggere, predefinito `max_read_file_size`

Usa `bytes` quando:

* Potresti leggere file binari
* Vuoi una paginazione deterministica per intervalli di byte

#### Modalità: `lines`

Comportamento orientato al testo, ottimizzato per file sorgente, markdown, log e configurazioni. Il tool legge sequenzialmente riga per riga e si ferma quando raggiunge il budget di byte configurato.

Parametri:

* `path` (obbligatorio): percorso del file
* `start_line` (facoltativo): numero di riga iniziale, base 1 e inclusivo, predefinito `1`
* `max_lines` (facoltativo): numero massimo di righe da leggere, predefinito = tutte le righe rimanenti fino a EOF o al budget di byte

Note sul comportamento:

* I file che sembrano binari vengono rifiutati con l'indicazione di passare `read_file` a `mode = bytes`
* Le singole righe estremamente lunghe vengono troncate anziché saltate

Usa `mode = lines` quando:

* L'agent legge soprattutto file di testo
* Vuoi una paginazione basata sulle righe nei prompt e nelle chiamate ai tool
* Vuoi blocchi più puliti per code review, log e documentazione

#### Esempio

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

### Sicurezza Exec

| Chiave di configurazione | Tipo | Predefinito | Descrizione |
|--------------------------|------|-------------|-------------|
| `tools.exec.allow_remote` | bool | `true` | Consente lo strumento exec da canali remoti (Telegram/Discord ecc.). Imposta `false` per limitarlo ai canali interni |
| `tools.exec.enable_deny_patterns` | bool | `true` | Abilita l'intercettazione dei comandi pericolosi |
| `tools.exec.custom_deny_patterns` | string[] | `[]` | Pattern regex personalizzati da bloccare |
| `tools.exec.custom_allow_patterns` | string[] | `[]` | Pattern regex personalizzati da consentire |

> **Etichette di rischio:** `exec` e i comandi shell del cron sono attivi per default e ognuno è nominato all'avvio (`NeuroClaw risk=shell`, `NeuroClaw risk=cron_shell`). Anche l'installazione di skill e i tool del bus hardware hanno etichette. Vedi [Capabilities §2](../reference/capabilities.md) per l'elenco, cosa permette ciascuno al modello e come disattivarlo. Abbina `exec` a un `allow_from` non vuoto su ogni canale ([App di chat](chat-apps.md)).

> **Nota di sicurezza:** La protezione dei symlink è abilitata per impostazione predefinita — tutti i percorsi file vengono risolti tramite `filepath.EvalSymlinks` prima del confronto con la whitelist, prevenendo attacchi di escape tramite symlink.

#### Limitazione Nota: Processi Figlio degli Strumenti di Build

Il controllo di sicurezza exec ispeziona solo la riga di comando avviata direttamente da NeuroClaw. Non ispeziona ricorsivamente i processi figlio generati da strumenti di sviluppo consentiti come `make`, `go run`, `cargo`, `npm run` o script di build personalizzati.

Ciò significa che un comando di primo livello può comunque compilare o avviare altri binari dopo aver superato il controllo iniziale. In pratica, tratta gli script di build, i Makefile, gli script di pacchetti e i binari generati come codice eseguibile che richiede lo stesso livello di revisione di un comando shell diretto.

Per ambienti ad alto rischio:

* Esamina gli script di build prima dell'esecuzione.
* Preferisci l'approvazione/revisione manuale per i workflow di compilazione ed esecuzione.
* Esegui NeuroClaw in un container o VM se hai bisogno di un isolamento più forte di quello fornito dal controllo integrato.

#### Esempi di Errore

```
[ERROR] tool: Tool execution failed
{tool=exec, error=Command blocked by safety guard (path outside working dir)}
```

```
[ERROR] tool: Tool execution failed
{tool=exec, error=Command blocked by safety guard (dangerous pattern detected)}
```

#### Disabilitare le Restrizioni (Rischio di Sicurezza)

Se hai bisogno che l'agent acceda a percorsi al di fuori del workspace:

**Metodo 1: File di configurazione**

```json
{
  "agents": {
    "defaults": {
      "restrict_to_workspace": false
    }
  }
}
```

**Metodo 2: Variabile d'ambiente**

```bash
export NEUROCLAW_AGENTS_DEFAULTS_RESTRICT_TO_WORKSPACE=false
```

> ⚠️ **Attenzione**: Disabilitare questa restrizione consente all'agent di accedere a qualsiasi percorso sul tuo sistema. Usare con cautela solo in ambienti controllati.

#### Coerenza dei Confini di Sicurezza

L'impostazione `restrict_to_workspace` si applica in modo coerente a tutti i percorsi di esecuzione:

| Percorso di esecuzione | Confine di sicurezza              |
| ---------------------- | --------------------------------- |
| Main Agent             | `restrict_to_workspace` ✅        |
| Subagent / Spawn       | Eredita la stessa restrizione ✅  |
| Heartbeat tasks        | Eredita la stessa restrizione ✅  |

Tutti i percorsi condividono la stessa restrizione del workspace — non è possibile aggirare il confine di sicurezza tramite subagent o task pianificati.

### Heartbeat (Task Periodici)

NeuroClaw può eseguire task periodici automaticamente. Crea un file `HEARTBEAT.md` nel tuo workspace:

```markdown
# Periodic Tasks

- Check my email for important messages
- Review my calendar for upcoming events
- Check the weather forecast
```

L'agent leggerà questo file ogni 30 minuti (configurabile) ed eseguirà tutti i task usando gli strumenti disponibili.

#### Task Asincroni con Spawn

Per task di lunga durata (ricerca web, chiamate API), usa lo strumento `spawn` per creare un **subagent**:

```markdown
# Periodic Tasks

## Quick Tasks (respond directly)

- Report current time

## Long Tasks (use spawn for async)

- Search the web for AI news and summarize
- Check email and report important messages
```

**Comportamenti chiave:**

| Funzionalità            | Descrizione                                                          |
| ----------------------- | -------------------------------------------------------------------- |
| **spawn**               | Crea un subagent asincrono, non blocca l'heartbeat                   |
| **Contesto indipendente** | Il subagent ha un proprio contesto, senza cronologia di sessione   |
| **Tool message**        | Il subagent comunica direttamente con l'utente tramite il tool message |
| **Non bloccante**       | Dopo lo spawn, l'heartbeat passa al task successivo                  |

#### Come Funziona la Comunicazione del Subagent

```
Heartbeat triggers
    ↓
Agent reads HEARTBEAT.md
    ↓
For long task: spawn subagent
    ↓                           ↓
Continue to next task      Subagent works independently
    ↓                           ↓
All tasks done            Subagent uses "message" tool
    ↓                           ↓
Respond HEARTBEAT_OK      User receives result directly
```

Il subagent ha accesso agli strumenti (message, web_search, ecc.) e può comunicare con l'utente in modo indipendente, senza passare dall'agent principale.

**Configurazione:**

```json
{
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

| Opzione    | Predefinito | Descrizione                                   |
| ---------- | ----------- | --------------------------------------------- |
| `enabled`  | `true`      | Abilita/disabilita l'heartbeat                |
| `interval` | `30`        | Intervallo di controllo in minuti (min: 5)    |

**Variabili d'ambiente:**

* `NEUROCLAW_HEARTBEAT_ENABLED=false` per disabilitare
* `NEUROCLAW_HEARTBEAT_INTERVAL=60` per cambiare l'intervallo

### Provider

> [!NOTE]
> Groq offre la trascrizione vocale gratuita tramite Whisper. Se configurato, i messaggi audio di qualsiasi canale vengono trascritti automaticamente a livello di agent.

| Provider     | Scopo                                   | Ottieni la API Key                                           |
| ------------ | --------------------------------------- | ------------------------------------------------------------ |
| `gemini`     | LLM (Gemini diretto)                    | [aistudio.google.com](https://aistudio.google.com)           |
| `zhipu`      | LLM (Zhipu diretto)                     | [bigmodel.cn](https://bigmodel.cn)                           |
| `volcengine` | LLM (Volcengine diretto)                | [volcengine.com](https://www.volcengine.com) |
| `openrouter` | LLM (consigliato, accesso a tutti i modelli) | [openrouter.ai](https://openrouter.ai)                  |
| `anthropic`  | LLM (Claude diretto)                    | [console.anthropic.com](https://console.anthropic.com)       |
| `openai`     | LLM (GPT diretto)                       | [platform.openai.com](https://platform.openai.com)           |
| `deepseek`   | LLM (DeepSeek diretto)                  | [platform.deepseek.com](https://platform.deepseek.com)       |
| `qwen`       | LLM (Qwen diretto)                      | [dashscope.console.aliyun.com](https://dashscope.console.aliyun.com) |
| `groq`       | LLM + **Trascrizione vocale** (Whisper) | [console.groq.com](https://console.groq.com)                 |
| `cerebras`   | LLM (Cerebras diretto)                  | [cerebras.ai](https://cerebras.ai)                           |
| `vivgrid`    | LLM (Vivgrid diretto)                   | [vivgrid.com](https://vivgrid.com)                           |

### Configurazione dei Modelli (model_list)

> NeuroClaw preferisce la configurazione esplicita `provider` + `model` nativo (ad esempio `"provider": "zhipu", "model": "glm-4.7"`). La forma legacy a campo singolo `provider/model` resta supportata per compatibilità quando `provider` è omesso.

Questo design abilita anche il **supporto multi-agent** con una scelta flessibile del provider:

- **Agent diversi, provider diversi**: ogni agent può usare il proprio provider LLM
- **Fallback dei modelli**: configura modelli primari e di fallback per la resilienza
- **Bilanciamento del carico**: distribuisci le richieste su più endpoint o chiavi
- **Configurazione centralizzata**: gestisci tutti i provider in un unico posto
- **Abilitazione/disabilitazione dei modelli**: usa il campo `enabled` per disabilitare temporaneamente un modello senza rimuoverne la configurazione

#### 🔒 Configurazione di Sicurezza (Consigliata)

NeuroClaw supporta la separazione dei dati sensibili (API key, token, segreti) dalla configurazione principale, salvandoli in un file `.security.yml`.

**Vantaggi principali:**
- **Sicurezza**: i dati sensibili non sono mai nel file di configurazione principale
- **Condivisione semplice**: condividi config.json senza esporre le API key
- **Controllo di versione**: aggiungi `.security.yml` a `.gitignore`
- **Deploy flessibile**: ambienti diversi possono usare file di sicurezza diversi

**Configurazione rapida:**

1. Crea `~/.neuroclaw/.security.yml` con le tue API key:
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

2. Imposta i permessi corretti:
```bash
chmod 600 ~/.neuroclaw/.security.yml
```

3. Rimuovi i campi sensibili da `config.json` (consigliato):
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

**Come funziona:**
- I valori di `.security.yml` vengono mappati automaticamente sui campi della configurazione
- Non serve alcuna sintassi speciale: basta omettere i campi sensibili da config.json
- Se un campo esiste in entrambi i file, prevale il valore di `.security.yml`
- Puoi mescolare valori diretti in config.json con valori di sicurezza

Per la documentazione completa, vedi [`../security/security_configuration.md`](../security/security_configuration.md).

#### Tutti i Vendor Supportati

| Vendor                  | Valore `provider` | API Base predefinita                                | Protocollo | API Key                                                         |
| ----------------------- | ----------------- | --------------------------------------------------- | ---------- | --------------------------------------------------------------- |
| **OpenAI**              | `openai`          | `https://api.openai.com/v1`                         | OpenAI    | [Ottieni la chiave](https://platform.openai.com)                 |
| **Anthropic**           | `anthropic`       | `https://api.anthropic.com/v1`                      | Anthropic | [Ottieni la chiave](https://console.anthropic.com)               |
| **智谱 AI (GLM)**       | `zhipu`           | `https://open.bigmodel.cn/api/paas/v4`              | OpenAI    | [Ottieni la chiave](https://open.bigmodel.cn/usercenter/proj-mgmt/apikeys) |
| **DeepSeek**            | `deepseek`        | `https://api.deepseek.com/v1`                       | OpenAI    | [Ottieni la chiave](https://platform.deepseek.com)               |
| **Google Gemini**       | `gemini`          | `https://generativelanguage.googleapis.com/v1beta`  | Gemini    | [Ottieni la chiave](https://aistudio.google.com/api-keys)        |
| **Groq**                | `groq`            | `https://api.groq.com/openai/v1`                    | OpenAI    | [Ottieni la chiave](https://console.groq.com)                    |
| **Moonshot**            | `moonshot`        | `https://api.moonshot.cn/v1`                        | OpenAI    | [Ottieni la chiave](https://platform.moonshot.cn)                |
| **通义千问 (Qwen)**     | `qwen`            | `https://dashscope.aliyuncs.com/compatible-mode/v1` | OpenAI    | [Ottieni la chiave](https://dashscope.console.aliyun.com)        |
| **NVIDIA**              | `nvidia`          | `https://integrate.api.nvidia.com/v1`               | OpenAI    | [Ottieni la chiave](https://build.nvidia.com)                    |
| **Ollama**              | `ollama`          | `http://localhost:11434/v1`                         | OpenAI    | Locale (nessuna chiave necessaria)                               |
| **LM Studio**           | `lmstudio`        | `http://localhost:1234/v1`                          | OpenAI    | Facoltativa (predefinito locale: nessuna chiave)                 |
| **OpenRouter**          | `openrouter`      | `https://openrouter.ai/api/v1`                      | OpenAI    | [Ottieni la chiave](https://openrouter.ai/keys)                  |
| **LiteLLM Proxy**       | `litellm`         | `http://localhost:4000/v1`                          | OpenAI    | La tua chiave del proxy LiteLLM                                  |
| **VLLM**                | `vllm`            | `http://localhost:8000/v1`                          | OpenAI    | Locale                                                           |
| **Cerebras**            | `cerebras`        | `https://api.cerebras.ai/v1`                        | OpenAI    | [Ottieni la chiave](https://cerebras.ai)                         |
| **VolcEngine (Doubao)** | `volcengine`      | `https://ark.cn-beijing.volces.com/api/v3`          | OpenAI    | [Ottieni la chiave](https://www.volcengine.com)                  |
| **神算云**              | `shengsuanyun`    | `https://router.shengsuanyun.com/api/v1`            | OpenAI    | —                                                                |
| **BytePlus**            | `byteplus`        | `https://ark.ap-southeast.bytepluses.com/api/v3`    | OpenAI    | [Ottieni la chiave](https://www.byteplus.com)                    |
| **Vivgrid**             | `vivgrid`         | `https://api.vivgrid.com/v1`                        | OpenAI    | [Ottieni la chiave](https://vivgrid.com)                         |
| **LongCat**             | `longcat`         | `https://api.longcat.chat/openai`                   | OpenAI    | [Ottieni la chiave](https://longcat.chat/platform)               |
| **ModelScope (魔搭)**   | `modelscope`      | `https://api-inference.modelscope.cn/v1`            | OpenAI    | [Ottieni il token](https://modelscope.cn/my/tokens)              |
| **Antigravity**         | `antigravity`     | Google Cloud                                        | Custom    | Solo OAuth                                                       |
| **GitHub Copilot**      | `github-copilot`  | `localhost:4321`                                    | gRPC      | —                                                                |

#### Configurazione di Base

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

> **Nota di sicurezza**: puoi rimuovere i campi `api_keys` dalla configurazione e salvarli in `.security.yml`. Vedi [Configurazione di Sicurezza](#-configurazione-di-sicurezza-consigliata) sopra per i dettagli.
>
> **Nota**: il campo `enabled` può essere impostato su `false` per disabilitare una entry di modello senza rimuoverla. Se omesso, durante la migrazione vale `true` per i modelli che hanno API key.

Regole di risoluzione:

- Preferisci l'esplicito `"provider": "openai", "model": "gpt-5.4"`.
- Se `provider` è impostato, NeuroClaw invia `model` invariato.
- Se `provider` è omesso, NeuroClaw tratta il primo segmento `/` di `model` come provider e tutto ciò che segue quel primo `/` come ID del modello a runtime.
- Quindi `"model": "openrouter/openai/gpt-5.4"` funziona ancora come forma di compatibilità e invia `openai/gpt-5.4` a OpenRouter.

#### Configurazione Streaming

Lo streaming del provider usa un double opt-in ed è disattivato per impostazione predefinita. L'agent prova lo streaming solo quando il canale corrente ha `settings.streaming.enabled: true`, l'entry del modello attivo ha `streaming.enabled: true`, e sia il provider sia il canale supportano lo streaming. Se manca una qualsiasi condizione, NeuroClaw usa il normale percorso di richiesta non streaming.

Pico WebUI è il primo canale completamente collegato. Pico crea il primo messaggio assistant con il wire message esistente `message.create`, poi aggiorna lo stesso messaggio con `message.update`; non viene introdotto alcun nuovo tipo di wire message Pico.

Lascia `streaming` assente quando non vuoi usare lo streaming. Un blocco `streaming` omesso significa disattivato; non è necessario scrivere `"streaming": {"enabled": false}`.

Esempio di attivazione:

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

| Campo | Tipo | Predefinito | Descrizione |
| ----- | ---- | ----------- | ----------- |
| `channel_list.<name>.settings.streaming.enabled` | bool | `false` | Permette a questo canale di mostrare l'output streaming del provider |
| `channel_list.<name>.settings.streaming.throttle_seconds` | int | Predefinito Pico dopo l'attivazione: `0` | Intervallo minimo tra aggiornamenti intermedi; il contenuto finale viene sempre inviato |
| `channel_list.<name>.settings.streaming.min_growth_chars` | int | Predefinito Pico dopo l'attivazione: `1` | Crescita minima del testo prima di inviare un aggiornamento intermedio; il contenuto finale viene sempre inviato |
| `model_list[].streaming.enabled` | bool | `false` | Permette a questa entry di modello di provare richieste provider streaming |

Le variabili d'ambiente legacy di Telegram restano compatibili: `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_ENABLED`, `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_THROTTLE_SECONDS` e `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_MIN_GROWTH_CHARS`. Si applicano solo alle settings Telegram e non attivano né modificano `settings.streaming` di Pico.

Il comportamento in caso di errore è intenzionalmente conservativo: se lo streaming fallisce prima che venga inviato un chunk visibile, NeuroClaw riprova una volta tramite il normale percorso `Chat()`. Se un chunk è già stato mostrato all'utente, NeuroClaw non invia una seconda risposta non streaming, evitando output duplicato.

Per i campi di richiesta specifici del modello per il TTS, come nomi `voice` personalizzati per il parlato o
`response_format: "mp3"`, usa `model_list[].extra_body`.

#### Esempi Specifici per Vendor

> **Suggerimento**: puoi omettere i campi `api_key` e salvarli in `.security.yml` per maggiore sicurezza. Vedi [Configurazione di Sicurezza](#-configurazione-di-sicurezza-consigliata).

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

Abbinalo a:

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

> Esegui `neuroclaw auth login --provider anthropic` per incollare il tuo token API.

Per l'accesso diretto alla API Anthropic o per endpoint personalizzati che supportano solo il formato di messaggio nativo di Anthropic:

```json
{
  "model_name": "claude-opus-4-6",
  "provider": "anthropic-messages",
  "model": "claude-opus-4-6",
  "api_keys": ["sk-ant-your-key"],
  "api_base": "https://api.anthropic.com"
}
```

> Usa `anthropic-messages` quando l'endpoint richiede il formato nativo `/v1/messages` di Anthropic anziché `/v1/chat/completions` compatibile con OpenAI.

</details>

<details>
<summary><b>Ollama (locale)</b></summary>

```json
{
  "model_name": "llama3",
  "provider": "ollama",
  "model": "llama3"
}
```

</details>

<details>
<summary><b>LM Studio (locale)</b></summary>

```json
{
  "model_name": "lmstudio-local",
  "provider": "lmstudio",
  "model": "openai/gpt-oss-20b"
}
```

`api_base` ha come predefinito `http://localhost:1234/v1`. La API key è facoltativa, a meno che il tuo server LM Studio non abiliti l'autenticazione.<br/>
Con `provider` esplicito, NeuroClaw invia `openai/gpt-oss-20b` invariato a LM Studio. La forma di compatibilità legacy `"model": "lmstudio/openai/gpt-oss-20b"` si risolve comunque nello stesso ID di modello upstream quando `provider` è omesso.

</details>

<details>
<summary><b>Custom Proxy / LiteLLM</b></summary>

```json
{
  "model_name": "my-custom-model",
  "provider": "openai",
  "model": "custom-model",
  "api_base": "https://my-proxy.com/v1"
  // api_key: set in .security.yml
}
```

Con `provider` esplicito, NeuroClaw invia `model` invariato. Quindi `"provider": "litellm", "model": "lite-gpt4"` invia `lite-gpt4`, mentre `"provider": "litellm", "model": "openai/gpt-4o"` invia `openai/gpt-4o`. Le forme di compatibilità legacy `litellm/lite-gpt4` e `litellm/openai/gpt-4o` si risolvono allo stesso modo quando `provider` è omesso.

</details>

#### Bilanciamento del Carico

Configura più endpoint per lo stesso nome di modello — NeuroClaw farà automaticamente round-robin tra di essi:

**Opzione 1: più API key in .security.yml (consigliata)**

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

**Opzione 2: più entry di modello**

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

#### Migrazione dalla Configurazione Legacy `providers`

La vecchia configurazione `providers` è **deprecata** ed è stata rimossa nella V2. Le configurazioni V0/V1 esistenti vengono migrate automaticamente. Vedi [docs/migration/model-list-migration.md](../migration/model-list-migration.md) per la guida completa.

### Architettura dei Provider

NeuroClaw instrada i provider per famiglia di protocollo:

- **Compatibili con OpenAI**: OpenRouter, Groq, Zhipu, endpoint in stile vLLM e la maggior parte degli altri.
- **Gemini nativo**: Google Gemini tramite gli endpoint nativi `models/*:generateContent` e `models/*:streamGenerateContent`.
- **Anthropic**: comportamento della API nativa di Claude.
- **Codex/OAuth**: percorso di autenticazione OAuth/token di OpenAI.

Questo mantiene il runtime leggero e fa sì che aggiungere nuovi backend compatibili con OpenAI sia per lo più un'operazione di configurazione (`api_base` + `api_keys`).

<details>
<summary><b>Zhipu (formato legacy providers)</b></summary>

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

> **Nota**: il formato `providers` è deprecato. Usa il nuovo formato `model_list` con `.security.yml` per maggiore sicurezza.
>
> **`max_parallel_turns`**: controlla l'elaborazione concorrente dei messaggi di sessioni diverse. `1` (predefinito) = sequenziale; `>1` = parallelo. I messaggi della stessa sessione sono sempre serializzati. Vedi la [documentazione su Steering](../architecture/steering.md) per i dettagli.

</details>

<details>
<summary><b>Esempio di configurazione completa</b></summary>

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

> **Nota**: i campi sensibili (`api_key`, `token`, ecc.) possono essere omessi e salvati in `.security.yml` per maggiore sicurezza.

</details>

### Task Pianificati / Promemoria

NeuroClaw supporta task pianificati in stile cron tramite il tool `cron`. L'agent può impostare, elencare e annullare promemoria o job ricorrenti che scattano agli orari indicati.

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

I task pianificati persistono tra i riavvii e sono salvati in `~/.neuroclaw/workspace/cron/`.

I job cron di tipo comando possono eseguire comandi shell (`allow_command` è `true` per default e viene segnalato all'avvio come `NeuroClaw risk=cron_shell`; vedi [Capabilities §2](../reference/capabilities.md)). Per impostazione predefinita, i canali remoti non possono pianificare job di comando. Per consentire canali remoti specifici, imposta `command_allowed_remotes` con voci come `"telegram"` o `"telegram:1234567890"`; usa `"*"` solo se ogni canale non vuoto deve essere consentito. Il carattere jolly `"*"` è potenzialmente pericoloso perché qualsiasi canale remoto che può parlare con NeuroClaw può pianificare comandi shell. Questo non aggira `allow_command` (quando è `false`, un job richiede `command_confirm: true`) né i controlli di sicurezza di exec.

### Argomenti Avanzati

| Argomento | Descrizione |
| --------- | ----------- |
| [Configurazione di Sicurezza](../security/security_configuration.md) | Salva API key e segreti in un file `.security.yml` separato |
| [Filtraggio dei Dati Sensibili](../security/sensitive_data_filtering.md) | Filtra API key e token dai risultati dei tool prima di inviarli all'LLM |
| [Sistema di Hook](../architecture/hooks/README.md) | Hook guidati dagli eventi: observer, interceptor, hook di approvazione |
| [Steering](../architecture/steering.md) | Inietta messaggi in un loop agent in esecuzione tra una chiamata di tool e l'altra |
| [SubTurn](../architecture/subturn.md) | Coordinamento dei subagent, controllo della concorrenza, ciclo di vita |
| [Gestione del Contesto](../architecture/agent-refactor/context.md) | Rilevamento dei confini del contesto, controllo proattivo del budget, compressione |
