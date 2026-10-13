# Débogage de NeuroClaw

> Retour au [README](../project/README.fr.md)

NeuroClaw effectue de multiples interactions complexes en arrière-plan pour chaque requête qu'il reçoit — du routage des messages et de l'évaluation de la complexité, à l'exécution des outils et à l'adaptation aux défaillances de modèle. Pouvoir voir exactement ce qui se passe est crucial, non seulement pour résoudre les problèmes potentiels, mais aussi pour véritablement comprendre le fonctionnement de l'agent.

## Démarrer NeuroClaw en mode débogage

Pour obtenir des informations détaillées sur ce que fait l'agent (requêtes LLM, appels d'outils, routage des messages), vous pouvez démarrer la passerelle NeuroClaw avec le drapeau de débogage :

```bash
neuroclaw gateway --debug
# or
neuroclaw gateway -d
```

Dans ce mode, le système formate les logs de manière détaillée et affiche des aperçus des prompts système et des résultats d'exécution des outils.

## Étiquettes de risque au démarrage

Que `--debug` soit activé ou non, `gateway` et `agent` consignent un avertissement `NeuroClaw risk=<name>` pour chaque capacité à risque activée (par exemple `risk=shell`). Consultez-les en premier pour savoir ce que le modèle est autorisé à faire sur cette machine. Noms et significations : [capabilities.md section 2](../reference/capabilities.md#2-risk-labels).

## Désactiver la troncature des logs (logs complets)

Par défaut, NeuroClaw tronque les chaînes très longues (comme le *Prompt Système* ou les résultats JSON volumineux) dans les logs de débogage afin de garder la console lisible.

Si vous avez besoin d'inspecter la sortie complète d'une commande ou le payload exact envoyé au modèle LLM, vous pouvez utiliser le drapeau `--no-truncate`.

**Remarque :** Ce drapeau fonctionne *uniquement* en combinaison avec le mode `--debug`.

```bash
neuroclaw gateway --debug --no-truncate

```

Lorsque ce drapeau est actif, la fonction de troncature globale est désactivée. Cela est extrêmement utile pour :

* Vérifier la syntaxe exacte des messages envoyés au fournisseur.
* Lire la sortie complète d'outils comme `exec`, `web_fetch` ou `read_file`.
* Déboguer l'historique de session sauvegardé en mémoire.

## Visibilité des appels d'outils dans les logs de débogage

Lorsque le mode débogage est actif, l'agent émet des entrées de log structurées à chaque étape du cycle d'exécution d'un outil. Ces entrées portent le label `component=agent` et utilisent le niveau `INFO` ou `DEBUG` selon le niveau de détail :

| Message de log | Niveau | Champs clés | Description |
|---|---|---|---|
| `LLM requested tool calls` | INFO | `tools`, `count`, `iteration` | Liste des noms d'outils que le modèle a décidé d'appeler |
| `Tool call: <name>(<args>)` | INFO | `tool`, `iteration` | Le nom de l'outil et un aperçu de ses arguments (tronqué à 200 caractères) |
| `Sent tool result to user` | DEBUG | `tool`, `content_len` | Émis lorsqu'un résultat d'outil est transmis au canal de chat |
| `TTL tick after tool execution` | DEBUG | `agent_id`, `iteration` | Décrément du TTL de découverte d'outils MCP après chaque tour d'outils |
| `Async tool completed, publishing result` | INFO | `tool`, `content_len`, `channel` | Uniquement pour les outils qui s'exécutent de façon asynchrone en arrière-plan |

### Lire une entrée de log d'appel d'outil

Un appel d'outil synchrone typique produit deux lignes consécutives dans la console :

```
[...] [INFO] agent: LLM requested tool calls {tools=[web_search], count=1, iteration=1}
[...] [INFO] agent: Tool call: web_search({"query":"neuroclaw release notes"}) {tool=web_search, iteration=1}
```

L'aperçu des arguments est plafonné à **200 caractères** dans les logs, quel que soit le drapeau `--no-truncate`, car il relève du chemin de niveau `INFO`. Utilisez `--no-truncate` avec `--debug` pour voir le champ complet `tools_json` émis par l'entrée DEBUG `Full LLM request`, qui contient chaque définition d'outil envoyée au modèle.

## Retour d'outils en temps réel dans le chat (tool_feedback)

Les logs de débogage restent côté serveur. Si vous voulez que l'agent envoie une notification visible directement dans le canal de chat à chaque exécution d'un outil — utile lorsque le bot est partagé avec d'autres utilisateurs ou par souci de transparence — activez la fonctionnalité `tool_feedback` dans `config.json` :

```json
{
  "agents": {
    "defaults": {
      "tool_feedback": {
        "enabled": true,
        "max_args_length": 300,
        "separate_messages": true
      }
    }
  }
}
```

Lorsque `enabled` vaut `true`, chaque appel d'outil envoie un court message au chat avant que le résultat de l'outil soit renvoyé au modèle. Le message ressemble à ceci :

```bash
🔧 `web_search`
{"query": "neuroclaw release notes"}
```

### Options

| Champ | Type | Par défaut | Description |
|---|---|---|---|
| `enabled` | bool | `false` | Envoyer une notification de chat pour chaque appel d'outil |
| `separate_messages` | bool | `false` | Conserver chaque mise à jour de retour d'outil comme un message de chat distinct au lieu de réutiliser un unique message d'espace réservé/de progression |
| `max_args_length` | int | `300` | Nombre maximal de caractères des arguments sérialisés inclus dans la notification |

### Variables d'environnement

Les deux champs peuvent aussi être définis via des variables d'environnement :

```bash
NEUROCLAW_AGENTS_DEFAULTS_TOOL_FEEDBACK_ENABLED=true
NEUROCLAW_AGENTS_DEFAULTS_TOOL_FEEDBACK_MAX_ARGS_LENGTH=300
```

> **Remarque :** `tool_feedback` est indépendant du mode `--debug`. Il fonctionne en production et ne nécessite aucun drapeau particulier au démarrage de la passerelle.
