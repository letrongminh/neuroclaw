# NeuroClaw 🦞

Tudo o que o [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 faz — aplicativos de chat, provedores
de LLM, memória, lembretes, o shell, skills, MCP, o console web e o launcher — e o cérebro de uma casa
[NeuroEdge](https://github.com/letrongminh/neuroedge-init). Conectado a uma caixa doméstica NeuroEdge,
ele alcança os dispositivos da casa pelas ferramentas MCP da caixa, que decidem com as regras da casa e,
para tudo o que não pode ser desfeito, perguntam a você em casa primeiro. Tudo o que permite ao modelo ir
além do chat (um shell, barramentos de hardware, instalação de skills, hooks, servidores MCP stdio) fica
ligado ou desligado conforme a sua configuração, e cada item ligado é nomeado na inicialização (`risk=…`).

Rode em uma placa Linux, em um servidor (NeuroEdge Cloud), em um Mac ou em um PC Windows; as releases
cobrem as mesmas plataformas do PicoClaw. Instalação: [install.pt-br.md](../guides/install.pt-br.md).

Ou no próprio chip: [`esp32/`](../../esp32/) é o **NeuroClaw-ESP**, o mesmo cérebro reescrito em C como
componente ESP-IDF para um único ESP32-S3 — Telegram, o LLM (com fallback), memória, lembretes,
heartbeat, pesquisa web, mensagens e arquivos para o dono, e o gate em um chip de poucos dólares, com o
mesmo workspace e as mesmas ferramentas daqui ([o que é igual, o que não é](../../esp32/README.md)),
rotulado `risk=single_chip` porque, nesse caso, cérebro e gate compartilham o mesmo espaço de endereços.

O NeuroClaw é um fork rígido do [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 (MIT). Veja
[NOTICE](../../NOTICE).

## O que mudou em relação ao PicoClaw

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| MCP sobre HTTP | Apenas cabeçalhos | Também TLS mútuo: `tls.ca_file`, `cert_file`, `key_file`, `server_name`, TLS 1.3. Uma caixa NeuroEdge (`neuroedge mcp serve --http`) exige um certificado de cliente, então nenhuma ponte é necessária. |
| Shell, hardware, instalação de skills, hooks, MCP stdio | Ligados ou desligados por configuração | O mesmo, com os padrões do PicoClaw; cada item ligado é registrado na inicialização como `NeuroClaw risk=<name>` (`shell`, `cron_shell`, `hardware_bus`, `skill_install`, `process_hooks`, `usb_monitor`, `stdio_mcp`; [`pkg/config/risk.go`](../../pkg/config/risk.go)). Uma casa NeuroEdge que quer seu cérebro fechado diz isso no `config.json`. |
| Um gate que diz "pergunte ao dono" | Um resultado de ferramenta comum | Quando uma ferramenta da caixa retorna `BLOCK` com um `confirmation`, o NeuroClaw diz claramente ao modelo que nada foi executado, que só o dono em casa pode aprovar e que não deve tentar de novo ([`pkg/tools/integration/neuroedge_result.go`](../../pkg/tools/integration/neuroedge_result.go)). |
| `update` | Baixa as releases de `sipeed/picoclaw` | Baixa as releases deste repositório, conferidas com seus checksums |
| Nomes | `picoclaw`, `~/.picoclaw`, `PICOCLAW_*` | `neuroclaw`, `~/.neuroclaw`, `NEUROCLAW_*` |

Todo o resto é do PicoClaw: canais de chat (Telegram e outros), provedores de LLM, memória e sessões,
cron e heartbeat, pesquisa web, mensagens, skills embutidas, subagentes, o console web e o launcher
(`neuroclaw-launcher`), imagens Docker (`ghcr.io/letrongminh/neuroclaw`), builds noturnos, pacotes
deb/rpm e o DMG do macOS. Seus guias estão em [`docs/`](../) ([README original](PICOCLAW_README.md));
leia `picoclaw` como `neuroclaw` ali.

**[O que o NeuroClaw pode fazer](../reference/capabilities.md)** lista todas as capacidades dos dois
cérebros, o que vem ligado por padrão, o que significa cada rótulo de risco e o que se sabe estar errado.

## Conectar a uma caixa NeuroEdge

Guia: [neuroedge.pt-br.md](../guides/neuroedge.pt-br.md).

A caixa roda `neuroedge mcp serve --http --ui …`; as chaves vêm do `demo/i2f-chat-den-cua/tao-khoa.py`
do NeuroEdge. Em `~/.neuroclaw/config.json`:

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

`neuroclaw mcp test neuroedge` deve imprimir `reachable (3 tools)` para o agente de demonstração.

## Compilar

```bash
CGO_ENABLED=0 go build -tags goolm,stdjson -o neuroclaw ./cmd/neuroclaw
CGO_ENABLED=0 go test -tags goolm,stdjson ./...
```

`make build-launcher` compila o console web (`web/`, requer pnpm); `make build-all` e o goreleaser
(`.goreleaser.yaml`) compilam todas as plataformas. No macOS, três testes da ferramenta shell falham como
no PicoClaw v0.3.1; o CI roda no Linux, onde passam.

**Atualizando de um build do NeuroClaw anterior aos rótulos de risco** (2026-10-13): esses builds forçavam
o shell, os comandos cron, a instalação de skills e o monitor USB para desligado, e o `onboard` os salvava
desligados no `config.json`. Ligue-os lá, ou recomece dos padrões com `neuroclaw config reset`.

## Licença

MIT, como o PicoClaw. Copyright (c) 2026 PicoClaw contributors e NeuroClaw contributors.
