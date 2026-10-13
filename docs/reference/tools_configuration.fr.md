# 🔧 Configuration des Outils

> Retour au [README](../project/README.fr.md)

La configuration des outils de NeuroClaw se trouve dans le champ `tools` de `config.json`.

## Structure du répertoire

```json
{
  "tools": {
    "web": {
      ...
    },
    "mcp": {
      ...
    },
    "exec": {
      ...
    },
    "cron": {
      ...
    },
    "skills": {
      ...
    }
  }
}
```

## Filtrage des données sensibles

Avant que les résultats d'outils soient envoyés au LLM, NeuroClaw peut filtrer les valeurs sensibles (clés API, jetons, secrets) de la sortie. Le LLM ne voit ainsi pas ses propres identifiants.

Voir [Filtrage des données sensibles](../security/sensitive_data_filtering.md) pour la documentation complète.

| Config | Type | Par défaut | Description |
|--------|------|------------|-------------|
| `filter_sensitive_data` | bool | `true` | Activer/désactiver le filtrage |
| `filter_min_length` | int | `8` | Longueur minimale du contenu pour déclencher le filtrage |

## Outils Web

Les outils web sont utilisés pour la recherche et la récupération de pages web.

### Web Fetcher
Paramètres généraux pour la récupération et le traitement du contenu des pages web.

| Config              | Type   | Par défaut    | Description                                                                                   |
|---------------------|--------|---------------|-----------------------------------------------------------------------------------------------|
| `enabled`           | bool   | true          | Activer la capacité de récupération de pages web.                                             |
| `fetch_limit_bytes` | int    | 10485760      | Taille maximale du contenu de la page web à récupérer, en octets (par défaut 10 Mo).          |
| `format`            | string | "plaintext"   | Format de sortie du contenu récupéré. Options : `plaintext` ou `markdown` (recommandé).       |

### DuckDuckGo

| Config        | Type | Par défaut | Description                    |
|---------------|------|------------|--------------------------------|
| `enabled`     | bool | false      | Activer la recherche DuckDuckGo |
| `max_results` | int  | 5          | Nombre maximum de résultats    |

### Sogou

| Config        | Type | Par défaut | Description                    |
|---------------|------|------------|--------------------------------|
| `enabled`     | bool | true       | Activer la recherche Sogou     |
| `max_results` | int  | 5          | Nombre maximum de résultats    |

### Gemini Google Search

La recherche Gemini utilise Gemini avec l'ancrage Google Search. Elle renvoie une réponse synthétisée par l'IA, avec des citations issues de Google Search.

| Config        | Type   | Par défaut           | Description                            |
|---------------|--------|----------------------|----------------------------------------|
| `enabled`     | bool   | false                | Activer Gemini Google Search           |
| `api_key`     | string | -                    | Clé API Google Gemini                  |
| `model`       | string | `gemini-2.5-flash`   | Modèle Gemini utilisé pour la recherche |
| `max_results` | int    | 5                    | Nombre maximum de citations            |

```json
{
  "tools": {
    "web": {
      "gemini": {
        "enabled": true,
        "api_key": "YOUR_GEMINI_API_KEY",
        "model": "gemini-2.5-flash",
        "max_results": 5
      }
    }
  }
}
```

### Baidu Search

| Config        | Type   | Par défaut                                                      | Description                        |
|---------------|--------|-----------------------------------------------------------------|------------------------------------|
| `enabled`     | bool   | false                                                           | Activer la recherche Baidu         |
| `api_key`     | string | -                                                               | Clé API Qianfan                    |
| `base_url`    | string | `https://qianfan.baidubce.com/v2/ai_search/web_search`         | URL de l'API Baidu Search          |
| `max_results` | int    | 10                                                              | Nombre maximum de résultats        |

```json
{
  "tools": {
    "web": {
      "baidu_search": {
        "enabled": true,
        "api_key": "YOUR_BAIDU_QIANFAN_API_KEY",
        "max_results": 10
      }
    }
  }
}
```

### Perplexity

