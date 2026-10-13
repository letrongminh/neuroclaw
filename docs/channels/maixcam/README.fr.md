> Retour au [README](../../project/README.fr.md)

# MaixCam

MaixCam est un canal dédié à la connexion aux caméras AI Sipeed MaixCAM et MaixCAM2. Il utilise des sockets TCP pour une communication bidirectionnelle et prend en charge les scénarios de déploiement d'IA en périphérie.

## Configuration

```json
{
  "channel_list": {
    "maixcam": {
      "enabled": true,
      "type": "maixcam",
      "host": "0.0.0.0",
      "port": 18790,
      "allow_from": []
    }
  }
}
```

| Champ      | Type   | Requis | Description                                                                 |
| ---------- | ------ | ------ | --------------------------------------------------------------------------- |
| enabled    | bool   | Oui    | Activer ou non le canal MaixCam                                             |
| host       | string | Oui    | Adresse d'écoute du serveur TCP                                             |
| port       | int    | Oui    | Port d'écoute du serveur TCP (par défaut `18790`, identique au port HTTP de la passerelle ; choisissez-en un autre si les deux tournent sur le même hôte) |
| allow_from | array  | Non    | Liste blanche d'identifiants d'appareils ; vide laisse entrer tout le monde (la passerelle consigne un avertissement) ; renseignez-le, ou utilisez `["*"]` pour autoriser tout le monde volontairement |

## Cas d'utilisation

Le canal MaixCam permet à NeuroClaw de fonctionner comme backend IA pour les appareils en périphérie :

- **Surveillance intelligente** : MaixCAM envoie des images ; NeuroClaw les analyse via des modèles de vision
- **Contrôle IoT** : Les appareils envoient des données de capteurs ; NeuroClaw coordonne les réponses
- **IA hors ligne** : Déployer NeuroClaw sur un réseau local pour une inférence à faible latence
