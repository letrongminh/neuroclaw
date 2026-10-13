# NeuroClaw 🦞

Tout ce que fait [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 (applications de chat, fournisseurs de LLM,
mémoire, rappels, shell, skills, MCP, console web et lanceur), plus le cerveau d'une maison
[NeuroEdge](https://github.com/letrongminh/neuroedge-init). Relié à un boîtier NeuroEdge, il atteint
les appareils de la maison par les outils MCP du boîtier, qui décident selon les règles de la maison
et, pour tout ce qui ne peut pas être annulé, vous demandent d'abord confirmation à la maison. Tout ce
qui permet au modèle d'aller au-delà du chat (un shell, des bus matériels, l'installation de skills,
des hooks, des serveurs MCP stdio) est activé ou désactivé selon votre configuration, et chaque
fonction activée est nommée au démarrage (`risk=…`).

Il tourne sur une carte Linux, un serveur (NeuroEdge Cloud), un Mac ou un PC Windows ; les versions
couvrent les mêmes plateformes que PicoClaw. Installation : [docs/guides/install.md](../guides/install.md).

Ou directement sur la puce : [`esp32/`](../../esp32/) contient **NeuroClaw-ESP**, le même cerveau réécrit en C
comme composant ESP-IDF pour un seul ESP32-S3 : Telegram, le LLM (avec un repli), la mémoire, les rappels,
le heartbeat, la recherche web, les messages et fichiers envoyés au propriétaire, et la porte (gate) sur une
puce de quelques dollars, avec le même espace de travail et les mêmes outils qu'ici
([ce qui est identique, ce qui ne l'est pas](../../esp32/README.md)), étiqueté
`risk=single_chip` car le cerveau et la porte partagent alors un même espace d'adressage.

NeuroClaw est un fork complet de [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 (MIT). Voir
[NOTICE](../../NOTICE).

## Ce qui change par rapport à PicoClaw

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| MCP via HTTP | En-têtes uniquement | TLS mutuel aussi : `tls.ca_file`, `cert_file`, `key_file`, `server_name`, TLS 1.3. Un boîtier NeuroEdge (`neuroedge mcp serve --http`) exige un certificat client, donc aucun pont n'est nécessaire. |
| Shell, matériel, installation de skills, hooks, MCP stdio | Activés ou désactivés par la config | Pareil, avec les valeurs par défaut de PicoClaw ; chaque fonction activée est journalisée au démarrage sous la forme `NeuroClaw risk=<nom>` (`shell`, `cron_shell`, `hardware_bus`, `skill_install`, `process_hooks`, `usb_monitor`, `stdio_mcp` ; [`pkg/config/risk.go`](../../pkg/config/risk.go)). Une maison NeuroEdge qui veut un cerveau fermé l'indique dans `config.json`. |
| Une porte qui dit « demander au propriétaire » | Un résultat d'outil ordinaire | Quand un outil du boîtier renvoie `BLOCK` avec une `confirmation`, NeuroClaw dit clairement au modèle que rien n'a été exécuté, que seul le propriétaire à la maison peut approuver, et de ne pas réessayer ([`pkg/tools/integration/neuroedge_result.go`](../../pkg/tools/integration/neuroedge_result.go)). |
| `update` | Télécharge les versions de `sipeed/picoclaw` | Télécharge les versions de ce dépôt, vérifiées par leurs sommes de contrôle |
| Noms | `picoclaw`, `~/.picoclaw`, `PICOCLAW_*` | `neuroclaw`, `~/.neuroclaw`, `NEUROCLAW_*` |

Tout le reste vient de PicoClaw : canaux de chat (Telegram et d'autres), fournisseurs de LLM, mémoire et
sessions, cron et heartbeat, recherche web, messages, skills intégrés, sous-agents, console web et
lanceur (`neuroclaw-launcher`), images Docker (`ghcr.io/letrongminh/neuroclaw`), builds nocturnes,
paquets deb/rpm et DMG macOS. Ses guides
sont dans [`docs/`](../) ([README d'origine](PICOCLAW_README.md)) ; lisez `picoclaw` comme
`neuroclaw` dans ces pages.

**[Ce que NeuroClaw sait faire](../reference/capabilities.md)** liste chaque capacité des deux cerveaux, ce qui
est activé par défaut, ce que signifie chaque étiquette de risque, et ce qui est connu comme défaillant.

## Le relier à un boîtier NeuroEdge

Guide : [docs/guides/neuroedge.md](../guides/neuroedge.md).

Le boîtier exécute `neuroedge mcp serve --http --ui …` ; ses clés viennent de
`demo/i2f-chat-den-cua/tao-khoa.py` dans NeuroEdge. Dans `~/.neuroclaw/config.json` :

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

`neuroclaw mcp test neuroedge` doit afficher `reachable (3 tools)` pour l'agent de démonstration.

## Compiler

```bash
CGO_ENABLED=0 go build -tags goolm,stdjson -o neuroclaw ./cmd/neuroclaw
CGO_ENABLED=0 go test -tags goolm,stdjson ./...
```

`make build-launcher` compile la console web (`web/`, nécessite pnpm) ; `make build-all` et goreleaser
(`.goreleaser.yaml`) compilent toutes les plateformes. Sous macOS, trois tests de l'outil shell échouent comme
dans PicoClaw v0.3.1 ; la CI tourne sous Linux, où ils passent.

**Mise à jour depuis une version de NeuroClaw antérieure aux étiquettes de risque** (2026-10-13) : ces versions
forçaient la désactivation du shell, des commandes cron, de l'installation de skills et de la surveillance USB,
et `onboard` les enregistrait désactivés dans `config.json`. Activez-les à cet endroit, ou repartez des valeurs
par défaut avec `neuroclaw config reset`.

## Licence

MIT, comme PicoClaw. Copyright (c) 2026 PicoClaw contributors et NeuroClaw contributors.