| Config        | Type   | Par défaut | Description                    |
|---------------|--------|------------|--------------------------------|
| `enabled`     | bool     | false      | Activer la recherche Perplexity                              |
| `api_key`     | string   | -          | Clé API Perplexity                                           |
| `api_keys`    | string[] | -          | Plusieurs clés API Perplexity pour la rotation (`api_key` prioritaire) |
| `max_results` | int      | 5          | Nombre maximum de résultats                                  |

### Brave

| Config        | Type   | Par défaut | Description               |
|---------------|--------|------------|---------------------------|
| `enabled`     | bool     | false      | Activer la recherche Brave                                   |
| `api_key`     | string   | -          | Clé API Brave Search                                         |
| `api_keys`    | string[] | -          | Plusieurs clés API Brave Search pour la rotation (`api_key` prioritaire) |
| `max_results` | int      | 5          | Nombre maximum de résultats                                  |

### Kagi Search

Kagi Search utilise le client OpenAPI officiel de Kagi pour `POST /search` et renvoie les résultats web normaux de `data.search`.

| Config        | Type     | Par défaut                            | Description                                    |
|---------------|----------|---------------------------------------|------------------------------------------------|
| `enabled`     | bool     | false                                 | Activer Kagi Search                            |
| `api_key`     | string   | -                                     | Clé API Kagi                                   |
| `api_keys`    | string[] | -                                     | Plusieurs clés API pour la rotation (`api_key` prioritaire) |
| `base_url`    | string   | `https://kagi.com/api/v1/search`      | Point de terminaison de l'API Kagi Search      |
| `max_results` | int      | 5                                     | Nombre maximum de résultats                    |

```json
{
  "tools": {
    "web": {
      "provider": "kagi",
      "kagi": {
        "enabled": true,
        "max_results": 5,
        "base_url": "https://kagi.com/api/v1/search"
      }
    }
  }
}
```

Stockez les clés API Kagi dans `.security.yml` :

```yaml
web:
  kagi:
    api_keys:
      - "YOUR_KAGI_API_KEY"
```

L'utilisation de l'API Kagi peut être facturée ou limitée séparément d'un abonnement Kagi classique, selon votre compte et votre configuration d'API.

### Tavily

| Config        | Type   | Par défaut | Description                        |
|---------------|--------|------------|------------------------------------|
| `enabled`     | bool   | false      | Activer la recherche Tavily        |
| `api_key`     | string | -          | Clé API Tavily                     |
| `base_url`    | string | -          | URL de base Tavily personnalisée   |
| `max_results` | int    | 0          | Nombre maximum de résultats (0 = défaut) |

### SearXNG

| Config        | Type   | Par défaut               | Description                    |
|---------------|--------|--------------------------|--------------------------------|
| `enabled`     | bool   | false                    | Activer la recherche SearXNG   |
| `base_url`    | string | -                        | URL de l'instance SearXNG (par exemple `http://localhost:8888`) |
| `max_results` | int    | 5                        | Nombre maximum de résultats    |

### GLM Search

| Config          | Type   | Par défaut                                           | Description               |
|-----------------|--------|------------------------------------------------------|---------------------------|
| `enabled`       | bool   | false                                                | Activer GLM Search        |
| `api_key`       | string | -                                                    | Clé API GLM               |
| `base_url`      | string | `https://open.bigmodel.cn/api/paas/v4/web_search`   | URL de l'API GLM Search   |
| `search_engine` | string | `search_std`                                         | Type de moteur de recherche |
| `max_results`   | int    | 5                                                    | Nombre maximum de résultats |

### Paramètres web supplémentaires

| Config                   | Type     | Par défaut | Description                                                    |
|--------------------------|----------|------------|----------------------------------------------------------------|
| `provider`               | string   | `auto`     | Fournisseur de recherche à utiliser ; `auto` choisit parmi les moteurs activés |
| `proxy`                  | string   | `""`       | Proxy pour les outils web                                      |
| `prefer_native`          | bool     | true       | Préférer la recherche native du fournisseur aux moteurs de recherche configurés |
| `private_host_whitelist` | string[] | `[]`       | Hôtes privés/internes autorisés pour la récupération web       |

### Paramètres de l'outil `web_search`

À l'exécution, l'outil `web_search` accepte les paramètres suivants :

