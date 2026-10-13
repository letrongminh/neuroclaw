# NeuroClaw-ESP

NeuroClaw on one ESP32-S3: no Linux board, no cloud. The owner chats with the chip on Telegram; the chip
calls the LLM itself, remembers, keeps reminders, searches the web, and every action on the home goes to
the home's gate on the same chip. It is an ESP-IDF component (`components/neuroclaw/`, C, MIT) used by
NeuroEdge firmware ([NeuroEdge Q-79](https://github.com/letrongminh/neuroedge-init)).

The Go NeuroClaw needs about 10 MB of RAM and a Go runtime; an ESP32-S3 has 512 KB of SRAM and 8 MB of
PSRAM, and Go does not target Xtensa. So this is the same brain rewritten in C, with the same workspace,
the same tools and the same promises.

## Same as the Go NeuroClaw

The full list of what both brains do is in [What NeuroClaw can do](../docs/reference/capabilities.md).

| | Go NeuroClaw (Linux, cloud) | NeuroClaw-ESP (one chip) |
|:---|:---|:---|
| Way to a device | MCP tools of the home box over mTLS | `nc_gate` callbacks into the firmware's gate (`ne_gate`) |
| Shell, GPIO, I2C, cron commands | Off, whatever the config says | Do not exist; `cron` refuses `command` |
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
| Web | `web_search` (Brave, SearXNG, …), `web_fetch` (private hosts refused) | `web_search` (SearXNG or Brave), `web_fetch` (private hosts refused, redirects checked) |
| Messages and files | `message` (the final reply is then not sent too), `send_file` (the turn ends when the file was the answer) | Same names, arguments and rules, to the owner only; files streamed from flash, images as photos (a document if Telegram refuses the photo) |
| A model that fails | `fallbacks`: the next model on network, auth, billing, rate-limit or overload errors, not on a bad request; cooldown 1, 5, 25 min, then 1 h (billing 5 h → 24 h) | `llm_fallback_url` (+ key, model): the same rule and the same cooldown |

## Not on the chip

| Go NeuroClaw | Why not here |
|:---|:---|
| Channels other than Telegram (Discord, Slack, Matrix…) | Each is a client and a TLS session in 512 KB of SRAM; Telegram is what the owner uses |
| LLM providers other than OpenAI-compatible; more than one fallback | One protocol keeps the code small; OpenRouter or a local server cover the rest |
| Subagents, `spawn`, MCP client | One task on one chip; devices come through the gate in-process, not MCP |
| Images and voice from the owner; speech out | No camera or audio pipeline on this board yet |
| Search providers other than SearXNG and Brave | Kept to two JSON APIs; DuckDuckGo's HTML is too big to parse on the chip |

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
    .llm_fallback_url = "http://192.168.1.20:8080/v1/chat/completions",   /* optional */
    .llm_fallback_model = "qwen2.5-7b-instruct",
};
const nc_gate gate = {ctx, my_tools, my_call, my_answer}; /* the firmware's gate */
neuroclaw_start(&config, &gate);
```

`nc_gate.call` returns what NeuroEdge's `tool-result.v1` carries: `ALLOW`, `BLOCK` (with a
`confirmation` when the owner may approve) or `REJECTED`. `nc_gate.answer` is called when the owner
presses a button; the gate checks the id and decides again.

The clock is set from Telegram's `Date` header when nothing else (SNTP) has set it; reminders wait for it.
The Telegram offset is saved before updates are handled, so after a crash a message is acted on at most
once.

Serial lines for tests and logs start with `NC_` (`NC_READY`, `NC_SESSION`, `NC_IGNORED`, `NC_TOOL`,
`NC_ASK`, `NC_BUTTON`, `NC_CRON_RUN`, `NC_HEARTBEAT`, `NC_SUMMARY`, `NC_FETCH`, `NC_MESSAGE`,
`NC_SEND_FILE`, `NC_LLM_FALLBACK`, `NC_STACK`); none carries the token, the keys or what the owner wrote.

## Test it

```bash
esp32/scripts/host_test.sh                       # decisions, on the host, ASan + UBSan

# in espressif/idf:v5.4 with qemu-xtensa installed:
cd esp32/examples/qemu && idf.py set-target esp32s3 build && cd ../../..
python3 esp32/scripts/qemu_e2e.py esp32/examples/qemu/build
```

The QEMU example has a stub gate with the rules of NeuroEdge's chat-den-cua demo (light allowed; door
asks the owner) and talks to a fake Telegram, LLM and SearXNG on the host, over two boots of one flash
image: strangers dropped; the light; the door through the owner's button; memory written and read back;
the brain's own files refused; a reminder five seconds later; the conversation, memory and summary still
there after the reboot, no message handled twice; a file and a photo sent (a refused photo sent as a
document), a message to a stranger refused, a reminder delivered with `message` once; a web search; a
page fetched as text; the home's network refused, directly and behind a redirect; the configured LLM
down and the fallback answering, the first not called while it cools down; the heartbeat.

Telegram and chat-completions clients after [zclaw](https://github.com/tnm/zclaw) (MIT); workspace on
[LittleFS](https://github.com/joltwallet/esp_littlefs) (MIT); see [NOTICE](../NOTICE).
