# Instalar o NeuroClaw

> Voltar aos [Guias](README.md)

O NeuroClaw é distribuído como um único binário estático, `neuroclaw`, e um binário do console web, `neuroclaw-launcher`. As releases estão no [GitHub](https://github.com/letrongminh/neuroclaw/releases). A configuração e o workspace ficam em `~/.neuroclaw` (defina `NEUROCLAW_HOME` para mudá-los de lugar).

## Escolha um arquivo

| Onde roda | Download |
|:---|:---|
| Servidor Linux ou VM na nuvem | `neuroclaw_Linux_x86_64.tar.gz`, `neuroclaw_Linux_arm64.tar.gz`, ou um `.deb` / `.rpm` |
| Raspberry Pi e outras placas | `Linux_arm64` (SO de 64 bits), `Linux_armv7`, `Linux_armv6` (Pi Zero/1), `Linux_riscv64`, `Linux_loong64`, `Linux_mipsle` (roteadores soft-float), `Linux_s390x` |
| Docker | `ghcr.io/letrongminh/neuroclaw` (linux/amd64, arm64, riscv64): [Guia Docker](docker.pt-br.md) |
| macOS | `neuroclaw_Darwin_arm64.tar.gz`, `neuroclaw_Darwin_x86_64.tar.gz` |
| Windows | `neuroclaw_Windows_x86_64.zip`, `neuroclaw_Windows_arm64.zip` |
| FreeBSD, NetBSD | Arquivos `Freebsd_*`, `Netbsd_*` |
| Android | Termux com `Linux_arm64`: [Guia Termux](android-termux.md) |
| Um ESP32-S3 | Não é um download. Veja [esp32/README.md](../../esp32/README.md) |

Os arquivos se chamam `neuroclaw_<OS>_<arch>.tar.gz` (`.zip` no Windows). Todos os arquivos estão listados em `neuroclaw_<version>_checksums.txt` na página da release.

## Verificar um download

```bash
sha256sum --check --ignore-missing neuroclaw_*_checksums.txt     # Linux
shasum -a 256 --check --ignore-missing neuroclaw_*_checksums.txt # macOS
```

No Windows, compare `Get-FileHash <file>` com a linha em `neuroclaw_<version>_checksums.txt`.

## Primeira execução

```bash
neuroclaw onboard          # writes ~/.neuroclaw/config.json and the workspace
vim ~/.neuroclaw/config.json   # add a provider key to model_list
neuroclaw agent -m "Hello" # one message; without -m it is interactive
neuroclaw gateway          # long-running: chat apps, cron, heartbeat
```

Veja [Configuração](configuration.pt-br.md) e [Provedores](providers.pt-br.md) para a configuração. `neuroclaw status` mostra o que está configurado. Na inicialização, o NeuroClaw registra `NeuroClaw risk=<name>` para cada capacidade arriscada que esteja ligada; veja [capabilities](../reference/capabilities.md).

## Linux

```bash
tar xzf neuroclaw_Linux_x86_64.tar.gz
sudo install -m 0755 neuroclaw neuroclaw-launcher /usr/local/bin/
```

### deb / rpm

Os pacotes se chamam `neuroclaw_x86_64.deb`, `neuroclaw_aarch64.rpm` e assim por diante. Eles instalam os dois binários em `/usr/bin`, mais uma entrada de desktop para o launcher.

```bash
sudo dpkg -i neuroclaw_x86_64.deb      # Debian, Ubuntu
sudo rpm -i neuroclaw_x86_64.rpm       # Fedora, RHEL
```

### systemd

Rode o gateway com um usuário próprio. Antes, rode `neuroclaw onboard` uma vez como esse usuário.

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

O gateway escuta em `localhost:18790` por padrão. `neuroclaw gateway --host 0.0.0.0` (ou `NEUROCLAW_GATEWAY_HOST`) muda isso para uma execução.

## Console web (launcher)

```bash
neuroclaw-launcher                 # http://localhost:18800, opens a browser
neuroclaw-launcher -console -no-browser
neuroclaw-launcher -public         # listen on all interfaces
```

Flags: `-port` (padrão 18800), `-host`, `-public`, `-no-browser`, `-console`, `-lang`, `-d`. O launcher inicia e gerencia o gateway. Não o exponha a redes não confiáveis; veja [Configuração](configuration.pt-br.md#painel-web-do-launcher).

## Docker

```bash
docker compose -f docker/docker-compose.yml --profile gateway up       # first run: writes config, exits
docker compose -f docker/docker-compose.yml --profile gateway up -d
docker compose -f docker/docker-compose.yml --profile launcher up -d   # web console on :18800
docker compose -f docker/docker-compose.yml run --rm neuroclaw-agent -m "Hello"
```

Serviços: `neuroclaw-gateway` (profile `gateway`), `neuroclaw-launcher` (profile `launcher`), `neuroclaw-agent` (profile `agent`). Os dados ficam em `docker/data`. Detalhes: [Guia Docker](docker.pt-br.md).

## macOS

Os arquivos não são assinados nem notarizados. Depois de extrair, remova a flag de quarentena:

```bash
tar xzf neuroclaw_Darwin_arm64.tar.gz
xattr -d com.apple.quarantine neuroclaw neuroclaw-launcher
./neuroclaw onboard
```

## Windows

Descompacte `neuroclaw_Windows_x86_64.zip`. Ele contém `neuroclaw.exe` e `neuroclaw-launcher.exe`. Os arquivos não têm assinatura de código, então o SmartScreen pode avisar: escolha "Mais informações" e depois "Executar assim mesmo", somente depois de conferir o checksum. A ferramenta shell executa comandos pelo PowerShell.

```powershell
.\neuroclaw.exe onboard
.\neuroclaw.exe gateway
```

## Android (Termux)

O Termux em um celular ARM64 roda o arquivo `Linux_arm64` por meio do `termux-chroot`. Passos completos: [Guia Termux](android-termux.md).

## Atualizar

```bash
neuroclaw update
```

O `neuroclaw update` baixa a release mais recente deste repositório e confere o arquivo com o `neuroclaw_<version>_checksums.txt`. Flags: `--url` (uma página de release ou um asset), `--platform`, `--arch`. Pacotes instalados com `dpkg` ou `rpm` também podem ser atualizados instalando o pacote mais novo. Docker: `docker compose pull`.

## Como cérebro de uma casa NeuroEdge

Veja [NeuroClaw com NeuroEdge](neuroedge.pt-br.md).