| Champ | Type | Requis | Description |
|-------|------|--------|-------------|
| `query` | string | oui | Chaîne de requête de recherche |
| `count` | integer | non | Nombre de résultats à renvoyer. Par défaut : `10`, max : `10` |
| `range` | string | non | Filtre temporel optionnel : `d` (jour), `w` (semaine), `m` (mois), `y` (année) |

Si `range` est omis, NeuroClaw effectue une recherche sans restriction.
Pour Kagi, `d`, `w` et `m` correspondent à `time_relative` de la lens Kagi ; `y` correspond à une date `time_after` de la lens située un an avant le jour courant.

### Exemple d'appel `web_search`

```json
{
  "query": "ai agent news",
  "count": 10,
  "range": "w"
}
```

## Outil Exec

L'outil exec est utilisé pour exécuter des commandes shell.

| Config                 | Type  | Par défaut | Description                                    |
|------------------------|-------|------------|------------------------------------------------|
| `enabled`               | bool  | true       | Activer l'outil exec. Activé, il est signalé au démarrage par `risk=shell` |
| `enable_deny_patterns`  | bool  | true       | Activer le blocage par défaut des commandes dangereuses |
| `custom_deny_patterns`  | array | []         | Modèles de refus personnalisés (expressions régulières) |
| `custom_allow_patterns` | array | []         | Modèles d'autorisation personnalisés (expressions régulières) |
| `allow_remote`          | bool  | true       | Autoriser les appels exec provenant de canaux distants (applications de chat) ; `false` limite exec aux canaux internes |
| `timeout_seconds`       | int   | 60         | Délai d'expiration par commande en secondes ; `0` utilise la valeur par défaut (60) |

### Désactivation de l'Outil Exec

Pour désactiver complètement l'outil `exec`, définissez `enabled` à `false` :

**Via le fichier de configuration :**
```json
{
  "tools": {
    "exec": {
      "enabled": false
    }
  }
}
```

**Via la variable d'environnement :**
```bash
NEUROCLAW_TOOLS_EXEC_ENABLED=false
```

> **Note :** Lorsqu'il est désactivé, l'agent ne pourra pas exécuter de commandes shell. Cela affecte également la capacité de l'outil Cron à exécuter des commandes shell planifiées.

### Fonctionnalité

- **`enable_deny_patterns`** : Définir à `false` pour désactiver complètement les modèles de blocage par défaut des commandes dangereuses
- **`custom_deny_patterns`** : Ajouter des modèles regex de refus personnalisés ; les commandes correspondantes seront bloquées
- **`custom_allow_patterns`** : Ajouter des modèles regex d'autorisation ; les commandes correspondantes sont exemptées des modèles de refus
- **`allow_remote`** : Définir à `false` pour refuser les appels exec provenant de canaux distants

### Modèles de commandes bloquées par défaut

Par défaut, NeuroClaw bloque les commandes dangereuses suivantes :

- Commandes de suppression : `rm -rf`, `del /f/q`, `rmdir /s`
- Opérations disque : `format`, `mkfs`, `diskpart`, `dd if=`, écriture vers `/dev/sd*`
- Opérations système : `shutdown`, `reboot`, `poweroff`
- Substitution de commandes : `$()`, `${}`, backticks
- Pipe vers shell : `| sh`, `| bash`
- Élévation de privilèges : `sudo`, `chmod`, `chown`
- Contrôle de processus : `pkill`, `killall`, `kill -9`
- Opérations distantes : `curl | sh`, `wget | sh`, `ssh`
- Gestion de paquets : `apt`, `yum`, `dnf`, `npm install -g`, `pip install --user`
- Conteneurs : `docker run`, `docker exec`
- Git : `git push`, `git force`
- Autres : `eval`, `source *.sh`

### Limitation architecturale connue

Le garde exec ne valide que la commande de niveau supérieur envoyée à NeuroClaw. Il n'inspecte **pas** récursivement les processus enfants générés par les outils de build ou les scripts après le démarrage de cette commande.

Exemples de workflows pouvant contourner le garde de commande directe une fois la commande initiale autorisée :

- `make run`
- `go run ./cmd/...`
- `cargo run`
- `npm run build`

