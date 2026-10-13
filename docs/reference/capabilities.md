# What NeuroClaw can do

This is the list of what NeuroClaw does today, read from the code: what is on by default, what you can
turn on, and what each risky capability is labelled. It covers both brains:

- **NeuroClaw** (Go) — this repository's binary, on a Linux board, a server, a Mac or a Windows PC.
- **NeuroClaw-ESP** (C) — [`esp32/`](../../esp32/), the same brain as an ESP-IDF component on one ESP32-S3.

Where the two differ, section 12 says so. Section 13 lists what is known to be wrong or unfinished.
For how to configure each part, follow the links to the guides; this page says *what* exists, not
every key.

## 1. The promise

NeuroClaw can do everything PicoClaw v0.3.1 does, and it is the brain of a NeuroEdge home:

- The devices of a NeuroEdge home are reached through a tool of the home's gate: MCP tools of the
  NeuroEdge box (Go), or `nc_gate` callbacks into the firmware's gate (ESP). The system prompt tells
  the model never to go around them with other tools ([`pkg/agent/context.go`](../../pkg/agent/context.go)).
- When the gate answers `BLOCK` with a `confirmation`, the model is told that nothing ran, that only the
  owner at home can approve, that approval written in the chat does not count, and not to retry
  ([`pkg/tools/integration/neuroedge_result.go`](../../pkg/tools/integration/neuroedge_result.go)). A
  `BLOCK` with a reason, or `REJECTED`, is reported as "not done; do not work around it".
- Whatever lets the model reach beyond chat, its workspace and those tools is yours to turn on or off,
  and each one that is on is named at startup (section 2).

## 2. Risk labels

Nothing is locked: every PicoClaw capability is there, with PicoClaw's defaults. When `gateway` or
`agent` starts, [`pkg/config/risk.go`](../../pkg/config/risk.go) logs one warning
`NeuroClaw risk=<name>` for each of these that is on:

| Label | On when | What it lets the model do | Default |
|:---|:---|:---|:---|
| `shell` | `tools.exec.enabled` | Run shell commands on this machine | on |
| `cron_shell` | `tools.cron` with `allow_command`, and `exec` | Scheduled jobs run shell commands | on |
| `hardware_bus` | `tools.i2c`, `tools.spi`, `tools.serial` | Talk to buses and ports directly, not through a NeuroEdge gate | off |
| `skill_install` | `tools.skills` with `install_skill` or `find_skills` (the registries they use are listed) | Fetch skills (instructions it follows) from the internet | on |
| `process_hooks` | `hooks.enabled` and an enabled `hooks.processes.*` | External programs run on agent events | off |
| `usb_monitor` | `devices.enabled` and `devices.monitor_usb` | Watch and announce USB devices | off (`monitor_usb` on, `devices` off) |
| `stdio_mcp` | `tools.mcp.enabled` and an enabled stdio server | MCP servers run as programs on this machine | off |

The `claude-cli` and `codex-cli` LLM providers (section 4) also start a program, the `claude` or
`codex` CLI, when you configure one of them as a model.

**On a NeuroEdge home** the gate decides only what goes through it. A brain with `shell` or
`hardware_bus` on the same machine as the home box can reach devices around the gate unless it runs
as a user without device access; for a closed brain, turn those off in `config.json` (the NeuroEdge
demo `i2f-chat-den-cua` does).

Builds of NeuroClaw before the labels (2026-10-13) forced `shell`, `cron_shell`, `skill_install` and USB
monitoring off, and `onboard` saved them off: turn them on in `config.json`, or `neuroclaw config reset`.

## 3. Chat channels

Every channel is compiled into the default build ([`pkg/gateway/gateway.go`](../../pkg/gateway/gateway.go))
and every one is **disabled** until configured under `channel_list.<name>`. Setup: [Chat apps
guide](../guides/chat-apps.md), and `docs/channels/<name>/`.

