# NeuroClaw-ESP

NeuroClaw on one ESP32 chip: no Linux board, no cloud. The owner chats with the chip on Telegram; the chip
calls the LLM itself, remembers, keeps reminders, searches the web, and every action on the home goes to
the home's gate on the same chip. It is an ESP-IDF component (`components/neuroclaw/`, C, MIT) used by
NeuroEdge firmware ([NeuroEdge Q-79](https://github.com/letrongminh/neuroedge-init)).

The Go NeuroClaw needs about 10 MB of RAM and a Go runtime; an ESP32-S3 has 512 KB of SRAM and 8 MB of
PSRAM, and Go does not target Xtensa. So this is the same brain rewritten in C, with the same workspace,
the same tools and the same promises. It runs end to end in QEMU on the ESP32-S3 and builds for the
ESP32, ESP32-C3 and ESP32-C6 (`scripts/build_targets.sh`, CI job `esp32`).

## Same as the Go NeuroClaw

The full list of what both brains do is in [What NeuroClaw can do](../docs/reference/capabilities.md).

| | Go NeuroClaw (Linux, cloud) | NeuroClaw-ESP (one chip) |
|:---|:---|:---|
| Way to a device | MCP tools of the home box over mTLS | `nc_gate` callbacks into the firmware's gate (`ne_gate`) |
| Shell, GPIO, I2C, cron commands | As PicoClaw, by config, each labelled at startup (`risk=shell`…) | Do not exist; `cron` refuses `command` |
| Who is heard | The owner (`allow_from`) | The owner's Telegram user id; everyone else dropped before the model |
| The gate asks the owner | Model told it cannot approve | Same words; the chip sends the owner approve / refuse buttons, only the owner's press counts, once |
| Long-term memory | `memory/MEMORY.md`, in the prompt | Same file, same prompt, on LittleFS |
| Daily notes | `memory/YYYYMM/YYYYMMDD.md` | Same |
| Identity | `AGENT.md`, `SOUL.md`, `USER.md`, `IDENTITY.md` | Same files, same prompt |
| Skills | `skills/<name>/SKILL.md`, summarized in the prompt | Same |
| File tools | `read_file`, `write_file`, `append_file`, `edit_file`, `list_dir` | Same names, confined to the workspace (no `..`, 16 KB a file; `sessions/`, `cron/` are the brain's) |
| Conversation | Saved, older part summarized (`CONTEXT_SUMMARY`) | Saved after every turn, survives reboots; overflow folded by the LLM into `sessions/summary.md`, same prefix |
| Reminders | `cron` tool: `at_seconds`, `every_seconds`, `cron_expr`, add/list/get/update/remove/enable/disable | Same arguments; jobs in `cron/jobs.json`; a job runs the agent and the reply goes to the owner |
| Heartbeat | `HEARTBEAT.md`, every 30 min, `HEARTBEAT_OK` not sent | Same |
| Web | `web_search` (Brave, SearXNG, Tavily, …), `web_fetch` (private hosts refused) | `web_search` (SearXNG, Brave or Tavily, Tavily as the Go provider sends it), `web_fetch` (private hosts refused, redirects checked) |
| LLM protocol | OpenAI-compatible, Anthropic Messages, and more | Chat completions or the Anthropic Messages API, chosen per URL (`llm_api`, `llm_fallback_api`); tool calls and results converted both ways, as the Go `anthropic_messages` provider. A fallback in another format needs its own `llm_fallback_model` (else `NC_OFF fallback_model_required` and no fallback). A reply cut by `max_tokens` / `length` never runs its calls (`NC_LLM_ERROR … reason=truncated`); cut text is delivered and logged (`NC_LLM_TRUNCATED`) |
| Messages and files | `message` (the final reply is then not sent too), `send_file` (the turn ends when the file was the answer) | Same names, arguments and rules, to the owner only; files streamed from flash, images as photos (a document if Telegram refuses the photo) |
| A model that fails | `fallbacks`: the next model on network, auth, billing, rate-limit or overload errors, not on a bad request; cooldown 1, 5, 25 min, then 1 h (billing 5 h → 24 h) | `llm_fallback_url` (+ key, model, format): the same rule and the same cooldown |
| LLM call limits | `rate_limits` per model | Per hour and per day (`CONFIG_NEUROCLAW_LLM_PER_HOUR`, `_PER_DAY`, 0 = none; `neuroclaw_set_llm_limits` at run time). At the limit the owner is told plainly; a reminder still arrives, as written, with the reason |
| Health | `neuroclaw status`, logs | `get_diagnostics` (read-only): uptime, free heap and its low-water mark (counters; the heap is not walked), the brain's stack, Wi-Fi signal (when the app passes `wifi_rssi`), LLM calls, failures, cooldown, limits, the last error |
| Local administration | The CLI and web console | A C API and, when the app calls `neuroclaw_console_register`, serial commands: `memory_read`, `memory_write`, `session_list`, `session_clear`, `heartbeat_trigger`, `cron_start [job_id]`, `diag`, `llm_limit` |

## Not on the chip

| Go NeuroClaw | Why not here |
|:---|:---|
| Channels other than Telegram (Discord, Slack, Matrix…) | Each is a client and a TLS session in 512 KB of SRAM; Telegram is what the owner uses |
| LLM protocols other than chat completions and Anthropic Messages; more than one fallback | Two formats keep the code small; OpenRouter or a local server cover the rest |
| Subagents, `spawn`, MCP client | One task on one chip; devices come through the gate in-process, not MCP |
| Images and voice from the owner; speech out | No camera or audio pipeline on this board yet |
| Search providers other than SearXNG, Brave and Tavily | Kept to JSON APIs; DuckDuckGo's HTML is too big to parse on the chip |

## Risk, said plainly

Brain and gate share one chip and one address space: an exploitable bug in this code (TLS, JSON) reaches
the gate. NeuroEdge labels this mode `risk=single_chip`; the two Linux modes keep the brain in its own
process or machine. The workspace (memory, notes, conversation) is plaintext on flash unless the
firmware's flash encryption covers its partition. `web_fetch` checks every address a host resolves to
and every redirect, but the HTTP client resolves the name again when it connects, so DNS rebinding is
not caught.

## Use it

```c
#include "neuroclaw.h"

const nc_config config = {
    .telegram_token = token, .owner_id = "123456789",
    .llm_url = "https://openrouter.ai/api/v1/chat/completions", .llm_key = key,
    .llm_model = "openai/gpt-4o-mini",
    .workspace = "/ws",              /* a mounted LittleFS partition */
    .tz = "<+07>-7",
    .search_provider = "searxng", .search_url = "https://searx.example",   /* optional */
    .llm_fallback_url = "https://api.anthropic.com/v1/messages",          /* optional */
    .llm_fallback_key = anthropic_key, .llm_fallback_model = "claude-haiku-4-5",
    .llm_fallback_api = "anthropic",  /* each URL in its own format; NULL: "openai" */
    .wifi_rssi = my_rssi,             /* optional: the signal in get_diagnostics */
};
const nc_gate gate = {ctx, my_tools, my_call, my_answer}; /* the firmware's gate */
neuroclaw_start(&config, &gate);
neuroclaw_console_register();       /* optional: serial commands, after esp_console_init */
```

`examples/wifi` is the smallest app on a real board: Wi-Fi, LittleFS, an empty gate, `wifi_rssi` and
the console REPL. Kconfig (`NeuroClaw-ESP` menu): the brain's stack, its core
(`CONFIG_NEUROCLAW_TASK_CORE`, -1 = any; on a dual-core chip pin it away from the app), history, rounds,
heartbeat, LLM limits, `max_tokens` for the Messages API, file and HTTP sizes.

`nc_gate.call` returns what NeuroEdge's `tool-result.v1` carries: `ALLOW`, `BLOCK` (with a
`confirmation` when the owner may approve) or `REJECTED`. `nc_gate.answer` is called when the owner
presses a button; the gate checks the id and decides again.

The clock is set from Telegram's `Date` header when nothing else (SNTP) has set it; reminders wait for it.
The Telegram offset is saved before updates are handled, so after a crash a message is acted on at most
once.

Serial lines for tests and logs start with `NC_` (`NC_READY`, `NC_SESSION`, `NC_IGNORED`, `NC_TOOL`,
`NC_ASK`, `NC_BUTTON`, `NC_CRON_RUN`, `NC_HEARTBEAT`, `NC_SUMMARY`, `NC_FETCH`, `NC_MESSAGE`,
`NC_SEND_FILE`, `NC_LLM_FALLBACK`, `NC_LLM_LIMIT`, `NC_STACK`); none carries the token, the keys or what
the owner wrote. Replies to serial commands are `NC_CONSOLE <command> … result=<r>`: `ok`, `busy` (the brain stayed in a
turn for 2 minutes; nothing was done), `empty` (no memory yet), `not_started`, `bad_job_id`; `diag`
prints its JSON instead. `memory_read` prints the memory first, because the owner asked to see it.

## Test it

```bash
esp32/scripts/host_test.sh                       # decisions, on the host, ASan + UBSan

# in espressif/idf:v5.4 with qemu-xtensa installed:
cd esp32/examples/qemu && idf.py set-target esp32s3 build && cd ../../..
python3 esp32/scripts/qemu_e2e.py esp32/examples/qemu/build
esp32/scripts/build_targets.sh                   # examples/wifi for esp32, -s3, -c3, -c6
```

The QEMU example has a stub gate with the rules of NeuroEdge's chat-den-cua demo (light allowed; door
asks the owner) and talks to a fake Telegram, LLM and Tavily on the host; its UART is a socket the test
types serial commands into. Over two boots of one flash image: strangers dropped; the light; the door through the owner's button; memory written and read back;
the brain's own files refused; a reminder five seconds later; the conversation, memory and summary still
there after the reboot, no message handled twice; a file and a photo sent (a refused photo sent as a
document), a message to a stranger refused, a reminder delivered with `message` once; a web search; a
page fetched as text; the home's network refused, directly and behind a redirect; the configured LLM
down and the fallback answering in the Anthropic format (every request checked as the real API would:
roles alternate, each `tool_result` answers the `tool_use` before it), the first not called while it
cools down; the heartbeat; then on the console: `diag` and the model's `get_diagnostics`, the memory
read and replaced, a job and a heartbeat started by hand, an LLM limit the owner and a reminder are told
about plainly, the conversation listed and cleared.

## Compared with zclaw and MimiClaw

What NeuroClaw-ESP has and lacks next to the two ESP32 agents it learned from:
[What NeuroClaw can do, §12](../docs/reference/capabilities.md#12-neuroclaw-esp-the-same-brain-on-one-chip).

Telegram and chat-completions clients after [zclaw](https://github.com/tnm/zclaw) (MIT); workspace on
[LittleFS](https://github.com/joltwallet/esp_littlefs) (MIT); see [NOTICE](../NOTICE).
