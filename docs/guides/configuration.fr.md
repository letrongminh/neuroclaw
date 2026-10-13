# ⚙️ Guide de Configuration

> Retour au [README](../project/README.fr.md)

## ⚙️ Configuration

Fichier de configuration : `~/.neuroclaw/config.json`

> **Configuration de sécurité :** pour stocker les clés API, jetons et autres données sensibles, voir le [Guide de configuration de sécurité](../security/security_configuration.md).

### Variables d'Environnement

Vous pouvez remplacer les chemins par défaut à l'aide de variables d'environnement. Ceci est utile pour les installations portables, les déploiements conteneurisés ou l'exécution de NeuroClaw en tant que service système. Ces variables sont indépendantes et contrôlent des chemins différents.

| Variable          | Description                                                                                                                             | Chemin par défaut         |
|-------------------|-----------------------------------------------------------------------------------------------------------------------------------------|---------------------------|
| `NEUROCLAW_CONFIG` | Remplace le chemin vers le fichier de configuration. Indique directement à NeuroClaw quel `config.json` charger, en ignorant tous les autres emplacements. | `~/.neuroclaw/config.json` |
| `NEUROCLAW_HOME`   | Remplace le répertoire racine des données NeuroClaw. Change l'emplacement par défaut du `workspace` et des autres répertoires de données. | `~/.neuroclaw`             |

**Exemples :**

```bash
# Run neuroclaw using a specific config file
# The workspace path will be read from within that config file
NEUROCLAW_CONFIG=/etc/neuroclaw/production.json neuroclaw gateway

# Run neuroclaw with all its data stored in /opt/neuroclaw
# Config will be loaded from the default ~/.neuroclaw/config.json
# Workspace will be created at /opt/neuroclaw/workspace
NEUROCLAW_HOME=/opt/neuroclaw neuroclaw agent

# Use both for a fully customized setup
NEUROCLAW_HOME=/srv/neuroclaw NEUROCLAW_CONFIG=/srv/neuroclaw/main.json neuroclaw gateway
```

### Niveau de Log du Gateway

`gateway.log_level` contrôle la verbosité des logs du Gateway, configurable dans `config.json` :

```json
{
  "gateway": {
    "log_level": "warn"
  }
}
```

La valeur par défaut est `warn`. Valeurs supportées : `debug`, `info`, `warn`, `error`, `fatal`.

Peut également être surchargé via la variable d'environnement : `NEUROCLAW_LOG_LEVEL=info`

### Structure du Workspace

NeuroClaw stocke les données dans votre workspace configuré (par défaut : `~/.neuroclaw/workspace`) :

```
~/.neuroclaw/workspace/
├── sessions/          # Sessions de conversation et historique
├── memory/           # Mémoire à long terme (MEMORY.md)
├── state/            # État persistant (dernier canal, etc.)
├── cron/             # Base de données des tâches planifiées
├── skills/           # Compétences personnalisées
├── AGENT.md          # Guide de comportement de l'agent
├── HEARTBEAT.md      # Invites de tâches périodiques (vérifiées toutes les 30 min)
├── IDENTITY.md       # Identité de l'agent
├── SOUL.md           # Âme de l'agent
└── USER.md           # Préférences utilisateur
```

> **Remarque :** Les modifications apportées à `AGENT.md`, `SOUL.md`, `USER.md` et `memory/MEMORY.md` sont détectées automatiquement au moment de l'exécution via le suivi de la date de modification (mtime). Il n'est **pas nécessaire de redémarrer le gateway** après avoir modifié ces fichiers — l'agent charge le nouveau contenu à la prochaine requête.

### Auto-évolution de l'agent

Le bloc `evolution` contrôle le runtime d'auto-évolution de NeuroClaw. Lorsqu'il est activé, l'agent enregistre les tours terminés comme des enregistrements d'apprentissage. Dans les modes supérieurs, il peut regrouper les schémas réussis récurrents, générer des brouillons de skills et, en option, appliquer les brouillons acceptés aux skills du workspace.

```json
{
  "evolution": {
    "enabled": false,
    "mode": "observe",
    "state_dir": "",
    "min_task_count": 2,
    "min_success_ratio": 0.7,
    "cold_path_trigger": "after_turn",
    "cold_path_times": []
  }
}
```

| Champ | Défaut | Description |
|-------|--------|-------------|
| `enabled` | `false` | Active la capture d'enregistrements d'apprentissage pour les tours d'agent terminés. Les tours heartbeat sont ignorés. |
| `mode` | `observe` | `observe` enregistre seulement les données. `draft` peut générer des brouillons de skills candidats. `apply` peut appliquer les brouillons acceptés aux skills du workspace. |
| `state_dir` | `""` | Répertoire facultatif pour l'état de l'évolution. Laissez vide pour utiliser le répertoire par défaut dans le workspace. |
| `min_task_count` | `2` | Nombre minimal d'enregistrements de tâches liés requis avant qu'un schéma soit éligible à la génération de brouillons. |
| `min_success_ratio` | `0.7` | Ratio de réussite minimal d'un groupe de tâches. Utilisez une valeur supérieure à `0` et au plus `1`. |
| `cold_path_trigger` | `after_turn` | Lance la génération de brouillons `after_turn`, selon un calendrier `scheduled`, ou désactive les exécutions automatiques du cold path avec `manual`. Il n'existe pas encore de déclencheur manuel côté utilisateur. S'applique uniquement aux modes `draft` et `apply`. |
| `cold_path_times` | `[]` | Heures d'exécution planifiées utilisées quand `cold_path_trigger` vaut `scheduled`, écrites sous forme de chaînes `HH:MM`. |

Utilisez `observe` d'abord si vous voulez examiner les enregistrements d'apprentissage sans générer de modifications de skills. Utilisez `draft` quand vous voulez que NeuroClaw prépare des améliorations à relire. N'utilisez `apply` que si vous acceptez que les brouillons validés mettent à jour les skills du workspace.

### Politique de contexte de requête

`turn_profile` est une politique facultative sous `agents.defaults.turn_profile` pour contrôler le contexte chargé par chaque nouveau tour : historique, prompt système, prompts de skills et outils autorisés. Sans cette configuration, ou avec `"enabled": false`, NeuroClaw garde son comportement normal. Avec `"enabled": true`, la politique ci-dessous s'applique à chaque nouveau tour.

Tous les blocs utilisent les mêmes valeurs de `mode` :

| Mode | Signification |
| --- | --- |
| `default` | Garde le comportement normal de NeuroClaw. Un bloc absent ou sans `mode` vaut `default`. |
| `off` | Désactive ce bloc pour le tour. |
| `custom` | Utilise une liste d'autorisation. Dans cette version, `custom` est pris en charge seulement pour `skills` et `tools`; l'utiliser pour `history` ou `system_prompt` produit une erreur de validation. |