| Channel | Notes |
|:---|:---|
| Telegram | Bot token, proxy, streaming, MarkdownV2, media groups, voice in |
| Discord | `mention_only`, voice in |
| Slack | Socket mode (bot + app token) |
| Matrix | End-to-end encryption store, mention-only and join-on-invite by default, voice in; not built for mipsle, netbsd, freebsd/arm, android |
| WhatsApp | Through a bridge (`ws://localhost:3001` by default) |
| WhatsApp native | Only in a build with `-tags whatsapp_native`; otherwise a stub |
| LINE | Webhook `/webhook/line`, voice in |
| Feishu / Lark, DingTalk, WeCom, Weixin (iLink), QQ, OneBot | Chinese platforms; Feishu, Weixin, QQ, OneBot take voice |
| VK, IRC (SASL, NickServ), MQTT | |
| Pico | NeuroClaw's own WebSocket at `/pico/`, token auth, streaming |
| MaixCAM | TCP, port 18790 by default |
| Teams webhook, Slack webhook | Outbound only |

Common keys for every channel:

| Key | What it does |
|:---|:---|
| `allow_from` | Who may talk (canonical `platform:id`, [`pkg/identity`](../../pkg/identity)) |
| `group_trigger` | `mention_only` or `prefixes` |
| `typing` | Typing indicator |
| `placeholder` | Placeholder message |
| `reasoning_channel_id` | Where the model's reasoning goes |

> **Set `allow_from`.** Empty means *anyone* who finds the bot may talk to it; NeuroClaw only logs a
> warning ([`pkg/channels/base.go`](../../pkg/channels/base.go)). For a home brain, put the owner's ID
> there.

Slash commands in any channel ([`pkg/commands`](../../pkg/commands)): `/help`, `/start`, `/stop`,
`/clear`, `/reload`, `/context`, `/btw`, `/use`, `/check channel`,
`/list models|channels|agents|skills|mcp`, `/show model|channel|agents|mcp`, `/switch model|channel`,
`/subagents`.

## 4. LLM providers and models

Supported protocols ([`pkg/providers/factory_provider.go`](../../pkg/providers/factory_provider.go)):

| Kind | Providers |
|:---|:---|
| OpenAI | OpenAI (API key or OAuth), Azure OpenAI |
| OpenAI-compatible | OpenRouter, Groq, DeepSeek, Mistral, Cerebras, NVIDIA, Venice, Moonshot, Qwen, Zhipu, SiliconFlow, Volcengine, ModelScope, Novita, LiteLLM, vLLM, Ollama, LM Studio and more; any other endpoint with `neuroclaw model add` |
| Anthropic | API key or OAuth / setup token; Messages API variant |
| Others | Gemini, MiniMax, AWS Bedrock, Antigravity (OAuth), GitHub Copilot |
| Local CLIs | `claude-cli`, `codex-cli` (start the `claude` / `codex` program; see section 2) |

Models are entries of `model_list`. Each entry has:

- provider, model, `api_base`, proxy;
- `api_keys` (several, tried in turn) and `fallbacks`;
- `rpm` (token-bucket rate limit), request timeout, thinking level, streaming, extra body and headers.

Model selection and resilience:

- `agents.defaults` picks the main model, its fallbacks and an image model.
- Optional routing sends easy turns to a lighter model (`routing.light_model`, threshold 0.35).
- A failing model cools down 1 min → 5 → 25 → 1 h. A billing error cools it down 5 h, doubling to 24 h.
- Calls are retried twice, 2 s apart.

More in the [providers guide](../guides/providers.md) and [rate limiting](rate-limiting.md).
`neuroclaw auth login -p openai|anthropic|antigravity` does OAuth sign-in.

## 5. Tools the model can call

Defaults from [`pkg/config/defaults.go`](../../pkg/config/defaults.go); configuration in
[tools configuration](tools_configuration.md).

