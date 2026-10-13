# Installer NeuroClaw

> Retour aux [Guides](README.md)

NeuroClaw est distribué sous la forme d'un binaire statique, `neuroclaw`, et d'un binaire pour la console web, `neuroclaw-launcher`. Les versions sont sur [GitHub](https://github.com/letrongminh/neuroclaw/releases). La config et l'espace de travail sont dans `~/.neuroclaw` (définissez `NEUROCLAW_HOME` pour le déplacer).

## Choisir un fichier

| Où il tourne | Téléchargement |
|:---|:---|
| Serveur Linux ou VM cloud | `neuroclaw_Linux_x86_64.tar.gz`, `neuroclaw_Linux_arm64.tar.gz`, ou un `.deb` / `.rpm` |
| Raspberry Pi et autres cartes | `Linux_arm64` (OS 64 bits), `Linux_armv7`, `Linux_armv6` (Pi Zero/1), `Linux_riscv64`, `Linux_loong64`, `Linux_mipsle` (routeurs sans FPU), `Linux_s390x` |
| Docker | `ghcr.io/letrongminh/neuroclaw` (linux/amd64, arm64, riscv64) : [guide Docker](docker.fr.md) |
| macOS | `neuroclaw_Darwin_arm64.tar.gz`, `neuroclaw_Darwin_x86_64.tar.gz` |
| Windows | `neuroclaw_Windows_x86_64.zip`, `neuroclaw_Windows_arm64.zip` |
| FreeBSD, NetBSD | archives `Freebsd_*`, `Netbsd_*` |
| Android | Termux avec `Linux_arm64` : [guide Termux](android-termux.md) |
| Un seul ESP32-S3 | Pas un téléchargement. Voir [esp32/README.md](../../esp32/README.md) |

Les archives s'appellent `neuroclaw_<OS>_<arch>.tar.gz` (`.zip` sous Windows). Chaque fichier figure dans `neuroclaw_<version>_checksums.txt` sur la page de la version.

## Vérifier un téléchargement

```bash
sha256sum --check --ignore-missing neuroclaw_*_checksums.txt     # Linux
shasum -a 256 --check --ignore-missing neuroclaw_*_checksums.txt # macOS
```

Sous Windows, comparez `Get-FileHash <fichier>` avec la ligne correspondante de `neuroclaw_<version>_checksums.txt`.

## Premier lancement

```bash
neuroclaw onboard          # writes ~/.neuroclaw/config.json and the workspace
vim ~/.neuroclaw/config.json   # add a provider key to model_list
neuroclaw agent -m "Hello" # one message; without -m it is interactive
neuroclaw gateway          # long-running: chat apps, cron, heartbeat
```

Voir [Configuration](configuration.fr.md) et [Fournisseurs](providers.fr.md) pour la config. `neuroclaw status` montre ce qui est configuré. Au démarrage, NeuroClaw journalise `NeuroClaw risk=<nom>` pour chaque capacité à risque activée ; voir [capabilities](../reference/capabilities.md).

## Linux

```bash
tar xzf neuroclaw_Linux_x86_64.tar.gz
sudo install -m 0755 neuroclaw neuroclaw-launcher /usr/local/bin/
```

### deb / rpm

Les paquets s'appellent `neuroclaw_x86_64.deb`, `neuroclaw_aarch64.rpm`, etc. Ils installent les deux binaires dans `/usr/bin`, plus une entrée de bureau pour le lanceur.

```bash
sudo dpkg -i neuroclaw_x86_64.deb      # Debian, Ubuntu
sudo rpm -i neuroclaw_x86_64.rpm       # Fedora, RHEL
```

### systemd

Exécutez la passerelle (gateway) sous son propre utilisateur. Lancez d'abord `neuroclaw onboard` une fois avec cet utilisateur.

```ini
# /etc/systemd/system/neuroclaw.service
[Unit]
Description=NeuroClaw gateway
After=network-online.target
Wants=network-online.target

[Service]
User=neuroclaw
ExecStart=/usr/local/bin/neuroclaw gateway
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

```bash
sudo useradd --system --create-home neuroclaw
sudo -u neuroclaw neuroclaw onboard
sudo systemctl enable --now neuroclaw
journalctl -u neuroclaw -f
```

La passerelle écoute par défaut sur `localhost:18790`. `neuroclaw gateway --host 0.0.0.0` (ou `NEUROCLAW_GATEWAY_HOST`) change cela pour une exécution.

## Console web (launcher)

```bash
neuroclaw-launcher                 # http://localhost:18800, opens a browser
neuroclaw-launcher -console -no-browser
neuroclaw-launcher -public         # listen on all interfaces
```

Options : `-port` (18800 par défaut), `-host`, `-public`, `-no-browser`, `-console`, `-lang`, `-d`. Le lanceur démarre et gère la passerelle. Ne l'exposez pas à des réseaux non fiables ; voir [Configuration](configuration.fr.md#tableau-de-bord-du-lanceur-web).

## Docker

```bash
docker compose -f docker/docker-compose.yml --profile gateway up       # first run: writes config, exits
docker compose -f docker/docker-compose.yml --profile gateway up -d
docker compose -f docker/docker-compose.yml --profile launcher up -d   # web console on :18800
docker compose -f docker/docker-compose.yml run --rm neuroclaw-agent -m "Hello"
```

Services : `neuroclaw-gateway` (profil `gateway`), `neuroclaw-launcher` (profil `launcher`), `neuroclaw-agent` (profil `agent`). Les données sont dans `docker/data`. Détails : [guide Docker](docker.fr.md).

## macOS

Les archives ne sont ni signées ni notarisées. Après extraction, retirez le drapeau de quarantaine :

```bash
tar xzf neuroclaw_Darwin_arm64.tar.gz
xattr -d com.apple.quarantine neuroclaw neuroclaw-launcher
./neuroclaw onboard
```

## Windows

Décompressez `neuroclaw_Windows_x86_64.zip`. Il contient `neuroclaw.exe` et `neuroclaw-launcher.exe`. Les fichiers ne sont pas signés, donc SmartScreen peut afficher un avertissement : choisissez « Informations complémentaires », puis « Exécuter quand même », uniquement après avoir vérifié la somme de contrôle. L'outil shell exécute les commandes via PowerShell.

```powershell
.\neuroclaw.exe onboard
.\neuroclaw.exe gateway
```

## Android (Termux)

Termux sur un téléphone ARM64 exécute l'archive `Linux_arm64` via `termux-chroot`. Étapes complètes : [guide Termux](android-termux.md).

## Mise à jour

```bash
neuroclaw update
```

`neuroclaw update` télécharge la dernière version de ce dépôt et vérifie l'archive avec `neuroclaw_<version>_checksums.txt`. Options : `--url` (une page de version ou un fichier), `--platform`, `--arch`. Les paquets installés avec `dpkg` ou `rpm` peuvent aussi être mis à jour en installant le paquet plus récent. Docker : `docker compose pull`.

## Comme cerveau d'une maison NeuroEdge

Voir [NeuroClaw avec NeuroEdge](neuroedge.fr.md).
