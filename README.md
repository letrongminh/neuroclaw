# NeuroClaw 🦞

The brain of a [NeuroEdge](https://github.com/letrongminh/neuroedge-init) home. NeuroClaw chats
with you, calls an LLM, remembers across restarts, schedules reminders, searches the web and
uses its workspace, like any agent. It never touches a device or runs a command itself: the
only way to a device is the MCP tools of your NeuroEdge home box, which decides with its rules
and, for anything that cannot be undone, asks you at home first.

Run it on a cheap Linux board at home or on a server (NeuroEdge Cloud); it is the same binary.

Or on the chip itself: [`esp32/`](esp32/) is **NeuroClaw-ESP**, the same brain rewritten in C as an
ESP-IDF component for one ESP32-S3 — Telegram, the LLM, memory, reminders, heartbeat, web search and
the gate on a few-dollar chip, with the same workspace and tools as here ([what is the same, what is
not](esp32/README.md)), labelled
`risk=single_chip` because brain and gate then share one address space.

NeuroClaw is a hard fork of [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 (MIT). See
[NOTICE](NOTICE).

## What changed from PicoClaw

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| MCP over HTTP | Headers only | Mutual TLS too: `tls.ca_file`, `cert_file`, `key_file`, `server_name`, TLS 1.3. A NeuroEdge box (`neuroedge mcp serve --http`) requires a client certificate, so no bridge is needed. |
| Shell and hardware | On by default, off by config | Off, always: `exec`, `i2c`, `spi`, `serial`, `install_skill`, `find_skills`, shell commands in cron, process hooks, USB monitoring, skill registries and stdio MCP servers. Neither `config.json` nor `NEUROCLAW_*` variables can turn them on ([`pkg/config/lockdown.go`](pkg/config/lockdown.go)). |
| A gate that says "ask the owner" | An ordinary tool result | When a box tool returns `BLOCK` with a `confirmation`, NeuroClaw tells the model plainly that nothing ran, that only the owner at home can approve, and not to retry ([`pkg/tools/integration/neuroedge_result.go`](pkg/tools/integration/neuroedge_result.go)). |
| Web console | Included | Removed: NeuroClaw is configured by `config.json`. |
| `update` | Downloads `sipeed/picoclaw` releases | Downloads this repository's releases, checked against `SHA256SUMS` |
| Names | `picoclaw`, `~/.picoclaw`, `PICOCLAW_*` | `neuroclaw`, `~/.neuroclaw`, `NEUROCLAW_*` |

Everything else is PicoClaw's: chat channels (Telegram and more), LLM providers, memory and
sessions, cron and heartbeat, web search, messages, built-in skills, subagents. Its guides
are in [`docs/`](docs/) ([original README](docs/project/PICOCLAW_README.md)); read `picoclaw` as
`neuroclaw` there.

**[What NeuroClaw can do](docs/reference/capabilities.md)** lists every capability of both brains, what
is on by default, what the lockdown keeps off, and what is known to be wrong.

## Connect it to a NeuroEdge box

The box runs `neuroedge mcp serve --http --ui …`; its keys come from NeuroEdge's
`demo/i2f-chat-den-cua/tao-khoa.py`. In `~/.neuroclaw/config.json`:

```json
"tools": {
  "mcp": {
    "enabled": true,
    "servers": {
      "neuroedge": {
        "enabled": true,
        "type": "http",
        "url": "https://home-box.example:8443/mcp",
        "headers": { "Authorization": "Bearer <the brain's token>" },
        "tls": {
          "ca_file": "~/neuroedge-keys/ca.pem",
          "cert_file": "~/neuroedge-keys/nao.pem",
          "key_file": "~/neuroedge-keys/nao.key"
        }
      }
    }
  }
}
```

`neuroclaw mcp test neuroedge` should print `reachable (3 tools)` for the demo agent.

## Build

```bash
CGO_ENABLED=0 go build -tags goolm,stdjson -o neuroclaw ./cmd/neuroclaw
CGO_ENABLED=0 go test -tags goolm,stdjson ./...
```

On macOS three shell-tool tests fail as they do in PicoClaw v0.3.1; CI runs on Linux, where they
pass. The shell tool itself is never registered in NeuroClaw.

## License

MIT, as PicoClaw. Copyright (c) 2026 PicoClaw contributors and NeuroClaw contributors.