Cela signifie que le garde est utile pour bloquer les commandes directes manifestement dangereuses, mais ce n'est **pas** un bac à sable complet pour les pipelines de build non vérifiés. Si votre modèle de menace inclut du code non fiable dans l'espace de travail, utilisez une isolation plus forte comme des conteneurs, des VM ou un flux d'approbation autour des commandes de build et d'exécution.

### Exemple de configuration

```json
{
  "tools": {
    "exec": {
      "enable_deny_patterns": true,
      "custom_deny_patterns": [
        "\\brm\\s+-r\\b",
        "\\bkillall\\s+python"
      ]
    }
  }
}
```

## Outil Cron

L'outil cron est utilisé pour planifier des tâches périodiques.

| Config                    | Type     | Par défaut | Description                                                    |
|---------------------------|----------|------------|----------------------------------------------------------------|
| `enabled`                 | bool     | true       | Enregistrer l'outil cron exposé à l'agent                      |
| `allow_command`           | bool     | true       | Autoriser les tâches de commande sans confirmation supplémentaire. Avec `exec` activé, signalé au démarrage par `risk=cron_shell` |
| `command_allowed_remotes` | string[] | []         | Canaux distants ou valeurs `channel:chat_id` autorisés pour les tâches de commande ; `*` autorise tous les canaux et est dangereux sauf si tous les canaux distants sont de confiance |
| `exec_timeout_minutes`    | int      | 5          | Délai d'expiration en minutes, 0 signifie sans limite         |

Pour les types de planification, les modes d'exécution (`deliver`, tour d'agent et tâches de commande), la persistance et les garde-fous de sécurité actuels des commandes, voir [Tâches planifiées et jobs cron](cron.md).

## Interrupteurs par outil