| Tool | Default | What it does |
|:---|:---|:---|
| `read_file`, `write_file`, `edit_file`, `append_file`, `list_dir` | on | Files in the workspace only (`restrict_to_workspace`); `read_file` up to 64 KB |
| `web_search` | on | Search the web (below) |
| `web_fetch` | on | Read a page as text: up to 50 000 characters, 10 MB download, 5 redirects; **private and local addresses refused** unless in `tools.web.private_host_whitelist` |
| `message` | on | Send a message to a channel (media off by default) |
| `reaction` | on | React to a message |
| `send_file` | on | Send a workspace file to the chat |
| `load_image` | on | Look at an image |
| `send_tts` | off | Speak a reply; needs a TTS model |
| `skills` | on | Read a skill's instructions |
| `cron` | on | Reminders, scheduled agent runs and, with `exec` and `allow_command`, shell commands (section 8) |
| `spawn`, `subagent` | on | Run a task in a subagent, in the background or waiting for it ([spawn tasks](../guides/spawn-tasks.md)) |
| `spawn_status` | off | Ask how a spawned task is doing |
| `delegate` | when there is more than one agent | Hand a turn to another configured agent |
| MCP tools | off until a server is configured | Tools of remote MCP servers (section 6) |
| `tool_search_tool_bm25`, `tool_search_tool_regex` | off | Find deferred MCP tools when there are many |
| `short_grep`, `short_expand` | with the `seahorse` context manager | Search and expand old conversation |
| `exec` | on (`risk=shell`) | Run shell commands; `restrict_to_workspace` keeps the working directory in the workspace |
| `i2c`, `spi`, `serial` | off (`risk=hardware_bus`) | Talk to buses and serial ports on the board |
| `find_skills`, `install_skill` | on (`risk=skill_install`) | Search the skill registries and install a skill |

**Web search.** Providers are Sogou, DuckDuckGo, Brave, Tavily, Kagi, SearXNG, Perplexity, Gemini, GLM
and Baidu. Only Sogou is enabled by default, and it needs no key. With provider `auto`, NeuroClaw tries
in this order the providers that are enabled and configured:

1. Perplexity, Brave, Kagi, SearXNG, Tavily, Gemini;
2. Sogou or DuckDuckGo, picked by the query;
3. Baidu, GLM.

When the model has built-in search, that is preferred (`prefer_native`).

## 6. MCP and the NeuroEdge box

- MCP servers go under `tools.mcp.servers.<name>`. Transports are `stdio` (a program on this machine,
  `risk=stdio_mcp`; the default of `neuroclaw mcp add`), `http`, `streamable-http` and `sse`. Many tools can be `deferred`, found through the tool-search tools instead of
  sitting in every prompt.