Blocs disponibles :

| Bloc | Ce qu'il contrôle |
| --- | --- |
| `history` | Lecture de l'historique et du résumé, écriture des messages utilisateur/assistant/outil, ingestion de contexte, compression et résumé. |
| `system_prompt` | Injection de l'identité NeuroClaw, des instructions de l'espace de travail, de la mémoire, du contexte d'exécution et du résumé. Les prompts système externes restent autorisés quand ce bloc est `off`. |
| `skills` | Chargement du catalogue de skills et du contenu des skills actifs. `custom.allow` ne garde que les noms listés. |
| `tools` | Outils exposés au modèle et autorisés à l'exécution. `custom.allow` ne garde que les outils enregistrés et listés. |

Quand `system_prompt.mode` vaut `off`, que des outils restent visibles et qu'aucun prompt système externe n'est fourni, NeuroClaw réutilise sa règle existante d'utilisation des outils comme prompt minimal de secours. Si `tools.mode` vaut `off`, ce prompt de secours n'est pas ajouté.

Exemple de contexte propre avec outils web :

```json
{
  "agents": {
    "defaults": {
      "turn_profile": {
        "enabled": true,
        "history": { "mode": "off" },
        "system_prompt": { "mode": "off" },
        "skills": { "mode": "off" },
        "tools": {
          "mode": "custom",
          "allow": ["web_search", "web_fetch"]
        }
      }
    }
  }
}
```

### Tableau de bord du lanceur web

**neuroclaw-launcher** sert une interface navigateur qui exige d'abord une connexion par mot de passe. Au premier lancement, ouvrez `/launcher-setup` pour créer le mot de passe du tableau de bord. Les connexions manuelles ultérieures passent par `/launcher-login`.

- **Fichier de configuration** : même répertoire que `config.json` (ou le fichier indiqué par `NEUROCLAW_CONFIG`). Le fichier propre au lanceur est `launcher-config.json`.
- **Stockage du mot de passe** : sur les plateformes prises en charge, le mot de passe est stocké sous forme de hash bcrypt dans `launcher-auth.db`. Sur les plateformes où le stockage SQLite n'est pas disponible, le hash bcrypt est stocké dans `launcher-config.json`.
- **Migration de l'ancien format** : les anciennes valeurs `launcher_token` sont migrées une seule fois vers la connexion par mot de passe, puis supprimées de la configuration enregistrée du lanceur.
- **Connexion automatique locale** : quand le lanceur ouvre automatiquement un navigateur local après le démarrage, il utilise un point d'amorçage à usage unique, limité au loopback, pour définir automatiquement le cookie de session.
- **Méthodes d'authentification non prises en charge** : la connexion par jeton dans l'URL (`?token=...`), `NEUROCLAW_LAUNCHER_TOKEN` et l'authentification du tableau de bord par `Authorization: Bearer` ne sont plus prises en charge.
- **Déconnexion** : utilisez **`POST /api/auth/logout`** avec **`Content-Type: application/json`** (le corps peut être `{}`). Ne comptez pas sur une URL GET pour la déconnexion (schéma protégé contre le CSRF).
- **Force brute** : **`POST /api/auth/login`** est **limité par IP cliente et par minute** (HTTP 429 en cas de dépassement).
- **Durée de session** : le cookie de session HttpOnly dure environ **31 jours** par défaut, mais les sessions sont invalidées au redémarrage du processus du lanceur.

### Sources de Compétences

Par défaut, les compétences sont chargées depuis :

1. `~/.neuroclaw/workspace/skills` (workspace)
2. `~/.neuroclaw/skills` (global)
3. `<chemin-intégré-à-la-compilation>/skills` (intégré)

Pour les configurations avancées/de test, vous pouvez remplacer la racine des compétences builtin avec :

```bash
export NEUROCLAW_BUILTIN_SKILLS=/path/to/skills
```

### Utiliser les Commandes Depuis les Canaux de Chat

Une fois les compétences installées et les serveurs MCP configurés, vous pouvez aussi les inspecter et les activer directement depuis un canal de chat :

- `/list skills` affiche les noms des compétences installées visibles pour l'agent courant.
- `/list mcp` affiche les serveurs MCP configurés avec leur statut activé/différé/connecté.
- `/show mcp <server>` affiche les outils actifs exposés par un serveur MCP connecté.
- `/use <skill> <message>` force une compétence pour une seule requête.
- `/use <skill>` prépare cette compétence pour votre prochain message dans la meme conversation.
- `/use clear` annule une surcharge de compétence en attente creee via `/use <skill>`.
- `/btw <question>` pose une question annexe immediate sans modifier l'historique courant de la session. `/btw` est traite comme une requete directe sans outils et n'entre pas dans le flux normal d'execution des outils.

Exemples :

```text
/list skills
/list mcp
/show mcp github
/use git explique comment squash les 3 derniers commits
/btw rappelle-moi ce qu'on a deja decide pour le plan de deploiement
/use italiapersonalfinance
dammi le ultime news
```

### Politique Unifiée d'Exécution des Commandes

- Les commandes slash génériques sont exécutées via un chemin unique dans `pkg/agent/loop.go` via `commands.Executor`.
- Les adaptateurs de canaux ne consomment plus les commandes génériques localement ; ils transmettent le texte entrant au chemin bus/agent. Telegram enregistre toujours automatiquement au démarrage les commandes prises en charge, comme `/start`, `/help`, `/show`, `/list`, `/use` et `/btw`.
- Une commande slash inconnue (par exemple `/foo`) passe au traitement LLM normal.
- Une commande enregistrée mais non prise en charge sur le canal actuel (par exemple `/show` sur WhatsApp) renvoie une erreur explicite à l'utilisateur et arrête le traitement ultérieur.

### Isolation des Sessions

La portée de session contrôle la quantité de mémoire partagée entre conversations, utilisateurs, fils et espaces.

- Utilisez `session.dimensions` pour la valeur par défaut globale.
- Utilisez `session_dimensions` dans une règle de dispatch pour une exception routée précise.

Pour des recettes pas à pas et des schémas d'isolation, voir le [Guide des sessions](session-guide.md).

### Routage

Le routage se configure via `agents.dispatch.rules`.

Chaque règle est comparée au contexte entrant normalisé produit par les canaux. Les règles sont évaluées de haut en bas ; la première qui correspond l'emporte. Si aucune règle ne correspond, NeuroClaw se rabat sur l'agent par défaut configuré.

Champs de correspondance pris en charge :

* `channel`
* `account`
* `space`
* `chat`
* `topic`
* `sender`
* `mentioned`

