# NeuroClaw 🦞

Everything [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 does — chat apps, LLM providers,
memory, reminders, the shell, skills, MCP, the web console and launcher — and the brain of a
[NeuroEdge](https://github.com/letrongminh/neuroedge-init) home. Connected to a NeuroEdge home box,
it reaches the home's devices through the box's MCP tools, which decide with the home's rules and,
for anything that cannot be undone, ask you at home first. Whatever lets the model reach beyond
chat (a shell, hardware buses, skill installs, hooks, stdio MCP servers) is on or off as you
configure it, and each one that is on is named at startup (`risk=…`).

Run it on a Linux board, a server (NeuroEdge Cloud), a Mac or a Windows PC; releases cover the same
platforms as PicoClaw.

Or on the chip itself: [`esp32/`](esp32/) is **NeuroClaw-ESP**, the same brain rewritten in C as an
ESP-IDF component for one ESP32-S3 — Telegram, the LLM (with a fallback), memory, reminders,
heartbeat, web search, messages and files to the owner, and the gate on a few-dollar chip, with the
same workspace and tools as here ([what is the same, what is not](esp32/README.md)), labelled
`risk=single_chip` because brain and gate then share one address space.

NeuroClaw is a hard fork of [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 (MIT). See
[NOTICE](NOTICE).

## What changed from PicoClaw

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| MCP over HTTP | Headers only | Mutual TLS too: `tls.ca_file`, `cert_file`, `key_file`, `server_name`, TLS 1.3. A NeuroEdge box (`neuroedge mcp serve --http`) requires a client certificate, so no bridge is needed. |
| Shell, hardware, skill installs, hooks, stdio MCP | On or off by config | The same, with PicoClaw's defaults; each one that is on is logged at startup as `NeuroClaw risk=<name>` (`shell`, `cron_shell`, `hardware_bus`, `skill_install`, `process_hooks`, `usb_monitor`, `stdio_mcp`; [`pkg/config/risk.go`](pkg/config/risk.go)). A NeuroEdge home that wants its brain closed says so in `config.json`. |
| A gate that says "ask the owner" | An ordinary tool result | When a box tool returns `BLOCK` with a `confirmation`, NeuroClaw tells the model plainly that nothing ran, that only the owner at home can approve, and not to retry ([`pkg/tools/integration/neuroedge_result.go`](pkg/tools/integration/neuroedge_result.go)). |
| `update` | Downloads `sipeed/picoclaw` releases | Downloads this repository's releases, checked against their checksums |
| Names | `picoclaw`, `~/.picoclaw`, `PICOCLAW_*` | `neuroclaw`, `~/.neuroclaw`, `NEUROCLAW_*` |

Everything else is PicoClaw's: chat channels (Telegram and more), LLM providers, memory and
sessions, cron and heartbeat, web search, messages, built-in skills, subagents, the web console and
launcher (`neuroclaw-launcher`), Docker images (`ghcr.io/letrongminh/neuroclaw`), nightly builds,
deb/rpm packages and the macOS DMG. Its guides
are in [`docs/`](docs/) ([original README](docs/project/PICOCLAW_README.md)); read `picoclaw` as
`neuroclaw` there.

**[What NeuroClaw can do](docs/reference/capabilities.md)** lists every capability of both brains, what
is on by default, what each risk label means, and what is known to be wrong.

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

`make build-launcher` builds the web console (`web/`, needs pnpm); `make build-all` and goreleaser
(`.goreleaser.yaml`) build every platform. On macOS three shell-tool tests fail as they do in
PicoClaw v0.3.1; CI runs on Linux, where they pass.

**Upgrading from a NeuroClaw build before the risk labels** (2026-10-13): those builds forced the
shell, cron commands, skill installs and USB monitoring off and `onboard` saved them off in
`config.json`. Turn them on there, or start again from the defaults with `neuroclaw config reset`.

## License

MIT, as PicoClaw. Copyright (c) 2026 PicoClaw contributors and NeuroClaw contributors.