Chaque autre outil a son propre interrupteur `enabled` sous `tools.<name>` (variable d'environnement `NEUROCLAW_TOOLS_<NAME>_ENABLED`). Les valeurs par défaut sont celles de `pkg/config/defaults.go` :

| Outil | Par défaut | Notes |
|-------|------------|-------|
| `read_file`, `write_file`, `append_file`, `edit_file`, `list_dir` | activé | `read_file` a aussi `mode` (`bytes` ou `lines`, par défaut `bytes`) et `max_read_file_size` (par défaut 65536) |
| `load_image`, `send_file`, `web_fetch` | activé | |
| `message` | activé | `media_enabled` est désactivé par défaut |
| `spawn`, `subagent` | activé | |
| `spawn_status` | désactivé | |
| `get_diagnostics` | activé | Voir [Outil de diagnostic](#diagnostics-tool) |
| `find_skills`, `install_skill` | activé | Effectifs seulement avec `tools.skills.enabled` ; signalés par `risk=skill_install` |
| `send_tts` | désactivé | |
| `i2c`, `spi`, `serial` | désactivé | Outils matériels ; `i2c` et `spi` uniquement sous Linux. Signalés par `risk=hardware_bus` lorsqu'ils sont activés |
| `media_cleanup` | activé | `max_age_minutes` 30, `interval_minutes` 5 |

Les étiquettes de risque et leur signification sont décrites dans [Ce que NeuroClaw sait faire, section 2](capabilities.md#2-risk-labels).

<a id="diagnostics-tool"></a>
## Outil de diagnostic

`get_diagnostics` permet au modèle de rendre compte de l'état de NeuroClaw lui-même. L'outil est en lecture seule, ne prend aucun argument et porte le même nom que l'outil de NeuroClaw-ESP. Il renvoie du JSON contenant :

- `version`, `git_commit`, `go_version`, `os`, `arch`, `num_cpu` ;
- `started_at` et `uptime_sec` du processus ;
- `memory` (tas Go et mémoire totale selon le runtime, nombre de GC) et `goroutines` ;
- `workspace_disk` : octets libres et totaux du système de fichiers qui contient l'espace de travail de l'agent (Linux, macOS, FreeBSD, Windows ; ailleurs, une entrée `error` indique que la valeur n'est pas disponible) ;
- `channels_enabled` : noms des canaux activés ;
- `models` : le modèle par défaut, ses replis et `cooldowns`, une entrée par modèle en échec depuis le démarrage (`available: false` tant que la chaîne de repli l'ignore, `remaining_sec`, `disabled_reason` pour les erreurs de facturation) ;
- `llm_budget` : limites, appels de l'heure et du jour, heures de remise à zéro (voir [budget d'appels LLM](rate-limiting.md#llm-call-budget), en anglais) ;
- `risk_labels` : les capacités à risque activées (telles que journalisées au démarrage) ;
- `last_llm_error` : date et message du dernier appel LLM en échec, ou `null`.

L'outil ne rapporte que des noms, des compteurs et des états. Les clés d'API, jetons et URL n'y figurent jamais : dans la dernière erreur LLM, les identifiants et les paramètres de requête des URL sont retirés, puis le texte passe par le [filtrage des données sensibles](../security/sensitive_data_filtering.md).

| Config | Type | Par défaut | Description |
|--------|------|------------|-------------|
| `enabled` | bool | true | Enregistre l'outil `get_diagnostics` |

```json
{
  "tools": {
    "get_diagnostics": {
      "enabled": true
    }
  }
}
```

Variable d'environnement : `NEUROCLAW_TOOLS_GET_DIAGNOSTICS_ENABLED`.

<a id="mcp-tool"></a>
## Outil MCP

L'outil MCP permet l'intégration avec des serveurs Model Context Protocol externes.

Si vous préférez ne pas modifier le JSON à la main, NeuroClaw fournit aussi une CLI de gestion de la configuration MCP :

- `neuroclaw mcp add` — ajouter ou mettre à jour un serveur (prend en charge `--deferred` / `--no-deferred`)
- `neuroclaw mcp list` — lister tous les serveurs configurés avec leur état et leur mode différé
- `neuroclaw mcp show <name>` — afficher les détails complets et la liste des outils d'un serveur
- `neuroclaw mcp test <name>` — vérifier la connectivité d'un serveur
- `neuroclaw mcp remove <name>` — supprimer l'entrée d'un serveur
- `neuroclaw mcp edit` — ouvrir `config.json` dans `$EDITOR` pour des modifications avancées

Ces commandes gèrent la même section `tools.mcp.servers` documentée ci-dessous. Voir [CLI du serveur MCP](mcp-cli.md) pour la syntaxe, les exemples et le comportement détaillé.

### Découverte d'outils (chargement paresseux)

Lors de la connexion à plusieurs serveurs MCP, exposer simultanément des centaines d'outils peut épuiser la fenêtre de contexte du LLM et augmenter les coûts API. La fonctionnalité **Discovery** résout ce problème en gardant les outils MCP *masqués* par défaut.

Au lieu de charger tous les outils, le LLM reçoit un outil de recherche léger (utilisant la correspondance par mots-clés BM25 ou les expressions régulières). Lorsque le LLM a besoin d'une capacité spécifique, il recherche dans la bibliothèque masquée. Les outils correspondants sont alors temporairement « déverrouillés » et injectés dans le contexte pour un nombre configuré de tours (`ttl`).

### Configuration globale

| Config      | Type   | Par défaut | Description                                  |
|-------------|--------|------------|----------------------------------------------|
| `enabled`   | bool   | false      | Activer l'intégration MCP globalement        |
| `discovery` | object | `{}`       | Configuration de la découverte d'outils (voir ci-dessous) |
| `servers`   | object | `{}`       | Mappage du nom de serveur à la configuration du serveur |

### Configuration Discovery (`discovery`)

| Config               | Type | Par défaut | Description                                                                                                                       |
|----------------------|------|------------|-----------------------------------------------------------------------------------------------------------------------------------|
| `enabled`            | bool | false      | Valeur globale par défaut : si `true`, tous les outils MCP sont masqués et chargés à la demande via la recherche ; si `false`, tous les outils sont chargés dans le contexte. Chaque serveur peut la remplacer avec son champ `deferred`         |
| `ttl`                | int  | 5          | Nombre de tours de conversation pendant lesquels un outil découvert reste déverrouillé                                            |
| `max_search_results` | int  | 5          | Nombre maximum d'outils retournés par requête de recherche                                                                        |
| `use_bm25`           | bool | true       | Activer l'outil de recherche par langage naturel/mots-clés (`tool_search_tool_bm25`). **Attention** : consomme plus de ressources que la recherche regex |
| `use_regex`          | bool | false      | Activer l'outil de recherche par motif regex (`tool_search_tool_regex`)                                                           |

> **Note :** Si `discovery.enabled` est `true`, vous **devez** activer au moins un moteur de recherche (`use_bm25` ou `use_regex`),
> sinon l'application ne démarrera pas.

### Configuration par serveur

| Config     | Type   | Requis   | Description                                |
|------------|--------|----------|--------------------------------------------|
| `enabled`  | bool   | oui      | Activer ce serveur MCP                     |
| `deferred` | bool   | non      | Remplace le mode différé pour ce serveur seul. `true` = outils masqués et découvrables par recherche ; `false` = outils toujours visibles dans le contexte. Si omis, la valeur globale `discovery.enabled` s'applique. |
| `type`     | string | non      | Type de transport : `stdio`, `sse`, `http` (`streamable-http` est accepté comme alias de `http`) |
| `command`  | string | stdio    | Commande exécutable pour le transport stdio |
| `args`     | array  | non      | Arguments de commande pour le transport stdio |
| `env`      | object | non      | Variables d'environnement pour le processus stdio |
| `env_file` | string | non      | Chemin vers le fichier d'environnement pour le processus stdio |
| `url`      | string | sse/http | URL du point de terminaison pour le transport `sse`/`http` |
| `headers`  | object | non      | En-têtes HTTP pour le transport `sse`/`http` |
| `tls`      | object | non      | NeuroClaw : TLS mutuel pour le transport `sse`/`http` (voir ci-dessous) |

### TLS mutuel (`tls`)

Ajout de NeuroClaw. Un boîtier NeuroEdge domestique démarré avec `mcp serve --http` refuse tout client sans certificat signé par lui. Définissez `tls` sur un serveur `http` ou `sse` pour que le cerveau présente un certificat client et ne fasse confiance qu'à la CA du boîtier, sans pont TLS à côté.

| Config        | Type   | Description |
|---------------|--------|-------------|
| `ca_file`     | string | Fichier PEM de la CA qui a signé le certificat du serveur. Vide : racines système |
| `cert_file`   | string | Fichier PEM du certificat de ce client. À définir avec `key_file`, ou aucun des deux |
| `key_file`    | string | Fichier PEM de la clé privée de ce client |
| `server_name` | string | Remplace le nom vérifié dans le certificat du serveur |

Les chemins peuvent commencer par `~`. Règles, d'après `pkg/mcp/tls.go` :

- `url` doit être en `https://` ; sinon la connexion au serveur échoue avec `tls is set but url ... is not https`.
- La connexion utilise TLS 1.3 au minimum.
- Sans `tls`, le transport HTTP par défaut est utilisé, comme dans PicoClaw.
- `tls` est ignoré par les serveurs `stdio`.
- `neuroclaw mcp add` n'a pas d'option pour `tls` ; ajoutez-le en modifiant `config.json` (`neuroclaw mcp edit`). Relancer `mcp add` sur le même nom remplace l'entrée et supprime le bloc `tls`.

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "servers": {
        "home": {
          "enabled": true,
          "type": "http",
          "url": "https://neuroedge-box.local:8443/mcp",
          "tls": {
            "ca_file": "~/.neuroclaw/pki/ca.pem",
            "cert_file": "~/.neuroclaw/pki/brain.pem",
            "key_file": "~/.neuroclaw/pki/brain.key",
            "server_name": "neuroedge-box.local"
          }
        }
      }
    }
  }
}
```

Voir le [guide NeuroEdge](../guides/neuroedge.md) pour le côté boîtier domestique.

### Comportement du transport

- Si `type` est omis, le transport est détecté automatiquement :
    - `url` est défini → `sse`
    - `command` est défini → `stdio`
- `http` et `sse` utilisent tous deux `url` + `headers` optionnels + `tls` optionnel.
- `neuroclaw mcp add` utilise `stdio` par défaut (voir [CLI MCP](mcp-cli.md)) ; cette détection automatique ne s'applique qu'aux entrées écrites à la main.
- Un serveur `stdio` activé est signalé au démarrage par `risk=stdio_mcp`.
- `env` et `env_file` ne sont appliqués qu'aux serveurs `stdio`.

### Exemples de configuration

#### 1) Serveur MCP Stdio

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "servers": {
        "filesystem": {
          "enabled": true,
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-filesystem",
            "/tmp"
          ]
        }
      }
    }
  }
}
```

#### 2) Serveur MCP distant SSE/HTTP

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "servers": {
        "remote-mcp": {
          "enabled": true,
          "type": "sse",
          "url": "https://example.com/mcp",
          "headers": {
            "Authorization": "Bearer YOUR_TOKEN"
          }
        }
      }
    }
  }
}
```

#### 3) Configuration MCP massive avec découverte d'outils activée

*Dans cet exemple, le LLM ne verra que `tool_search_tool_bm25`. Il recherchera et déverrouillera dynamiquement les outils Github ou Postgres uniquement lorsque l'utilisateur le demande.*

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "discovery": {
        "enabled": true,
        "ttl": 5,
        "max_search_results": 5,
        "use_bm25": true,
        "use_regex": false
      },
      "servers": {
        "github": {
          "enabled": true,
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-github"
          ],
          "env": {
            "GITHUB_PERSONAL_ACCESS_TOKEN": "YOUR_GITHUB_TOKEN"
          }
        },
        "postgres": {
          "enabled": true,
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-postgres",
            "postgresql://user:password@localhost/dbname"
          ]
        },
        "slack": {
          "enabled": true,
          "type": "slack",
          "command": "npx",
          "args": [
            "-y",
            "@modelcontextprotocol/server-slack"
          ],
          "env": {
            "SLACK_BOT_TOKEN": "YOUR_SLACK_BOT_TOKEN",
            "SLACK_TEAM_ID": "YOUR_SLACK_TEAM_ID"
          }
        }
      }
    }
  }
}
```