Les valeurs de correspondance utilisent le même vocabulaire de portée que le système de sessions :

* `space` : `workspace:t001`, `guild:123456`
* `chat` : `direct:user123`, `group:-100123`, `channel:c123`
* `topic` : `topic:42`
* `sender` : un identifiant d'expéditeur normalisé pour la plateforme

Une règle peut éventuellement remplacer la valeur globale `session.dimensions` via `session_dimensions`. Cela permet de garder le routage et l'allocation des sessions alignés sans réintroduire les anciens formats `bindings` ou `dm_scope`.

Exemple :

```json
{
  "agents": {
    "list": [
      { "id": "main", "default": true },
      { "id": "support" },
      { "id": "sales" }
    ],
    "dispatch": {
      "rules": [
        {
          "name": "vip in support group",
          "agent": "sales",
          "when": {
            "channel": "telegram",
            "chat": "group:-1001234567890",
            "sender": "12345"
          },
          "session_dimensions": ["chat", "sender"]
        },
        {
          "name": "telegram support group",
          "agent": "support",
          "when": {
            "channel": "telegram",
            "chat": "group:-1001234567890"
          },
          "session_dimensions": ["chat"]
        }
      ]
    }
  },
  "session": {
    "dimensions": ["chat"]
  }
}
```

Dans l'exemple ci-dessus, la règle VIP doit apparaître avant la règle de groupe plus large. Le routage étant strictement ordonné, placez les règles les plus spécifiques en premier et les règles de repli plus larges ensuite.

Pour des exemples plus complets de routage et de niveaux de modèles, voir le [Guide du routage](routing-guide.md).

### Liste d'autorisation des outils par agent

Les déclarations d'outils par agent se trouvent dans le frontmatter de `AGENT.md`, pas dans `config.json`.

Si `tools` est absent du frontmatter, l'agent reçoit l'ensemble normal des outils activés globalement. Si `tools` est présent, NeuroClaw n'enregistre pour cet agent que les outils d'exécution listés.

```md
---
name: Research Agent
description: Specialist for web research and in-depth analysis.
tools: [read_file, write_file, web_search, web_fetch, message]
skills: [deep-research]
mcpServers: [web-index]
---

You are the research agent.
```

Remarques :

- C'est une liste d'autorisation, pas une simple préférence.
- Les noms d'outils sont comparés 1:1 aux noms d'outils d'exécution.
- Utilisez les noms d'outils d'exécution tels que `web_search`, `web_fetch`, `spawn`, `subagent`, `send_file`.
- Les déclarations d'outils dans `AGENT.md` sont utilisées par le runtime et l'outillage, mais elles ne sont pas injectées dans le prompt de découverte.

### Découverte des agents (automatique)

Lorsqu'un agent a des pairs qu'il peut lancer et peut appeler `spawn`, NeuroClaw injecte un registre d'agents structuré dans le prompt système de cet agent à chaque tour. Aucun appel supplémentaire à un outil `list_agents` n'est nécessaire.

Ce registre rend la délégation concrète et fiable, en particulier avec `spawn` et un `agent_id` cible.

Chaque entrée contient :

| Champ | Signification |
|-------|---------------|
| `id` | Identifiant d'agent stable |
| `name` | Nom d'identité de l'agent, issu du frontmatter de `AGENT.md` |
| `description` | Description d'identité de l'agent, issue du frontmatter de `AGENT.md` |

Comportement important :

- La section de découverte n'apparaît que si l'agent courant dispose de l'outil `spawn`, et elle ne contient que les agents pairs qu'il est autorisé à lancer via `subagents.allow_agents`.
- L'agent courant et les pairs non lançables sont omis, afin que le modèle ne planifie pas avec des agents indisponibles.
- La découverte est volontairement légère. Elle ne donne au modèle que l'identité nécessaire pour choisir un pair : `id`, `name` et `description`.
- `config.json` reste la couche d'infrastructure : workspace, choix de l'agent par défaut, routage et permissions des subagents. Ces permissions contrôlent aussi la visibilité dans la découverte.
- `AGENT.md` reste la couche d'identité. Le code runtime/outillage peut toujours utiliser ses champs `tools`, `skills`, `mcpServers` et `model` lors d'une délégation.

Exemple de forme injectée :

```json
{
  "agents": [
    {
      "id": "research",
      "name": "Research Agent",
      "description": "Specialist for long-form investigation and web work."
    }
  ]
}
```

En pratique, un agent généraliste peut choisir un pair d'après la description de son rôle, puis appeler `spawn` avec l'`agent_id` du pair. Le runtime résout le reste.

### 🔒 Sandbox de Sécurité

NeuroClaw s'exécute dans un environnement sandboxé par défaut. L'agent ne peut accéder aux fichiers et exécuter des commandes que dans le workspace configuré.

#### Configuration par Défaut

```json
{
  "agents": {
    "defaults": {
      "workspace": "~/.neuroclaw/workspace",
      "restrict_to_workspace": true
    }
  }
}
```

| Option                  | Par défaut              | Description                                       |
| ----------------------- | ----------------------- | ------------------------------------------------- |
| `workspace`             | `~/.neuroclaw/workspace` | Répertoire de travail de l'agent                  |
| `restrict_to_workspace` | `true`                  | Restreindre l'accès fichiers/commandes au workspace |

#### Outils Protégés

Lorsque `restrict_to_workspace: true`, les outils suivants sont sandboxés :

| Outil         | Fonction              | Restriction                                    |
| ------------- | --------------------- | ---------------------------------------------- |
| `read_file`   | Lire des fichiers     | Uniquement les fichiers dans le workspace      |
| `write_file`  | Écrire des fichiers   | Uniquement les fichiers dans le workspace      |
| `list_dir`    | Lister les répertoires| Uniquement les répertoires dans le workspace   |
| `edit_file`   | Modifier des fichiers | Uniquement les fichiers dans le workspace      |
| `append_file` | Ajouter aux fichiers  | Uniquement les fichiers dans le workspace      |
| `exec`        | Exécuter des commandes| Les chemins de commande doivent être dans le workspace |

#### Protection Exec Supplémentaire

Même avec `restrict_to_workspace: false`, l'outil `exec` bloque ces commandes dangereuses :

* `rm -rf`, `del /f`, `rmdir /s` — Suppression en masse
* `format`, `mkfs`, `diskpart` — Formatage de disque
* `dd if=` — Imagerie de disque
* Écriture vers `/dev/sd[a-z]` — Écritures directes sur disque
* `shutdown`, `reboot`, `poweroff` — Arrêt du système
* Fork bomb `:(){ :|:& };:`
* Entre autres : `sudo`, `chmod`/`chown`, `kill`/`pkill`, `curl ... | sh`, `git push`, `docker run`, les installations de paquets et la substitution de commandes (`$(...)`, backticks). La liste complète est `defaultDenyPatterns` dans `pkg/tools/shell.go`.

