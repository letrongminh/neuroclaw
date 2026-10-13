# NeuroClaw-ESP

NeuroClaw on one ESP32-S3: no Linux board, no cloud. The owner chats with the chip on Telegram; the chip
calls the LLM itself, and every action the model asks for goes to the home's gate on the same chip.
It is an ESP-IDF component (`components/neuroclaw/`, C, MIT) used by NeuroEdge firmware
([NeuroEdge Q-79](https://github.com/letrongminh/neuroedge-init)).

The Go NeuroClaw needs about 10 MB of RAM and a Go runtime; an ESP32-S3 has 512 KB of SRAM and 8 MB of
PSRAM, and Go does not target Xtensa. So this is a rewrite in C of what matters on a chip, keeping
NeuroClaw's promises:

| | Go NeuroClaw (Linux, cloud) | NeuroClaw-ESP (one chip) |
|:---|:---|:---|
| Way to a device | MCP tools of the home box over mTLS | `nc_gate` callbacks into the firmware's gate (`ne_gate`) |
| Shell, GPIO, I2C… | Off, whatever the config says | Do not exist: the model sees only the gate's tools |
| Who is heard | `allow_from` of the channel | The owner's Telegram user id; everyone else dropped before the model |
| The gate asks the owner | The model is told it cannot approve; the owner answers on the page at home | Same words; the chip sends the owner approve / refuse buttons, and only the owner's press counts, once |
| Memory, cron, skills, web search | Yes | Conversation in RAM only; no cron, skills or search yet |

**Risk, said plainly.** Brain and gate share one chip and one address space: an exploitable bug in this
code (TLS, JSON) reaches the gate. NeuroEdge labels this mode `risk=single_chip`; the two Linux modes
keep the brain in its own process or machine.

## Use it

```c
#include "neuroclaw.h"

const nc_config config = {
    .telegram_token = token, .owner_id = "123456789",
    .llm_url = "https://openrouter.ai/api/v1/chat/completions", .llm_key = key, .llm_model = "openai/gpt-4o-mini",
};
const nc_gate gate = {ctx, my_tools, my_call, my_answer}; /* the firmware's gate */
neuroclaw_start(&config, &gate);
```

`nc_gate.call` returns what NeuroEdge's `tool-result.v1` carries: `ALLOW`, `BLOCK` (with a
`confirmation` when the owner may approve) or `REJECTED`. `nc_gate.answer` is called when the owner
presses a button; the gate checks the id and decides again.

Serial lines for tests and logs start with `NC_` (`NC_READY`, `NC_IGNORED`, `NC_TOOL`, `NC_ASK`,
`NC_BUTTON`, `NC_REPLY`); none carries the token, the key or what the owner wrote.

## Test it

```bash
esp32/scripts/host_test.sh                       # decisions, on the host, ASan + UBSan

# in espressif/idf:v5.4 with qemu-xtensa installed:
cd esp32/examples/qemu && idf.py set-target esp32s3 build && cd ../../..
python3 esp32/scripts/qemu_e2e.py esp32/examples/qemu/build
```

The QEMU example has a stub gate with the rules of NeuroEdge's chat-den-cua demo (light allowed; door
asks the owner) and talks to a fake Telegram and a fake LLM on the host: a stranger is dropped, the light
turns on, the door waits for the owner's button, a stranger's press is dropped, the owner's press opens
it once.

Telegram and chat-completions clients after [zclaw](https://github.com/tnm/zclaw) (MIT); see
[NOTICE](../NOTICE).