#### 4) Configuration mixte : surcharge `deferred` par serveur

*La découverte est activée globalement, mais `filesystem` est fixé comme toujours visible tandis que `context7` suit la valeur globale par défaut (différé). `aws` choisit explicitement le mode différé, même si c'est identique à la valeur globale.*

```json
{
  "tools": {
    "mcp": {
      "enabled": true,
      "discovery": {
        "enabled": true,
        "ttl": 5,
        "max_search_results": 5,
        "use_bm25": true
      },
      "servers": {
        "filesystem": {
          "enabled": true,
          "command": "npx",
          "args": ["-y", "@modelcontextprotocol/server-filesystem", "/workspace"],
          "deferred": false
        },
        "context7": {
          "enabled": true,
          "command": "npx",
          "args": ["-y", "@upstash/context7-mcp"]
        },
        "aws": {
          "enabled": true,
          "command": "npx",
          "args": ["-y", "aws-mcp-server"],
          "deferred": true
        }
      }
    }
  }
}
```

> **Astuce :** `deferred` par serveur est indépendant de `discovery.enabled`. Vous pouvez garder
> `discovery.enabled: false` globalement (tous les outils visibles par défaut) et marquer malgré tout certains
> serveurs volumineux avec `"deferred": true` pour éviter de polluer le contexte avec leurs outils.