### Contrôle d'Accès aux Fichiers

| Clé de configuration | Type | Par défaut | Description |
|----------------------|------|------------|-------------|
| `tools.allow_read_paths` | string[] | `[]` | Chemins supplémentaires autorisés en lecture en dehors du workspace |
| `tools.allow_write_paths` | string[] | `[]` | Chemins supplémentaires autorisés en écriture en dehors du workspace |
| `tools.message.media_enabled` | bool | `false` | Autorise l'outil `message` à joindre des fichiers média locaux par chemin. Distinct de `tools.send_file.enabled` ; à activer seulement si une livraison unifiée texte/média/légende est voulue. |

### Mode de lecture de fichier

`read_file` a deux implémentations mutuellement exclusives, choisies par la configuration. NeuroClaw n'en enregistre qu'une seule au démarrage :

| Clé de configuration | Type | Par défaut | Description |
|----------------------|------|------------|-------------|
| `tools.read_file.enabled` | bool | `true` | Active l'outil `read_file` |
| `tools.read_file.mode` | string | `bytes` | Choisit l'implémentation de `read_file` : `bytes` ou `lines` |
| `tools.read_file.max_read_file_size` | int | `65536` | Nombre maximal d'octets renvoyés par `read_file` |

#### Mode : `bytes`

Optimisé pour des fichiers quelconques et une pagination sûre pour le binaire.

Paramètres :

* `path` (obligatoire) : chemin du fichier
* `offset` (facultatif) : décalage d'octet de départ, `0` par défaut
* `length` (facultatif) : nombre maximal d'octets à lire, `max_read_file_size` par défaut

Utilisez `bytes` quand :

* vous pouvez avoir à lire des fichiers binaires
* vous voulez une pagination déterministe par plages d'octets

#### Mode : `lines`

Comportement orienté texte, optimisé pour les fichiers source, le markdown, les logs et les configs. L'outil lit séquentiellement ligne par ligne et s'arrête lorsque le budget d'octets configuré est atteint.

Paramètres :

* `path` (obligatoire) : chemin du fichier
* `start_line` (facultatif) : numéro de ligne de départ, indexé à partir de 1 et inclusif, `1` par défaut
* `max_lines` (facultatif) : nombre maximal de lignes à lire, par défaut toutes les lignes restantes jusqu'à la fin du fichier ou jusqu'au budget d'octets

Remarques sur le comportement :

* Les fichiers d'apparence binaire sont rejetés, avec l'indication de passer `read_file` en `mode = bytes`
* Les lignes isolées extrêmement longues sont tronquées plutôt qu'ignorées

Utilisez `mode = lines` quand :

* l'agent lit surtout des fichiers texte
* vous voulez une pagination par lignes dans les prompts et les appels d'outils
* vous voulez des blocs plus propres pour la revue de code, les logs et la documentation

#### Exemple

```json
{
  "tools": {
    "read_file": {
      "enabled": true,
      "mode": "lines",
      "max_read_file_size": 65536
    }
  }
}
```

### Sécurité Exec

| Clé de configuration | Type | Par défaut | Description |
|----------------------|------|------------|-------------|
| `tools.exec.allow_remote` | bool | `true` | Autoriser l'outil exec depuis les canaux distants (Telegram/Discord etc.). Mettre `false` pour le limiter aux canaux internes |
| `tools.exec.enable_deny_patterns` | bool | `true` | Activer l'interception des commandes dangereuses |
| `tools.exec.custom_deny_patterns` | string[] | `[]` | Patterns regex personnalisés à bloquer |
| `tools.exec.custom_allow_patterns` | string[] | `[]` | Patterns regex personnalisés à autoriser |

> **Étiquettes de risque :** `exec` et les commandes shell de cron sont activés par défaut et chacun est signalé au démarrage (`NeuroClaw risk=shell`, `NeuroClaw risk=cron_shell`). L'installation de skills et les outils de bus matériel ont aussi leurs étiquettes. Voir [Capabilities §2](../reference/capabilities.md) pour la liste, ce que chacune permet au modèle et comment la désactiver. Associez `exec` à un `allow_from` non vide sur chaque canal ([Applications de chat](chat-apps.fr.md)).

> **Note de sécurité :** La protection Symlink est activée par défaut — tous les chemins de fichiers sont résolus via `filepath.EvalSymlinks` avant la correspondance avec la liste blanche, empêchant les attaques d'évasion par symlink.

#### Limitation Connue : Processus Enfants des Outils de Build

Le garde de sécurité exec n'inspecte que la ligne de commande lancée directement par NeuroClaw. Il n'inspecte pas récursivement les processus enfants générés par les outils de développement autorisés tels que `make`, `go run`, `cargo`, `npm run` ou les scripts de build personnalisés.

Cela signifie qu'une commande de niveau supérieur peut toujours compiler ou lancer d'autres binaires après avoir passé la vérification initiale du garde. En pratique, traitez les scripts de build, les Makefiles, les scripts de packages et les binaires générés comme du code exécutable nécessitant le même niveau de revue qu'une commande shell directe.

Pour les environnements à haut risque :

* Examinez les scripts de build avant l'exécution.
* Préférez l'approbation/revue manuelle pour les workflows de compilation et d'exécution.
* Exécutez NeuroClaw dans un conteneur ou une VM si vous avez besoin d'une isolation plus forte que celle fournie par le garde intégré.

#### Exemples d'Erreurs

```
[ERROR] tool: Tool execution failed
{tool=exec, error=Command blocked by safety guard (path outside working dir)}
```

```
[ERROR] tool: Tool execution failed
{tool=exec, error=Command blocked by safety guard (dangerous pattern detected)}
```

#### Désactiver les Restrictions (Risque de Sécurité)

Si vous avez besoin que l'agent accède à des chemins en dehors du workspace :

**Méthode 1 : Fichier de configuration**

```json
{
  "agents": {
    "defaults": {
      "restrict_to_workspace": false
    }
  }
}
```

**Méthode 2 : Variable d'environnement**

```bash
export NEUROCLAW_AGENTS_DEFAULTS_RESTRICT_TO_WORKSPACE=false
```

> ⚠️ **Avertissement** : Désactiver cette restriction permet à l'agent d'accéder à n'importe quel chemin sur votre système. À utiliser avec précaution dans des environnements contrôlés uniquement.

#### Cohérence des Limites de Sécurité

