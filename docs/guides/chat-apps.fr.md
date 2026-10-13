# 💬 Configuration des Applications de Chat

> Retour au [README](../project/README.fr.md)

## 💬 Applications de Chat

Communiquez avec votre NeuroClaw via Telegram, Discord, WhatsApp, Matrix, QQ, DingTalk, LINE, WeCom, Feishu, Slack, IRC, OneBot, MQTT, MaixCam ou Pico (protocole natif).

> **Note** : Tous les canaux basés sur les webhooks (LINE, WeCom, etc.) sont servis sur un seul serveur HTTP Gateway partagé (`gateway.host`:`gateway.port`, par défaut `127.0.0.1:18790`). Il n'y a pas de ports par canal à configurer. Note : Feishu, DingTalk et WeCom utilisent des modes socket/stream et n'utilisent pas le serveur HTTP webhook partagé pour la réception.

> **Renseignez `allow_from` sur chaque canal.** Un `allow_from` vide laisse entrer toute personne pouvant joindre le bot (la passerelle journalise seulement l'avertissement `SECURITY: Channel allows EVERYONE`). Indiquez votre propre identifiant utilisateur, ou utilisez `["*"]` pour dire explicitement qu'un accès ouvert est voulu. Plusieurs exemples ci-dessous montrent `"allow_from": []` par souci de brièveté ; ne le laissez pas vide sur un bot capable d'exécuter des outils. Ce qu'un utilisateur de chat peut faire faire à l'agent (shell, commandes planifiées, installation de skills) est listé avec ses étiquettes de risque dans [Capabilities §2](../reference/capabilities.md).

| Canal                | Difficulté         | Description                                           | Documentation                                                                                                    |
| -------------------- | ------------------ | ----------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| **Telegram**         | ⭐ Facile          | Recommandé, transcription vocale, long polling (pas d'IP publique requise) | [Documentation](../channels/telegram/README.fr.md)                                                  |
| **Discord**          | ⭐ Facile          | WebSocket Gateway (pas d'IP publique requise), groupes/DM | [Documentation](../channels/discord/README.fr.md)                                                               |
| **WhatsApp**         | ⭐ Facile          | Natif (scan QR) ou Bridge URL                         | [Documentation](#whatsapp)                                                                                       |
| **Weixin**           | ⭐ Facile          | Scan QR natif (API Tencent iLink)                     | [Documentation](#weixin)                                                                                         |
| **Slack**            | ⭐ Facile          | **Socket Mode** (pas d'IP publique requise), entreprise | [Documentation](../channels/slack/README.fr.md)                                                                |
| **Matrix**           | ⭐⭐ Moyen         | Protocole fédéré, auto-hébergement possible           | [Documentation](../channels/matrix/README.fr.md)                                                                |
| **QQ**               | ⭐⭐ Moyen         | API bot officielle, communauté chinoise               | [Documentation](../channels/qq/README.fr.md)                                                                    |
| **DingTalk**         | ⭐⭐ Moyen         | Mode Stream (pas d'IP publique requise), entreprise   | [Documentation](../channels/dingtalk/README.fr.md)                                                              |
| **LINE**             | ⭐⭐⭐ Avancé      | HTTPS Webhook requis                                  | [Documentation](../channels/line/README.fr.md)                                                                  |
| **WeCom (企业微信)** | ⭐⭐⭐ Avancé      | AI Bot officiel via WebSocket, streaming + médias     | [Guide](../channels/wecom/README.fr.md) |
| **Feishu (飞书)**    | ⭐⭐⭐ Avancé      | Collaboration entreprise, fonctionnalités riches      | [Documentation](../channels/feishu/README.fr.md)                                                                |
| **IRC**              | ⭐⭐ Moyen         | Serveur + configuration TLS                           | [Documentation](#irc) |
| **OneBot**           | ⭐⭐ Moyen         | Compatible NapCat/Go-CQHTTP, écosystème communautaire | [Documentation](../channels/onebot/README.fr.md)                                                                |
| **MQTT**             | ⭐ Facile          | N'importe quel client MQTT via broker pub/sub         | [Documentation](../channels/mqtt/README.fr.md)                                                                  |
| **MaixCam**          | ⭐ Facile          | Canal d'intégration matérielle pour caméras AI Sipeed | [Documentation](../channels/maixcam/README.fr.md)                                                               |
| **Pico**             | ⭐ Facile          | Canal protocole natif NeuroClaw                        |                                                                                                                  |

<a id="telegram"></a>
<details>
<summary><b>Telegram</b> (Recommandé)</summary>

**1. Créer un bot**

* Ouvrez Telegram, recherchez `@BotFather`
* Envoyez `/newbot`, suivez les instructions
* Copiez le token

**2. Configurer**

```json
{
  "channel_list": {
    "telegram": {
      "enabled": true,
      "type": "telegram",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"]
    }
  }
}
```

> Obtenez votre identifiant utilisateur via `@userinfobot` sur Telegram.

**3. Lancer**

```bash
neuroclaw gateway
```

**4. Menu de commandes Telegram (enregistré automatiquement au démarrage)**

NeuroClaw conserve les définitions de commandes dans un registre partagé unique. Au démarrage, Telegram enregistre automatiquement les commandes bot prises en charge (par exemple `/start`, `/help`, `/show`, `/list`, `/use`, `/btw`) afin que le menu de commandes et le comportement à l'exécution restent synchronisés.
L'enregistrement du menu de commandes Telegram reste une découverte UX locale au canal ; l'exécution générique des commandes est gérée de manière centralisée dans la boucle agent via l'exécuteur de commandes.

Si l'enregistrement des commandes échoue (erreurs transitoires réseau/API), le canal démarre quand même et NeuroClaw réessaie l'enregistrement en arrière-plan.

Vous pouvez aussi gerer les competences installees directement depuis Telegram :

- `/list skills`
- `/use <skill> <message>`
- `/use <skill>` puis envoyer la vraie requete dans le message suivant
- `/use clear`
- `/btw <question>` pour poser une question annexe immediate sans modifier l'historique actif de la session ; `/btw` est traite comme une requete directe sans outils et n'entre pas dans le flux normal d'execution des outils

</details>

<a id="discord"></a>
<details>
<summary><b>Discord</b></summary>

**1. Créer un bot**

* Allez sur <https://discord.com/developers/applications>
* Créez une application → Bot → Add Bot
* Copiez le token du bot

**2. Activer les intents**

* Dans les paramètres du Bot, activez **MESSAGE CONTENT INTENT**
* (Optionnel) Activez **SERVER MEMBERS INTENT** si vous prévoyez d'utiliser des listes d'autorisation basées sur les données des membres

**3. Obtenir votre identifiant utilisateur**
* Paramètres Discord → Avancé → activez **Developer Mode**
* Clic droit sur votre avatar → **Copy User ID**

**4. Configurer**

```json
{
  "channel_list": {
    "discord": {
      "enabled": true,
      "type": "discord",
      "token": "YOUR_BOT_TOKEN",
      "allow_from": ["YOUR_USER_ID"]
    }
  }
}
```

**5. Inviter le bot**

* OAuth2 → URL Generator
* Scopes : `bot`
* Bot Permissions : `Send Messages`, `Read Message History`
* Ouvrez l'URL d'invitation générée et ajoutez le bot à votre serveur

**Mode déclenchement en groupe (optionnel)**

Par défaut, le bot répond à tous les messages dans un canal de serveur. Pour limiter les réponses aux @mentions uniquement, ajoutez :

```json
{
  "channel_list": {
    "discord": {
      "group_trigger": { "mention_only": true }
    }
  }
}
```

Vous pouvez également déclencher par préfixes de mots-clés (par ex. `!bot`) :

```json
{
  "channel_list": {
    "discord": {
      "group_trigger": { "prefixes": ["!bot"] }
    }
  }
}
```

**6. Lancer**

```bash
neuroclaw gateway
```

</details>

<a id="whatsapp"></a>
<details>
<summary><b>WhatsApp</b> (natif via whatsmeow)</summary>

NeuroClaw peut se connecter à WhatsApp de deux manières :

- **Natif (recommandé) :** En processus via [whatsmeow](https://github.com/tulir/whatsmeow). Pas de bridge séparé. Définissez `"use_native": true` et laissez `bridge_url` vide. Au premier lancement, scannez le code QR avec WhatsApp (Appareils liés). La session est stockée dans votre workspace (par ex. `workspace/whatsapp/`). Le canal natif est **optionnel** pour garder le binaire par défaut léger ; compilez avec `-tags whatsapp_native` (par ex. `make build-whatsapp-native` ou `go build -tags whatsapp_native ./cmd/...`).
- **Bridge :** Connectez-vous à un bridge WebSocket externe. Définissez `bridge_url` (par ex. `ws://localhost:3001`) et gardez `use_native` à false.

**Configurer (natif)**

```json
{
  "channel_list": {
    "whatsapp": {
      "enabled": true,
      "type": "whatsapp",
      "use_native": true,
      "session_store_path": "",
      "allow_from": []
    }
  }
}
```

Si `session_store_path` est vide, la session est stockée dans `<workspace>/whatsapp/`. Lancez `neuroclaw gateway` ; au premier lancement, scannez le code QR affiché dans le terminal avec WhatsApp → Appareils liés.

</details>

<a id="weixin"></a>
<details>
<summary><b>Weixin</b> (WeChat Personnel)</summary>

NeuroClaw prend en charge la connexion à votre compte WeChat personnel via l'API officielle Tencent iLink.

**1. Connexion**

Lancez le flux de connexion interactif par QR code :
```bash
neuroclaw auth weixin
```
Scannez le QR code affiché avec votre application WeChat mobile. Une fois connecté, le token est sauvegardé dans votre configuration.

**2. Configurer**

(Optionnel) Ajoutez votre identifiant utilisateur WeChat dans `allow_from` pour restreindre qui peut envoyer des messages au bot :
```json
{
  "channel_list": {
    "weixin": {
      "enabled": true,
      "type": "weixin",
      "token": "YOUR_TOKEN",
      "allow_from": ["YOUR_USER_ID"]
    }
  }
}
```

**3. Lancer**
```bash
neuroclaw gateway
```

</details>

<a id="qq"></a>
<details>
<summary><b>QQ</b></summary>

**Configuration rapide (recommandée)**

QQ Open Platform propose une page de configuration en un clic pour les bots compatibles OpenClaw :

1. Ouvrez [QQ Bot Quick Start](https://q.qq.com/qqbot/openclaw/index.html) et scannez le QR code pour vous connecter
2. Un bot est créé automatiquement — copiez l'**App ID** et l'**App Secret**
3. Configurez NeuroClaw :

```json
{
  "channel_list": {
    "qq": {
      "enabled": true,
      "type": "qq",
      "app_id": "YOUR_APP_ID",
      "app_secret": "YOUR_APP_SECRET",
      "allow_from": []
    }
  }
}
```

4. Lancez `neuroclaw gateway` et ouvrez QQ pour discuter avec votre bot

> L'App Secret n'est affiché qu'une seule fois. Enregistrez-le immédiatement — le consulter à nouveau forcera une réinitialisation.
>
> Les bots créés via la page de configuration rapide sont initialement réservés au créateur et ne prennent pas en charge les discussions de groupe. Pour activer l'accès en groupe, configurez le mode sandbox sur la [QQ Open Platform](https://q.qq.com/).

**Configuration manuelle**

Si vous préférez créer le bot manuellement :

* Connectez-vous sur [QQ Open Platform](https://q.qq.com/) pour vous inscrire en tant que développeur
* Créez un bot QQ — personnalisez son avatar et son nom
* Copiez l'**App ID** et l'**App Secret** depuis les paramètres du bot
* Configurez comme indiqué ci-dessus et lancez `neuroclaw gateway`

</details>

<a id="dingtalk"></a>
<details>
<summary><b>DingTalk</b></summary>

**1. Créer un bot**

* Allez sur [Open Platform](https://open.dingtalk.com/)
* Créez une application interne
* Copiez le Client ID et le Client Secret

**2. Configurer**

```json
{
  "channel_list": {
    "dingtalk": {
      "enabled": true,
      "type": "dingtalk",
      "client_id": "YOUR_CLIENT_ID",
      "client_secret": "YOUR_CLIENT_SECRET",
      "allow_from": []
    }
  }
}
```

> Indiquez des identifiants utilisateur DingTalk dans `allow_from` pour restreindre l'accès. Vide autorise tous les utilisateurs ; utilisez `["*"]` pour le rendre explicite.

**3. Lancer**

```bash
neuroclaw gateway
```
</details>

<a id="matrix"></a>
<details>
<summary><b>Matrix</b></summary>

**1. Préparer le compte bot**

* Utilisez votre homeserver préféré (par ex. `https://matrix.org` ou auto-hébergé)
* Créez un utilisateur bot et obtenez son access token

**2. Configurer**

```json
{
  "channel_list": {
    "matrix": {
      "enabled": true,
      "type": "matrix",
      "homeserver": "https://matrix.org",
      "user_id": "@your-bot:matrix.org",
      "access_token": "YOUR_MATRIX_ACCESS_TOKEN",
      "allow_from": []
    }
  }
}
```

**3. Lancer**

```bash
neuroclaw gateway
```

Pour toutes les options (`device_id`, `join_on_invite`, `group_trigger`, `placeholder`, `reasoning_channel_id`), voir le [Guide de Configuration du Canal Matrix](../channels/matrix/README.md).

</details>

<a id="line"></a>
<details>
<summary><b>LINE</b></summary>

**1. Créer un compte officiel LINE**

- Allez sur [LINE Developers Console](https://developers.line.biz/)
- Créez un provider → Créez un canal Messaging API
- Copiez le **Channel Secret** et le **Channel Access Token**

**2. Configurer**

```json
{
  "channel_list": {
    "line": {
      "enabled": true,
      "type": "line",
      "channel_secret": "YOUR_CHANNEL_SECRET",
      "channel_access_token": "YOUR_CHANNEL_ACCESS_TOKEN",
      "webhook_path": "/webhook/line",
      "allow_from": []
    }
  }
}
```

> Le webhook LINE est servi sur le serveur Gateway partagé (`gateway.host`:`gateway.port`, par défaut `127.0.0.1:18790`).

**3. Configurer l'URL du Webhook**

LINE nécessite HTTPS pour les webhooks. Utilisez un reverse proxy ou un tunnel :

```bash
# Exemple avec ngrok (le port par défaut du gateway est 18790)
ngrok http 18790
```

Puis définissez l'URL du Webhook dans la console LINE Developers à `https://your-domain/webhook/line` et activez **Use webhook**.

**4. Lancer**

```bash
neuroclaw gateway
```

> Dans les discussions de groupe, le bot ne répond que lorsqu'il est @mentionné. Les réponses citent le message original.

</details>

<a id="wecom"></a>
<details>
<summary><b>WeCom (企业微信)</b></summary>

NeuroClaw expose WeCom comme un seul canal AI Bot via WebSocket.
Aucune URL de callback webhook publique n'est requise.

Voir le [Guide de Configuration WeCom](../channels/wecom/README.fr.md) pour la référence complète de configuration et les notes de migration.

**Configuration rapide - Recommandée**

**1. S'authentifier**

```bash
neuroclaw auth wecom
```

Cette commande affiche un QR code, attend l'approbation dans WeCom, puis écrit `bot_id` + `secret` dans l'entrée `wecom` de `channel_list`.

**2. Configurer manuellement si nécessaire**

```json
{
  "channel_list": {
    "wecom": {
      "enabled": true,
      "type": "wecom",
      "bot_id": "YOUR_BOT_ID",
      "secret": "YOUR_SECRET",
      "websocket_url": "wss://openws.work.weixin.qq.com",
      "send_thinking_message": true,
      "allow_from": [],
      "reasoning_channel_id": ""
    }
  }
}
```

**3. Lancer**

```bash
neuroclaw gateway
```

> Les anciennes entrées `wecom_app` et `wecom_aibot` sont remplacées par l'entrée unifiée `wecom` sous `channel_list`.

</details>

<a id="feishu"></a>
<details>
<summary><b>Feishu (飞书)</b></summary>

NeuroClaw se connecte à Feishu via le mode WebSocket/SDK — aucune URL webhook publique ni serveur de callback nécessaire.

**1. Créer une application**

* Allez sur [Feishu Open Platform](https://open.feishu.cn/) et créez une application
* Dans les paramètres de l'application, activez la capacité **Bot**
* Créez une version et publiez l'application (l'application doit être publiée pour prendre effet)
* Copiez l'**App ID** (commence par `cli_`) et l'**App Secret**

**2. Configurer**

```json
{
  "channel_list": {
    "feishu": {
      "enabled": true,
      "type": "feishu",
      "app_id": "cli_xxx",
      "app_secret": "YOUR_APP_SECRET",
      "allow_from": []
    }
  }
}
```

Optionnel : `encrypt_key` et `verification_token` pour le chiffrement des événements (recommandé en production).

**3. Lancer et discuter**

```bash
neuroclaw gateway
```

Ouvrez Feishu, recherchez le nom de votre bot et commencez à discuter. Vous pouvez aussi ajouter le bot à un groupe — utilisez `group_trigger.mention_only: true` pour ne répondre que lorsqu'il est @mentionné.

Pour toutes les options, voir le [Guide de Configuration du Canal Feishu](../channels/feishu/README.fr.md).

</details>

<a id="slack"></a>
<details>
<summary><b>Slack</b></summary>

**1. Créer une application Slack**

* Allez sur [Slack API](https://api.slack.com/apps) et créez une nouvelle application
* Sous **OAuth & Permissions**, ajoutez les scopes bot : `chat:write`, `app_mentions:read`, `im:history`, `im:read`, `im:write`
* Installez l'application dans votre workspace
* Copiez le **Bot Token** (`xoxb-...`) et l'**App-Level Token** (`xapp-...`, activez Socket Mode pour l'obtenir)

**2. Configurer**

```json
{
  "channel_list": {
    "slack": {
      "enabled": true,
      "type": "slack",
      "bot_token": "xoxb-YOUR-BOT-TOKEN",
      "app_token": "xapp-YOUR-APP-TOKEN",
      "allow_from": []
    }
  }
}
```

**3. Lancer**

```bash
neuroclaw gateway
```

</details>

<a id="irc"></a>
<details>
<summary><b>IRC</b></summary>

**1. Configurer**

```json
{
  "channel_list": {
    "irc": {
      "enabled": true,
      "type": "irc",
      "server": "irc.libera.chat:6697",
      "tls": true,
      "nick": "neuroclaw-bot",
      "channels": ["#your-channel"],
      "password": "",
      "allow_from": []
    }
  }
}
```

Optionnel : `nickserv_password` pour l'authentification NickServ, `sasl_user`/`sasl_password` pour l'authentification SASL.

**2. Lancer**

```bash
neuroclaw gateway
```

Le bot se connectera au serveur IRC et rejoindra les canaux spécifiés.

</details>

<a id="onebot"></a>
<details>
<summary><b>OneBot (QQ via protocole OneBot)</b></summary>

OneBot est un protocole ouvert pour les bots QQ. NeuroClaw se connecte à toute implémentation compatible OneBot v11 (par ex. [Lagrange](https://github.com/LagrangeDev/Lagrange.Core), [NapCat](https://github.com/NapNeko/NapCatQQ)) via WebSocket.

**1. Configurer une implémentation OneBot**

Installez et exécutez un framework de bot QQ compatible OneBot v11. Activez son serveur WebSocket.

**2. Configurer**

```json
{
  "channel_list": {
    "onebot": {
      "enabled": true,
      "type": "onebot",
      "ws_url": "ws://127.0.0.1:8080",
      "access_token": "",
      "allow_from": []
    }
  }
}
```

| Champ | Description |
|-------|-------------|
| `ws_url` | URL WebSocket de l'implémentation OneBot |
| `access_token` | Token d'accès pour l'authentification (si configuré dans OneBot) |
| `reconnect_interval` | Intervalle de reconnexion en secondes (par défaut : 5) |

**3. Lancer**

```bash
neuroclaw gateway
```

</details>

<a id="maixcam"></a>
<details>
<summary><b>MaixCam</b></summary>

**1. Préparer le matériel**

* Obtenez un appareil [Sipeed MaixCam](https://wiki.sipeed.com/maixcam)

**2. Configurer**

```json
{
  "channel_list": {
    "maixcam": {
      "enabled": true,
      "type": "maixcam",
      "allow_from": []
    }
  }
}
```

> MaixCam est une intégration matérielle Sipeed pour l'interaction IA embarquée.

**3. Lancer**

```bash
neuroclaw gateway
```

</details>

<a id="mqtt"></a>
<details>
<summary><b>MQTT</b></summary>

N'importe quel client MQTT peut communiquer avec NeuroClaw via un broker. Les appareils ou services publient des requêtes vers le broker ; NeuroClaw s'abonne, les traite et publie les réponses en retour.

**1. Configurer**

```json
{
  "channel_list": {
    "mqtt": {
      "enabled": true,
      "type": "mqtt",
      "settings": {
        "broker": "ssl://votre-broker:8883",
        "agent_id": "assistant",
        "topic_prefix": "/neuroclaw",
        "keep_alive": 60,
        "qos": 0
      }
    }
  }
}
```

Nom d'utilisateur et mot de passe dans `~/.neuroclaw/.security.yml` :

```yaml
channel_list:
  mqtt:
    settings:
      username: votre_utilisateur
      password: votre_mot_de_passe
```

**Format des topics**

```
{prefix}/{agent_id}/{client_id}/request    # Client → NeuroClaw
{prefix}/{agent_id}/{client_id}/response   # NeuroClaw → Client
```

Le `client_id` est défini par votre application cliente pour identifier les appareils ou sessions.

**2. Lancer**

```bash
neuroclaw gateway
```

**3. Tester**

```bash
mosquitto_pub -t "/neuroclaw/assistant/device1/request" \
  -m '{"text": "Bonjour"}'

mosquitto_sub -t "/neuroclaw/assistant/device1/response"
```

Pour les options complètes, voir [Documentation du canal MQTT](../channels/mqtt/README.fr.md).

</details>