<a id="skills-tool"></a>
## Outil Skills

L'outil skills configure la découverte et l'installation de compétences via des registres comme ClawHub et GitHub.

### Registres

| Config                             | Type   | Par défaut           | Description                                  |
|------------------------------------|--------|----------------------|----------------------------------------------|
| `registries.clawhub.enabled`       | bool   | true                 | Activer le registre ClawHub                  |
| `registries.clawhub.base_url`      | string | `https://clawhub.ai` | URL de base ClawHub                          |
| `registries.clawhub.auth_token`    | string | `""`                 | Jeton Bearer optionnel pour des limites de débit plus élevées |
| `registries.clawhub.search_path`   | string | `""`                 | Chemin de l'API de recherche                 |
| `registries.clawhub.skills_path`   | string | `""`                 | Chemin de l'API Skills                       |
| `registries.clawhub.download_path` | string | `""`                 | Chemin de l'API de téléchargement            |
| `registries.clawhub.timeout`       | int    | 0                    | Délai d'expiration des requêtes en secondes (0 = défaut) |
| `registries.clawhub.max_zip_size`  | int    | 0                    | Taille maximale du zip d'une compétence en octets (0 = défaut) |
| `registries.clawhub.max_response_size` | int | 0                   | Taille maximale de la réponse de l'API en octets (0 = défaut) |
| `registries.github.enabled`        | bool   | true                 | Activer les installations GitHub via la configuration des registres |
| `registries.github.base_url`       | string | `https://github.com` | URL de base de GitHub ou GitHub Enterprise   |
| `registries.github.auth_token`     | string | `""`                 | Jeton d'accès personnel GitHub               |
| `registries.github.proxy`          | string | `""`                 | Proxy HTTP pour les requêtes à l'API GitHub  |