- **Mutual TLS** (NeuroClaw only): `tls.ca_file`, `cert_file`, `key_file`, `server_name`
  ([`pkg/mcp/tls.go`](../../pkg/mcp/tls.go)). It requires `https` and TLS 1.3, and the certificate and
  key must come together. A NeuroEdge box (`neuroedge mcp serve --http`) asks for a client certificate.
  Example in the [README](../../README.md#connect-it-to-a-neuroedge-box).
- CLI: `neuroclaw mcp list|show|add|edit|test|remove` ([MCP CLI](mcp-cli.md)).
  `neuroclaw mcp test neuroedge` checks the box is reachable and counts its tools.

## 7. Memory, conversation and identity

The workspace (`~/.neuroclaw/workspace`) is the brain's memory. The model reads and writes it with the
file tools:

| File | Role |
|:---|:---|
| `AGENT.md` (or `AGENTS.md`) | How the agent works |
| `SOUL.md` | Its character |
| `USER.md` | What it knows about the owner |
| `IDENTITY.md` | Who it is |
| `memory/MEMORY.md` | Long-term memory, in every prompt |
| `memory/YYYYMM/YYYYMMDD.md` | Daily notes; the last three days are in the prompt |
| `skills/<name>/SKILL.md` | Skills (section 9) |
| `HEARTBEAT.md` | Standing checks (section 8) |
| `sessions/`, `cron/jobs.json`, `state/` | The brain's own state |

`neuroclaw onboard` writes the starting files from [`workspace/`](../../workspace/).

**Conversations** are kept per session as JSONL ([`pkg/memory/jsonl.go`](../../pkg/memory/jsonl.go))
and survive restarts. When a conversation grows past 20 messages, or past 75% of the context window
(4 × `max_tokens`, 32 768 by default), the older part is summarized by the model. The summary is put
back into the prompt as `CONTEXT_SUMMARY:`. If the context still overflows, the oldest half is dropped.
See the [session guide](../guides/session-guide.md).

**Seahorse** (optional, `agents.defaults.context_manager: "seahorse"`) keeps layered summaries in
SQLite with full-text search (`sessions/seahorse.db`). It gives the model `short_grep` and
`short_expand` to recall old turns. It is not built for mipsle, netbsd or freebsd/arm.

**Evolution** (off by default; modes `observe`, `draft`, `apply`) watches repeated workflows and drafts
or applies new *skills* under `state/evolution`. It improves skills, not chat memory.

## 8. Reminders and heartbeat

**Cron** ([`pkg/cron`](../../pkg/cron), [reference](cron.md)):

- The model's `cron` tool supports `add`, `list`, `get`, `update`, `remove`, `enable` and `disable`.
- A job's schedule is one of `at_seconds` (once), `every_seconds` or `cron_expr` (with time zone).
- A job runs an agent turn, and its reply goes to the channel and chat it names. A once-only job is
  removed after it runs.
- Jobs are stored in `cron/jobs.json`.
- The `neuroclaw cron` CLI does the same: `list`, `add -n -m -e|-c --to --channel`, `remove`,
  `enable`, `disable`.
- A job can run a shell command (`command`) when `exec` and `tools.cron.allow_command` are on
  (`risk=cron_shell`), as in PicoClaw.

**Heartbeat** ([`pkg/heartbeat`](../../pkg/heartbeat)):

- It is on by default and runs every 30 minutes (at least 5).
- Each time, the agent reads `HEARTBEAT.md` (created from a template if missing) and does the checks
  listed there.
- An answer of `HEARTBEAT_OK` means nothing to report and is not sent. Anything else goes to the last
  active chat.

## 9. Skills

A skill is a folder with `SKILL.md`. The catalog of skill names and descriptions is in the system
prompt, and the model reads a skill with the `skills` tool.

Skills are loaded from three places. When two have the same name, the earlier place wins:

1. the workspace (`skills/`);
2. `~/.neuroclaw/skills`;
3. the built-in skills (`$NEUROCLAW_BUILTIN_SKILLS`, or `./skills`).

To manage them: `neuroclaw skills list|show|remove|list-builtin|install-builtin`. Registries
(`search`, `install`; ClawHub and GitHub, on by default, `risk=skill_install`) fetch skills from the internet.

Shipped in [`workspace/skills`](../../workspace/skills): `weather`, `skill-creator`, `summarize`,
`github`, `tmux`, `agent-browser`, `hardware`, `picoclaw-agent`. See section 13: most of them need
tools that NeuroClaw locks.

## 10. Voice, images and files

| What | How |
|:---|:---|
| Voice in | Speech-to-text with Whisper-compatible models, ElevenLabs or an audio-capable model (`voice.model_name`, or picked from `model_list`); on Telegram, Discord, Matrix, QQ, Weixin, LINE, Feishu and OneBot |
| Voice out | `send_tts` with OpenAI or MiMo speech (`voice.tts_model_name`); off by default |
| Images | `load_image`, and an `image_model` for pictures sent in chat |
| Files | `send_file`, incoming media up to 20 MB, kept in a media store cleaned every 5 minutes (files older than 30 minutes are removed) |

## 11. Running it

**CLI:**

| Command | What it does |
|:---|:---|
| `onboard [--enc]` | Create config and workspace; `--enc` encrypts stored credentials |
| `agent [-m msg] [-s session] [--model m]` | Chat in the terminal, or one message |
| `gateway [-d] [--host]` | The long-running service: channels, cron, heartbeat |
| `status`, `version` | |
| `auth login\|logout\|status\|models\|weixin\|wecom` | Sign-in for providers and QR-code channels |
| `model [name]`, `model add` | Show or set the default model; add an OpenAI-compatible endpoint |
| `cron …`, `mcp …`, `skills …` | Sections 6, 8, 9 |
| `migrate --from openclaw` | Import an OpenClaw config and workspace |
| `config reset` | Back to defaults |
| `update` | Replace the binary with the latest release of this repository, checked against its checksums; `-u` for another release URL |
| `neuroclaw-launcher` | The web console: configure, chat, channels, skills, logs, in a browser (`-console`, `-public`, `-no-browser`) |

**Configuration:**

- The config file is `~/.neuroclaw/config.json`. `NEUROCLAW_HOME` and `NEUROCLAW_CONFIG` move it.
- Any key can be set with a `NEUROCLAW_*` variable.
- Secrets can live in `.security.yml` next to `config.json`.
- `onboard --enc` stores secrets encrypted (`enc://`, AES-256-GCM, passphrase in
  `NEUROCLAW_KEY_PASSPHRASE`).
- Without a config file, NeuroClaw starts on PicoClaw's defaults.
- [Configuration guide](../guides/configuration.md).

**Gateway HTTP** (default `localhost:18790`):

- `/health` and `/ready`;
- `POST /reload`, with the bearer token written to the PID file;
- the webhooks of the channels that use them (`/pico/`, `/webhook/line`).

**Isolation** ([`pkg/isolation`](../../pkg/isolation)) is opt-in. It sandboxes child processes only
(bwrap on Linux, restricted token on Windows), not NeuroClaw itself. It covers `exec`, cron commands, process
hooks, stdio MCP servers and the `claude-cli` / `codex-cli` providers.

## 12. NeuroClaw-ESP: the same brain on one chip

NeuroClaw-ESP is a rewrite in C, not a port: the Go runtime does not target Xtensa, and an ESP32-S3 has
512 KB of SRAM. What it keeps is the workspace format, the tool names and arguments, and the promises.
Details and the risk of sharing one chip with the gate: [`esp32/README.md`](../../esp32/README.md).

| | NeuroClaw (Go) | NeuroClaw-ESP |
|:---|:---|:---|
| Channel | 20 channels | Telegram only, owner's user ID only; everyone else dropped before the model |
| LLM | Many protocols, fallbacks, cooldown | One OpenAI-compatible chat-completions URL and an optional fallback URL, with the Go rules for when to fall back and the same cooldown |
| Way to devices | MCP over mTLS to the box | `nc_gate` callbacks into the firmware's gate, same `ALLOW` / `BLOCK` / `REJECTED` |
| Owner approval | Owner approves at the box | Same guidance text; the chip sends approve / refuse buttons, only the owner's press counts, once |
| File tools | Five, workspace only | Same five, workspace only, 16 KB a file; `sessions/` and `cron/` not writable by the model |
| Memory and identity files | Section 7 | Same files, same prompt, on LittleFS |
| Conversation | JSONL, summarized past a threshold | `sessions/telegram.json` after every turn (16 messages kept); overflow folded by the LLM into `sessions/summary.md`, same `CONTEXT_SUMMARY` prefix |
| Cron | Section 8 | Same arguments, `cron/jobs.json`, at most 16 jobs; waits until the clock is set (SNTP, or Telegram's `Date` header) |
| Heartbeat | 30 min | 30 min (`CONFIG_NEUROCLAW_HEARTBEAT_MIN`, 0 = off) |
| `web_search` | Ten providers | SearXNG or Brave |
| `web_fetch` | Private addresses refused | Same rule; every resolved address and every redirect (at most 3) checked; 6000 bytes of text |
| `message`, `send_file` | Any channel; media off by default in `message` | Same names and rules (final reply not sent after `message`; a turn whose round only delivered files ends there), owner only; files streamed from flash up to `CONFIG_NEUROCLAW_SEND_FILE_MAX` (10 MB), images as photos |
| Rounds per message | Configurable | `CONFIG_NEUROCLAW_MAX_ROUNDS`, 4 |
| After a crash | | Telegram offset saved before handling: a message is acted on at most once |
| Not there | | Other channels, other providers, more than one fallback, subagents, MCP client, voice, images from the owner, `reaction`, `load_image`, skill registries |

Tested on the host (ASan, UBSan) and end to end in QEMU over two boots of one flash image
(`esp32/scripts/`, CI job `esp32`).

## 13. Known gaps

What the code does today that does not match the promise or the name. None is being changed for now;
know them when you configure a brain:

| Gap | Where | Effect |
|:---|:---|:---|
| Empty `allow_from` lets anyone talk | `pkg/channels/base.go` | Only a warning is logged; set the owner's ID |
| `picoclaw-agent` skill | `workspace/skills` | Still describes PicoClaw |
| PicoClaw names left over | help text, banner, MQTT topic `/picoclaw`, IRC nick, links to docs.picoclaw.io | Cosmetic |
| WeCom variables are not prefixed | `pkg/config/config.go` (`BOT_ID`, `SECRET`) | Can collide with other programs' variables |