Le paramètre `restrict_to_workspace` s'applique de manière cohérente à tous les chemins d'exécution :

| Chemin d'exécution | Limite de sécurité               |
| ------------------ | -------------------------------- |
| Main Agent         | `restrict_to_workspace` ✅       |
| Subagent / Spawn   | Hérite de la même restriction ✅ |
| Heartbeat tasks    | Hérite de la même restriction ✅ |

Tous les chemins partagent la même restriction de workspace — il n'y a aucun moyen de contourner la limite de sécurité via les subagents ou les tâches planifiées.

### Heartbeat (Tâches Périodiques)

NeuroClaw peut effectuer des tâches périodiques automatiquement. Créez un fichier `HEARTBEAT.md` dans votre workspace :

```markdown
# Periodic Tasks

- Check my email for important messages
- Review my calendar for upcoming events
- Check the weather forecast
```

L'agent lira ce fichier toutes les 30 minutes (configurable) et exécutera toutes les tâches en utilisant les outils disponibles.

#### Tâches Asynchrones avec Spawn

Pour les tâches longues (recherche web, appels API), utilisez l'outil `spawn` pour créer un **subagent** :

```markdown
# Tâches Périodiques

## Tâches Rapides (répondre directement)

- Indiquer l'heure actuelle

## Tâches Longues (utiliser spawn pour l'asynchrone)

- Rechercher les actualités IA sur le web et résumer
- Vérifier les e-mails et signaler les messages importants
```

**Comportements clés :**

| Fonctionnalité   | Description                                                        |
| ---------------- | ------------------------------------------------------------------ |
| **spawn**        | Crée un subagent asynchrone, ne bloque pas le heartbeat            |
| **Contexte indépendant** | Le subagent a son propre contexte, sans historique de session |
| **message tool** | Le subagent communique directement avec l'utilisateur              |
| **Non-bloquant** | Après le spawn, le heartbeat continue vers la tâche suivante       |

#### Flux de Communication du Subagent

```
Heartbeat déclenché
    ↓
Agent lit HEARTBEAT.md
    ↓
Tâche longue : spawn subagent
    ↓                           ↓
Continue tâche suivante    Subagent travaille indépendamment
    ↓                           ↓
Toutes tâches terminées    Subagent utilise "message" tool
    ↓                           ↓
Répond HEARTBEAT_OK        Utilisateur reçoit le résultat
```

Le subagent a accès aux outils (message, web_search, etc.) et peut communiquer avec l'utilisateur de façon indépendante, sans passer par l'agent principal.

**Configuration :**

```json
{
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

| Option     | Défaut | Description                              |
| ---------- | ------ | ---------------------------------------- |
| `enabled`  | `true` | Activer/désactiver le heartbeat          |
| `interval` | `30`   | Intervalle en minutes (minimum : 5)      |

**Variables d'environnement :**

* `NEUROCLAW_HEARTBEAT_ENABLED=false` pour désactiver
* `NEUROCLAW_HEARTBEAT_INTERVAL=60` pour changer l'intervalle

### Providers

> [!NOTE]
> Groq fournit une transcription vocale gratuite via Whisper. Si configuré, les messages audio de n'importe quel canal seront automatiquement transcrits au niveau de l'agent.

| Provider     | Usage                                   | Obtenir une clé API                                          |
| ------------ | --------------------------------------- | ------------------------------------------------------------ |
| `gemini`     | LLM (Gemini direct)                     | [aistudio.google.com](https://aistudio.google.com)           |
| `zhipu`      | LLM (Zhipu direct)                      | [bigmodel.cn](https://bigmodel.cn)                           |
| `volcengine` | LLM (Volcengine direct)                 | [volcengine.com](https://www.volcengine.com) |
| `openrouter` | LLM (recommandé, accès à tous modèles)  | [openrouter.ai](https://openrouter.ai)                       |
| `anthropic`  | LLM (Claude direct)                     | [console.anthropic.com](https://console.anthropic.com)       |
| `openai`     | LLM (GPT direct)                        | [platform.openai.com](https://platform.openai.com)           |
| `deepseek`   | LLM (DeepSeek direct)                   | [platform.deepseek.com](https://platform.deepseek.com)       |
| `qwen`       | LLM (Qwen direct)                       | [dashscope.console.aliyun.com](https://dashscope.console.aliyun.com) |
| `groq`       | LLM + **Transcription vocale** (Whisper)| [console.groq.com](https://console.groq.com)                 |
| `cerebras`   | LLM (Cerebras direct)                   | [cerebras.ai](https://cerebras.ai)                           |
| `vivgrid`    | LLM (Vivgrid direct)                    | [vivgrid.com](https://vivgrid.com)                           |

### Configuration des Modèles (model_list)

> NeuroClaw préfère la configuration explicite `provider` + `model` natif (par exemple `"provider": "zhipu", "model": "glm-4.7"`). L'ancienne forme à champ unique `provider/model` reste prise en charge par compatibilité quand `provider` est omis.

Cette conception permet aussi le **support multi-agent** avec une sélection de provider flexible :

- **Agents différents, providers différents** : chaque agent peut utiliser son propre provider LLM
- **Modèles de repli** : configurez des modèles principal et de secours pour la résilience
- **Répartition de charge** : répartissez les requêtes entre plusieurs endpoints ou clés
- **Configuration centralisée** : gérez tous les providers au même endroit
- **Activation/désactivation de modèle** : utilisez le champ `enabled` pour désactiver temporairement un modèle sans supprimer sa configuration

#### 🔒 Configuration de sécurité (recommandé)

NeuroClaw permet de séparer les données sensibles (clés API, jetons, secrets) de votre configuration principale en les stockant dans un fichier `.security.yml`.

**Avantages principaux :**
- **Sécurité** : les données sensibles ne sont jamais dans votre fichier de configuration principal
- **Partage facile** : partagez config.json sans exposer vos clés API
- **Gestion de versions** : ajoutez `.security.yml` à `.gitignore`
- **Déploiement flexible** : chaque environnement peut utiliser des fichiers de sécurité différents

**Mise en place rapide :**

1. Créez `~/.neuroclaw/.security.yml` avec vos clés API :
```yaml
model_list:
  gpt-5.4:
    api_keys:
      - "sk-proj-your-actual-openai-key"
  claude-sonnet-4.6:
    api_keys:
      - "sk-ant-your-actual-anthropic-key"
channel_list:
  telegram:
    token: "your-telegram-bot-token"
web:
  brave:
    api_keys:
      - "BSAyour-brave-api-key"
  glm_search:
    api_key: "your-glm-search-api-key"