### Configuration GitHub héritée

`github.*` est obsolète. Utilisez `registries.github.*` à la place. Les anciens champs restent pris en charge pour la compatibilité et seront supprimés plus tard.

| Config             | Type   | Par défaut           | Description                    |
|--------------------|--------|----------------------|--------------------------------|
| `github.base_url`  | string | `https://github.com` | URL de base GitHub (obsolète)  |
| `github.proxy`     | string | `""`                 | Proxy GitHub (obsolète)        |
| `github.token`     | string | `""`                 | Jeton GitHub (obsolète)        |

### Paramètres de recherche

| Config                    | Type | Par défaut | Description                                |
|---------------------------|------|------------|--------------------------------------------|
| `max_concurrent_searches` | int  | 2          | Nombre maximal de recherches de compétences simultanées |
| `search_cache.max_size`   | int  | 50         | Nombre maximal de résultats de recherche en cache |
| `search_cache.ttl_seconds`| int  | 300        | Durée de vie du cache en secondes          |

### Exemple de configuration

```json
{
  "tools": {
    "skills": {
      "registries": {
        "clawhub": {
          "enabled": true,
          "base_url": "https://clawhub.ai",
          "auth_token": "",
          "search_path": "",
          "skills_path": "",
          "download_path": "",
          "timeout": 0,
          "max_zip_size": 0,
          "max_response_size": 0
        },
        "github": {
          "enabled": true,
          "base_url": "https://github.com",
          "auth_token": "",
          "proxy": ""
        }
      },
      "github": {
        "base_url": "https://github.com",
        "proxy": "",
        "token": ""
      },
      "max_concurrent_searches": 2,
      "search_cache": {
        "max_size": 50,
        "ttl_seconds": 300
      }
    }
  }
}
```

## Variables d'environnement

Toutes les options de configuration peuvent être remplacées via des variables d'environnement au format `NEUROCLAW_TOOLS_<SECTION>_<KEY>` :

Par exemple :

- `NEUROCLAW_TOOLS_WEB_BRAVE_ENABLED=true`
- `NEUROCLAW_TOOLS_EXEC_ENABLED=false`
- `NEUROCLAW_TOOLS_EXEC_ENABLE_DENY_PATTERNS=false`
- `NEUROCLAW_TOOLS_CRON_EXEC_TIMEOUT_MINUTES=10`
- `NEUROCLAW_TOOLS_MCP_ENABLED=true`
- `NEUROCLAW_TOOLS_MCP_MAX_INLINE_TEXT_CHARS=16384`

Note : La configuration de type map imbriquée (par exemple `tools.mcp.servers.<name>.*`) est configurée dans `config.json` plutôt que via des variables d'environnement.

Pour les outils MCP, `tools.mcp.max_inline_text_chars` contrôle la quantité de texte d'un résultat conservée en ligne dans le contexte du modèle. Le seuil est compté en caractères Unicode (runes Go), pas en octets. Par exemple, `16384` signifie jusqu'à 16 384 caractères en ligne, ce qui peut dépasser 16 Ko pour du texte multioctet comme le CJK. Au-delà de ce seuil, NeuroClaw enregistre le résultat texte MCP comme artefact local dans l'espace de travail de l'agent et donne au modèle une courte note ainsi qu'un chemin d'artefact structuré `[file:...]` au lieu d'injecter toute la charge utile dans le contexte.
