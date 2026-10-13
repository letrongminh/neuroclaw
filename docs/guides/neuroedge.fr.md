# NeuroClaw avec NeuroEdge

> Retour aux [Guides](README.md)

Un boîtier de maison [NeuroEdge](https://github.com/letrongminh/neuroedge-init) pilote les appareils de la maison derrière une porte (gate). NeuroClaw en est le cerveau : il lit votre chat, réfléchit et appelle les outils du boîtier via MCP. Le boîtier décide de chaque appel selon les règles de la maison et, pour tout ce qui ne peut pas être annulé, demande au propriétaire à la maison.

## Se connecter au boîtier

Le boîtier sert MCP en HTTPS avec `neuroedge mcp serve --http`. Il exige un certificat client qu'il a lui-même signé. NeuroClaw en présente un grâce au bloc `tls` d'un serveur MCP de type `http`, donc aucun pont n'est nécessaire.

Dans `~/.neuroclaw/config.json` :

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

| Champ `tls` | Signification |
|:---|:---|
| `ca_file` | AC (CA) au format PEM qui a signé le certificat du boîtier. Seule cette AC est approuvée. |
| `cert_file`, `key_file` | Le certificat client et la clé du cerveau. Renseignez les deux ou aucun. |
| `server_name` | Nom à vérifier dans le certificat du boîtier, s'il diffère de l'hôte dans `url`. |

Règles de `pkg/mcp/tls.go` : `url` doit être en `https` quand `tls` est défini ; le minimum est TLS 1.3 ; les chemins acceptent `~/`. Vérifiez avec :

```bash
neuroclaw mcp test neuroedge
```

## Ce qu'on dit au modèle

Un outil du boîtier renvoie un résultat accompagné d'un statut. NeuroClaw transforme tout résultat autre qu'un simple `ALLOW` en consignes fixes pour le modèle (`pkg/tools/integration/neuroedge_result.go`), de sorte que la réponse est la même quel que soit le modèle.

| Statut | Ce qu'on dit au modèle |
|:---|:---|
| `BLOCK` avec une confirmation | Le boîtier ne l'a pas encore fait. Le propriétaire doit approuver sur la page NeuroEdge à la maison dans le délai indiqué en secondes environ. Le modèle ne peut pas approuver, et une approbation dans le chat ne compte pas. Dire à l'utilisateur que c'est en attente ; ne pas rappeler l'outil ; ne pas dire que c'est fait. |
| `BLOCK` sans confirmation | Le boîtier a refusé, avec la raison et le critère échoué. Dire à l'utilisateur que ce n'est pas fait et pourquoi ; ne pas réessayer avec d'autres arguments ni chercher à contourner la règle. |
| `REJECTED` | Le boîtier a rejeté l'appel lui-même (outil inconnu ou arguments hors de son schéma). Rien n'a été exécuté. |

Les résultats des autres serveurs MCP ne sont pas modifiés.

## Étiquettes de risque

Au démarrage, NeuroClaw journalise `NeuroClaw risk=<nom>` pour chaque capacité qui permet au modèle d'aller au-delà du chat, de son espace de travail et des outils du boîtier :

| Étiquette | Activée quand |
|:---|:---|
| `shell` | `tools.exec` (activé par défaut) |
| `cron_shell` | `tools.cron.allow_command` avec `exec` (activé par défaut) |
| `skill_install` | `tools.install_skill` ou `find_skills`, avec `tools.skills` activé (activé par défaut) |
| `hardware_bus` | `tools.i2c`, `spi` ou `serial` (désactivé par défaut) |
| `process_hooks` | une entrée activée dans `hooks.processes` (désactivé par défaut) |
| `usb_monitor` | `devices.monitor_usb` avec `devices` (désactivé par défaut) |
| `stdio_mcp` | un serveur stdio dans `tools.mcp.servers` (désactivé par défaut) |

Les étiquettes informent ; elles ne bloquent rien. Signification de chacune : [capabilities](../reference/capabilities.md).

## Même machine que le boîtier : fermer le cerveau

La porte du boîtier ne couvre que les appels qui passent par elle. Un shell ou un bus matériel sur la machine qui exécute aussi le boîtier peut atteindre des appareils en contournant la porte. Si le cerveau partage une machine avec le boîtier, désactivez-les :

```json
{
  "agents": { "defaults": { "restrict_to_workspace": true } },
  "tools": {
    "exec": { "enabled": false },
    "cron": { "allow_command": false },
    "install_skill": { "enabled": false },
    "find_skills": { "enabled": false },
    "skills": {
      "registries": {
        "clawhub": { "enabled": false },
        "github": { "enabled": false }
      }
    },
    "i2c": { "enabled": false },
    "spi": { "enabled": false },
    "serial": { "enabled": false }
  },
  "hooks": { "enabled": false },
  "devices": { "enabled": false }
}
```

Fusionnez-le avec votre config ; conservez le bloc `mcp` ci-dessus. Redémarrez NeuroClaw et vérifiez qu'aucune ligne `risk=` correspondante n'apparaît dans le journal.

Exécutez aussi le cerveau sous son propre utilisateur du système (voir l'unité systemd du [guide d'installation](install.fr.md#systemd)), sans accès aux clés du boîtier autres que le certificat client propre au cerveau, et sans accès aux nœuds de périphériques.

## Sur une autre machine ou NeuroEdge Cloud

Quand le cerveau tourne sur une autre machine, ou sur NeuroEdge Cloud, la porte décide toujours de chaque action sur un appareil. Le shell et les bus matériels n'atteignent que la machine où tourne le cerveau ; laissez-les donc selon vos besoins.

## NeuroClaw-ESP

Le même cerveau sur un seul ESP32-S3, avec la porte sur la même puce : [esp32/README.md](../../esp32/README.md). Il est étiqueté `risk=single_chip` car le cerveau et la porte partagent un même espace d'adressage.