```

2. Définissez les bonnes permissions :
```bash
chmod 600 ~/.neuroclaw/.security.yml
```

3. Retirez les champs sensibles de `config.json` (recommandé) :
```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4"
      // api_key loaded from .security.yml
    }
  ],
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      // token loaded from .security.yml
    }
  }
}
```

**Fonctionnement :**
- Les valeurs de `.security.yml` sont automatiquement associées aux champs de configuration
- Aucune syntaxe spéciale : omettez simplement les champs sensibles de config.json
- Si un champ existe dans les deux fichiers, la valeur de `.security.yml` est prioritaire
- Vous pouvez mélanger des valeurs directes dans config.json et des valeurs de sécurité

Pour la documentation complète, voir [`../security/security_configuration.md`](../security/security_configuration.md).

#### Tous les Vendors Supportés

| Vendor                  | Valeur `provider` | API Base par défaut                                    | Protocole | API Key                                                          |
| ----------------------- | ----------------- | --------------------------------------------------- | --------- | ---------------------------------------------------------------- |
| **OpenAI**              | `openai`          | `https://api.openai.com/v1`                         | OpenAI    | [Obtenir](https://platform.openai.com)                           |
| **Anthropic**           | `anthropic`       | `https://api.anthropic.com/v1`                      | Anthropic | [Obtenir](https://console.anthropic.com)                         |
| **智谱 AI (GLM)**       | `zhipu`           | `https://open.bigmodel.cn/api/paas/v4`              | OpenAI    | [Obtenir](https://open.bigmodel.cn/usercenter/proj-mgmt/apikeys) |
| **DeepSeek**            | `deepseek`        | `https://api.deepseek.com/v1`                       | OpenAI    | [Obtenir](https://platform.deepseek.com)                         |
| **Google Gemini**       | `gemini`          | `https://generativelanguage.googleapis.com/v1beta`  | Gemini    | [Obtenir](https://aistudio.google.com/api-keys)                  |
| **Groq**                | `groq`            | `https://api.groq.com/openai/v1`                    | OpenAI    | [Obtenir](https://console.groq.com)                              |
| **Moonshot**            | `moonshot`        | `https://api.moonshot.cn/v1`                        | OpenAI    | [Obtenir](https://platform.moonshot.cn)                          |
| **通义千问 (Qwen)**     | `qwen`            | `https://dashscope.aliyuncs.com/compatible-mode/v1` | OpenAI    | [Obtenir](https://dashscope.console.aliyun.com)                  |
| **NVIDIA**              | `nvidia`          | `https://integrate.api.nvidia.com/v1`               | OpenAI    | [Obtenir](https://build.nvidia.com)                              |
| **Ollama**              | `ollama`          | `http://localhost:11434/v1`                         | OpenAI    | Local (pas de clé)                                            |
| **LM Studio**           | `lmstudio`        | `http://localhost:1234/v1`                          | OpenAI    | Facultative (local : pas de clé par défaut)                                 |
| **OpenRouter**          | `openrouter`      | `https://openrouter.ai/api/v1`                      | OpenAI    | [Obtenir](https://openrouter.ai/keys)                            |
| **LiteLLM Proxy**       | `litellm`         | `http://localhost:4000/v1`                          | OpenAI    | Votre clé de proxy LiteLLM                                           |
| **VLLM**                | `vllm`            | `http://localhost:8000/v1`                          | OpenAI    | Local                                                            |
| **Cerebras**            | `cerebras`        | `https://api.cerebras.ai/v1`                        | OpenAI    | [Obtenir](https://cerebras.ai)                                   |
| **VolcEngine (Doubao)** | `volcengine`      | `https://ark.cn-beijing.volces.com/api/v3`          | OpenAI    | [Obtenir](https://www.volcengine.com) |
| **神算云**              | `shengsuanyun`    | `https://router.shengsuanyun.com/api/v1`            | OpenAI    | —                                                                |
| **BytePlus**            | `byteplus`        | `https://ark.ap-southeast.bytepluses.com/api/v3`    | OpenAI    | [Obtenir](https://www.byteplus.com)                              |
| **Vivgrid**             | `vivgrid`         | `https://api.vivgrid.com/v1`                        | OpenAI    | [Obtenir](https://vivgrid.com)                                   |
| **LongCat**             | `longcat`         | `https://api.longcat.chat/openai`                   | OpenAI    | [Obtenir](https://longcat.chat/platform)                         |
| **ModelScope (魔搭)**   | `modelscope`      | `https://api-inference.modelscope.cn/v1`            | OpenAI    | [Obtenir le jeton](https://modelscope.cn/my/tokens)                     |
| **Antigravity**         | `antigravity`     | Google Cloud                                        | Custom    | OAuth uniquement                                                       |
| **GitHub Copilot**      | `github-copilot`  | `localhost:4321`                                    | gRPC      | —                                                                |

#### Configuration de Base

```json
{
  "model_list": [
    {
      "model_name": "ark-code-latest",
      "provider": "volcengine",
      "model": "ark-code-latest",
      "api_keys": ["sk-your-api-key"]
    },
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_keys": ["sk-your-openai-key"]
    },
    {
      "model_name": "claude-sonnet-4.6",
      "provider": "anthropic",
      "model": "claude-sonnet-4.6",
      "api_keys": ["sk-ant-your-key"]
    },
    {
      "model_name": "glm-4.7",
      "provider": "zhipu",
      "model": "glm-4.7",
      "api_keys": ["your-zhipu-key"]
    }
  ],
  "agents": {
    "defaults": {
      "model_name": "gpt-5.4"
    }
  }
}
```

> **Note de sécurité** : vous pouvez retirer les champs `api_keys` de votre configuration et les stocker dans `.security.yml`. Voir [Configuration de sécurité](#-configuration-de-sécurité-recommandé) ci-dessus.
>
> **Note** : le champ `enabled` peut être mis à `false` pour désactiver une entrée de modèle sans la supprimer. S'il est omis, il vaut `true` lors de la migration pour les modèles qui ont des clés API.

Règles de résolution :

- Préférez l'écriture explicite `"provider": "openai", "model": "gpt-5.4"`.
- Si `provider` est défini, NeuroClaw envoie `model` tel quel.
- Si `provider` est omis, NeuroClaw traite le premier segment avant `/` de `model` comme le provider et tout ce qui suit ce premier `/` comme l'ID de modèle d'exécution.
- Ainsi, `"model": "openrouter/openai/gpt-5.4"` fonctionne toujours comme forme de compatibilité et envoie `openai/gpt-5.4` à OpenRouter.

#### Configuration du Streaming

Le streaming provider utilise un double opt-in et est désactivé par défaut. L'agent ne tente le streaming que lorsque le channel courant a `settings.streaming.enabled: true`, que l'entrée de modèle active a `streaming.enabled: true`, et que le provider comme le channel prennent en charge le streaming. Si une condition manque, NeuroClaw utilise le chemin de requête non-streaming normal.

Pico WebUI est le premier channel entièrement câblé. Pico crée le premier message assistant avec le message wire existant `message.create`, puis met à jour ce même message avec `message.update`; aucun nouveau type de message Pico n'est introduit.

Laissez `streaming` absent si vous ne voulez pas de streaming. Un bloc `streaming` omis signifie désactivé; il n'est pas nécessaire d'écrire `"streaming": {"enabled": false}`.

Exemple d'activation :

```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_keys": ["sk-your-openai-key"],
      "streaming": {
        "enabled": true
      }
    }
  ],
  "channel_list": {
    "pico": {
      "enabled": true,
      "type": "pico",
      "settings": {
        "token": "YOUR_PICO_TOKEN",
        "streaming": {
          "enabled": true
        }
      }
    }
  }
}
```

| Champ | Type | Défaut | Description |
| ----- | ---- | ------ | ----------- |
| `channel_list.<name>.settings.streaming.enabled` | bool | `false` | Autorise ce channel à afficher la sortie streaming du provider |
| `channel_list.<name>.settings.streaming.throttle_seconds` | int | Défaut Pico après activation : `0` | Intervalle minimal entre les mises à jour intermédiaires; le contenu final est toujours envoyé |
| `channel_list.<name>.settings.streaming.min_growth_chars` | int | Défaut Pico après activation : `1` | Croissance minimale du texte avant une mise à jour intermédiaire; le contenu final est toujours envoyé |
| `model_list[].streaming.enabled` | bool | `false` | Autorise cette entrée de modèle à tenter des requêtes provider en streaming |

Les anciennes variables d'environnement Telegram restent compatibles : `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_ENABLED`, `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_THROTTLE_SECONDS` et `NEUROCLAW_CHANNELS_TELEGRAM_STREAMING_MIN_GROWTH_CHARS`. Elles s'appliquent uniquement aux settings Telegram et n'activent ni ne modifient `settings.streaming` de Pico.

Le comportement d'échec est volontairement conservateur : si le streaming échoue avant l'envoi d'un chunk visible, NeuroClaw réessaie une fois via le chemin `Chat()` normal. Si un chunk a déjà été affiché à l'utilisateur, NeuroClaw n'envoie pas une deuxième réponse non-streaming, afin d'éviter une sortie dupliquée.

Pour les champs de requête TTS propres à un modèle, comme des noms de `voice` personnalisés ou `response_format: "mp3"`, utilisez `model_list[].extra_body`.

#### Exemples par Vendor

> **Astuce** : vous pouvez omettre les champs `api_key` et les stocker dans `.security.yml` pour plus de sécurité. Voir [Configuration de sécurité](#-configuration-de-sécurité-recommandé).

<details>
<summary><b>OpenAI</b></summary>

```json
{
  "model_name": "gpt-5.4",
  "provider": "openai",
  "model": "gpt-5.4"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>OpenRouter TTS (MAI Voice 2)</b></summary>

```json
{
  "model_name": "mai-voice-2",
  "provider": "openrouter",
  "model": "microsoft/mai-voice-2",
  "api_base": "https://openrouter.ai/api/v1",
  "extra_body": {
    "voice": "en-US-Harper:MAI-Voice-2",
    "response_format": "mp3"
  }
  // api_key: set in .security.yml
}
```

Associez-le à :

```json
{
  "voice": {
    "tts_model_name": "mai-voice-2"
  }
}
```

</details>

<details>
<summary><b>VolcEngine (Doubao)</b></summary>

```json
{
  "model_name": "ark-code-latest",
  "provider": "volcengine",
  "model": "ark-code-latest"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>智谱 AI (GLM)</b></summary>

```json
{
  "model_name": "glm-4.7",
  "provider": "zhipu",
  "model": "glm-4.7"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>DeepSeek</b></summary>

```json
{
  "model_name": "deepseek-chat",
  "provider": "deepseek",
  "model": "deepseek-chat"
  // api_key: set in .security.yml
}
```

</details>

<details>
<summary><b>Anthropic</b></summary>

```json
{
  "model_name": "claude-sonnet-4.6",
  "provider": "anthropic",
  "model": "claude-sonnet-4.6"
  // api_key: set in .security.yml
}
```

> Exécutez `neuroclaw auth login --provider anthropic` pour coller votre jeton API.

Pour un accès direct à l'API Anthropic ou des endpoints personnalisés qui ne prennent en charge que le format de messages natif d'Anthropic :

```json
{
  "model_name": "claude-opus-4-6",
  "provider": "anthropic-messages",
  "model": "claude-opus-4-6",
  "api_keys": ["sk-ant-your-key"],
  "api_base": "https://api.anthropic.com"
}
```

> Utilisez `anthropic-messages` lorsque l'endpoint exige le format natif `/v1/messages` d'Anthropic au lieu du format compatible OpenAI `/v1/chat/completions`.

</details>

<details>
<summary><b>Ollama (local)</b></summary>

```json
{
  "model_name": "llama3",
  "provider": "ollama",
  "model": "llama3"
}
```

</details>

<details>
<summary><b>LM Studio (local)</b></summary>

```json
{
  "model_name": "lmstudio-local",
  "provider": "lmstudio",
  "model": "openai/gpt-oss-20b"
}
```

`api_base` vaut par défaut `http://localhost:1234/v1`. La clé API est facultative, sauf si votre serveur LM Studio active l'authentification.<br/>
Avec un `provider` explicite, NeuroClaw envoie `openai/gpt-oss-20b` tel quel à LM Studio. L'ancienne forme de compatibilité `"model": "lmstudio/openai/gpt-oss-20b"` se résout toujours vers le même ID de modèle amont lorsque `provider` est omis.

</details>

<details>
<summary><b>Proxy personnalisé / LiteLLM</b></summary>

```json
{
  "model_name": "my-custom-model",
  "provider": "openai",
  "model": "custom-model",
  "api_base": "https://my-proxy.com/v1"
  // api_key: set in .security.yml
}
```

Avec un `provider` explicite, NeuroClaw envoie `model` tel quel. Ainsi, `"provider": "litellm", "model": "lite-gpt4"` envoie `lite-gpt4`, tandis que `"provider": "litellm", "model": "openai/gpt-4o"` envoie `openai/gpt-4o`. Les anciennes formes de compatibilité `litellm/lite-gpt4` et `litellm/openai/gpt-4o` se résolvent de la même façon lorsque `provider` est omis.

</details>

#### Équilibrage de Charge

Configurez plusieurs endpoints pour le même nom de modèle — NeuroClaw effectuera automatiquement un round-robin :

**Option 1 : plusieurs clés API dans .security.yml (recommandé)**

```yaml
# .security.yml
model_list:
  gpt-5.4:
    api_keys:
      - "sk-proj-key-1"
      - "sk-proj-key-2"
```

```json
// config.json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_base": "https://api.openai.com/v1"
      // api_keys loaded from .security.yml
    }
  ]
}
```

**Option 2 : plusieurs entrées de modèle**

```json
{
  "model_list": [
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_base": "https://api1.example.com/v1",
      "api_keys": ["sk-key1"]
    },
    {
      "model_name": "gpt-5.4",
      "provider": "openai",
      "model": "gpt-5.4",
      "api_base": "https://api2.example.com/v1",
      "api_keys": ["sk-key2"]
    }
  ]
}
```

#### Migration depuis l'ancienne config `providers`

L'ancienne configuration `providers` est **dépréciée** et a été supprimée dans V2. Les configs V0/V1 existantes sont auto-migrées. Voir [docs/migration/model-list-migration.md](../migration/model-list-migration.md).

### Architecture des Providers

NeuroClaw route les providers par famille de protocole :

- **Compatible OpenAI** : OpenRouter, Groq, Zhipu, endpoints vLLM et la plupart des autres.
- **Gemini natif** : Google Gemini via les endpoints natifs `models/*:generateContent` et `models/*:streamGenerateContent`.
- **Anthropic** : Comportement natif de l'API Claude.
- **Codex/OAuth** : Route d'authentification OAuth/token OpenAI.

Cela maintient le runtime léger tout en faisant des nouveaux backends compatibles OpenAI principalement une opération de configuration (`api_base` + `api_keys`).

<details>
<summary><b>Zhipu (ancien format providers)</b></summary>

```json
{
  "agents": {
    "defaults": {
      "workspace": "~/.neuroclaw/workspace",
      "model": "glm-4.7",
      "max_tokens": 8192,
      "temperature": 0.7,
      "max_tool_iterations": 20,
      "max_parallel_turns": 1
    }
  },
  "providers": {
    "zhipu": {
      "api_key": "Your API Key",
      "api_base": "https://open.bigmodel.cn/api/paas/v4"
    }
  }
}
```

> **Note** : le format `providers` est déprécié. Utilisez le nouveau format `model_list` avec `.security.yml` pour plus de sécurité.
>
> **`max_parallel_turns`** : contrôle le traitement concurrent des messages de sessions différentes. `1` (défaut) = séquentiel ; `>1` = parallèle. Les messages d'une même session sont toujours sérialisés. Voir la [documentation Steering](../architecture/steering.md).

</details>

<details>
<summary><b>Exemple de configuration complète</b></summary>

```json
{
  "agents": {
    "defaults": {
      "model_name": "claude-opus-4-5"
    }
  },
  "session": {
    "dm_scope": "per-channel-peer",
    "backlog_limit": 20
  },
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      // token: set in .security.yml
      "allow_from": ["123456789"]
    }
  },
  "tools": {
    "web": {
      "duckduckgo": {
        "enabled": true,
        "max_results": 5
      }
    }
  },
  "heartbeat": {
    "enabled": true,
    "interval": 30
  }
}
```

> **Note** : les champs sensibles (`api_key`, `token`, etc.) peuvent être omis et stockés dans `.security.yml` pour plus de sécurité.

</details>

### Journaux d'événements

NeuroClaw publie des événements d'exécution provenant de l'agent, des canaux, du gateway, du bus de messages et de MCP. Par défaut, seuls les événements `agent.*` sont écrits dans le journal ; les autres passent quand même par le bus d'événements d'exécution, mais pas par le journal.

```json
{
  "events": {
    "logging": {
      "enabled": true,
      "include": ["agent.*"],
      "exclude": [],
      "min_severity": "info",
      "include_payload": false
    }
  }
}
```

Un réglage courant :

```json
{
  "events": {
    "logging": {
      "include": ["*"],
      "exclude": ["agent.llm.delta"],
      "min_severity": "warn"
    }
  }
}
```

`include` / `exclude` acceptent des noms d'événements exacts ou des motifs comme `gateway.*` et `channel.lifecycle.*`. `include_payload` est désactivé par défaut afin que les messages complets des utilisateurs et les arguments des outils n'apparaissent pas dans le journal ; les événements de l'agent consignent à la place des champs de synthèse (longueurs, compteurs, statut).

Autres champs et exemples : [Événements d'exécution et journaux d'événements](../architecture/runtime-events.md).

### Tâches Planifiées / Rappels

NeuroClaw supporte les tâches planifiées via l'outil `cron`. L'agent peut définir, lister et annuler des rappels ou tâches récurrentes.

```json
{
  "tools": {
    "cron": {
      "enabled": true,
      "exec_timeout_minutes": 5,
      "allow_command": true,
      "command_allowed_remotes": []
    }
  }
}
```

Les tâches planifiées persistent après redémarrage dans `~/.neuroclaw/workspace/cron/`.

Les tâches de type commande peuvent exécuter des commandes shell (`allow_command` vaut `true` par défaut et est signalé au démarrage par `NeuroClaw risk=cron_shell` ; voir [Capabilities §2](../reference/capabilities.md)). Par défaut, les canaux distants ne peuvent pas planifier de tâches de commande. Pour en autoriser certains, définissez `command_allowed_remotes` avec des entrées comme `"telegram"` ou `"telegram:1234567890"` ; n'utilisez `"*"` que si tout canal non vide doit être autorisé. Le joker `"*"` est potentiellement dangereux, car tout canal distant pouvant parler à NeuroClaw peut planifier des commandes shell. Cela ne contourne pas `allow_command` (quand il vaut `false`, une tâche exige `command_confirm: true`) ni les contrôles de sécurité d'exec.

### Sujets Avancés

| Sujet | Description |
| ----- | ----------- |
| [Configuration de sécurité](../security/security_configuration.md) | Stocker les clés API et secrets dans un fichier `.security.yml` séparé |
| [Filtrage des données sensibles](../security/sensitive_data_filtering.md) | Filtrer les clés API et jetons des résultats d'outils avant l'envoi au LLM |
| [Système de Hooks](../architecture/hooks/README.md) | Hooks événementiels : observateurs, intercepteurs, hooks d'approbation |
| [Steering](../architecture/steering.md) | Injecter des messages dans une boucle agent en cours d'exécution |
| [SubTurn](../architecture/subturn.md) | Coordination de subagents, contrôle de concurrence, cycle de vie |
| [Gestion du Contexte](../architecture/agent-refactor/context.md) | Détection des limites de contexte, vérification proactive du budget, compression |
