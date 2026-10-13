# NeuroClaw 🦞

Tutto ciò che fa [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 — app di chat, provider LLM,
memoria, promemoria, la shell, skill, MCP, la console web e il launcher — e il cervello di una casa
[NeuroEdge](https://github.com/letrongminh/neuroedge-init). Collegato a un box NeuroEdge domestico,
raggiunge i dispositivi della casa tramite i tool MCP del box, che decidono con le regole della casa e,
per tutto ciò che non si può annullare, chiedono prima a te, a casa. Tutto ciò che permette al modello
di andare oltre la chat (una shell, bus hardware, installazione di skill, hook, server MCP stdio) è
attivo o disattivato a seconda della tua configurazione, e ognuno di quelli attivi viene nominato
all'avvio (`risk=…`).

Eseguilo su una scheda Linux, su un server (NeuroEdge Cloud), su un Mac o su un PC Windows; le release
coprono le stesse piattaforme di PicoClaw. Installazione: [docs/guides/install.md](../guides/install.md).

Oppure direttamente sul chip: [`esp32/`](../../esp32/) è **NeuroClaw-ESP**, lo stesso cervello riscritto
in C come componente ESP-IDF per un singolo ESP32-S3 — Telegram, l'LLM (con un fallback), memoria,
promemoria, heartbeat, ricerca web, messaggi e file al proprietario, e il gate su un chip da pochi
dollari, con lo stesso workspace e gli stessi tool di qui ([cosa è uguale, cosa no](../../esp32/README.md)),
etichettato `risk=single_chip` perché cervello e gate condividono allora un unico spazio di indirizzi.

NeuroClaw è un hard fork di [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 (MIT). Vedi
[NOTICE](../../NOTICE).

## Cosa è cambiato rispetto a PicoClaw

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| MCP su HTTP | Solo header | Anche mutual TLS: `tls.ca_file`, `cert_file`, `key_file`, `server_name`, TLS 1.3. Un box NeuroEdge (`neuroedge mcp serve --http`) richiede un certificato client, quindi non serve alcun bridge. |
| Shell, hardware, installazione di skill, hook, MCP stdio | Attivi o disattivati da config | Uguale, con i default di PicoClaw; ognuno di quelli attivi viene registrato all'avvio come `NeuroClaw risk=<name>` (`shell`, `cron_shell`, `hardware_bus`, `skill_install`, `process_hooks`, `usb_monitor`, `stdio_mcp`; [`pkg/config/risk.go`](../../pkg/config/risk.go)). Una casa NeuroEdge che vuole il proprio cervello chiuso lo dichiara in `config.json`. |
| Un gate che dice «chiedi al proprietario» | Un normale risultato di tool | Quando un tool del box restituisce `BLOCK` con una `confirmation`, NeuroClaw dice chiaramente al modello che nulla è stato eseguito, che solo il proprietario a casa può approvare e di non riprovare ([`pkg/tools/integration/neuroedge_result.go`](../../pkg/tools/integration/neuroedge_result.go)). |
| `update` | Scarica le release di `sipeed/picoclaw` | Scarica le release di questo repository, verificate con i loro checksum |
| Nomi | `picoclaw`, `~/.picoclaw`, `PICOCLAW_*` | `neuroclaw`, `~/.neuroclaw`, `NEUROCLAW_*` |

Tutto il resto è di PicoClaw: canali di chat (Telegram e altri), provider LLM, memoria e sessioni,
cron e heartbeat, ricerca web, messaggi, skill integrate, subagent, la console web e il launcher
(`neuroclaw-launcher`), immagini Docker (`ghcr.io/letrongminh/neuroclaw`), build notturne, pacchetti
deb/rpm e il DMG per macOS. Le sue guide sono in [`docs/`](../README.md)
([README originale](PICOCLAW_README.md)); lì leggi `picoclaw` come `neuroclaw`.

**[Cosa può fare NeuroClaw](../reference/capabilities.md)** elenca ogni capacità di entrambi i cervelli,
cosa è attivo per default, cosa significa ogni etichetta di rischio e cosa è noto per essere sbagliato.

## Collegarlo a un box NeuroEdge

Guida: [docs/guides/neuroedge.md](../guides/neuroedge.md).

Il box esegue `neuroedge mcp serve --http --ui …`; le sue chiavi provengono da
`demo/i2f-chat-den-cua/tao-khoa.py` di NeuroEdge. In `~/.neuroclaw/config.json`:

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

`neuroclaw mcp test neuroedge` dovrebbe stampare `reachable (3 tools)` per l'agente demo.

## Build

```bash
CGO_ENABLED=0 go build -tags goolm,stdjson -o neuroclaw ./cmd/neuroclaw
CGO_ENABLED=0 go test -tags goolm,stdjson ./...
```

`make build-launcher` compila la console web (`web/`, richiede pnpm); `make build-all` e goreleaser
(`.goreleaser.yaml`) compilano ogni piattaforma. Su macOS tre test del tool shell falliscono come in
PicoClaw v0.3.1; la CI gira su Linux, dove passano.

**Aggiornamento da una build di NeuroClaw precedente alle etichette di rischio** (2026-10-13): quelle
build forzavano disattivati la shell, i comandi cron, l'installazione di skill e il monitoraggio USB, e
`onboard` li salvava disattivati in `config.json`. Riattivali lì, oppure ricomincia dai default con
`neuroclaw config reset`.

## Licenza

MIT, come PicoClaw. Copyright (c) 2026 PicoClaw contributors e NeuroClaw contributors.
