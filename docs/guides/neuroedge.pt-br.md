# NeuroClaw com NeuroEdge

> Voltar aos [Guias](README.md)

Uma caixa doméstica [NeuroEdge](https://github.com/letrongminh/neuroedge-init) opera os dispositivos da casa atrás de um gate. O NeuroClaw é o cérebro: lê o seu chat, pensa e chama as ferramentas da caixa por MCP. A caixa decide cada chamada com as regras da casa e, para tudo o que não pode ser desfeito, pergunta ao dono em casa.

## Conectar à caixa

A caixa serve MCP sobre HTTPS com `neuroedge mcp serve --http`. Ela exige um certificado de cliente que ela mesma assinou. O NeuroClaw apresenta um com o bloco `tls` de um servidor MCP `http`, então nenhuma ponte é necessária.

Em `~/.neuroclaw/config.json`:

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

| Campo `tls` | Significado |
|:---|:---|
| `ca_file` | CA PEM que assinou o certificado da caixa. Só essa CA é confiável. |
| `cert_file`, `key_file` | O certificado de cliente e a chave do cérebro. Defina os dois ou nenhum. |
| `server_name` | Nome a verificar no certificado da caixa, se for diferente do host em `url`. |

Regras de `pkg/mcp/tls.go`: `url` deve ser `https` quando `tls` está definido; o mínimo é TLS 1.3; os caminhos aceitam `~/`. Verifique com:

```bash
neuroclaw mcp test neuroedge
```

## O que o modelo recebe

Uma ferramenta da caixa retorna um resultado com um status. O NeuroClaw transforma um resultado que não seja um `ALLOW` simples em instruções fixas para o modelo (`pkg/tools/integration/neuroedge_result.go`), de modo que a resposta é a mesma em qualquer modelo.

| Status | O que se diz ao modelo |
|:---|:---|
| `BLOCK` com uma confirmação | A caixa ainda não fez. O dono deve aprovar na página do NeuroEdge em casa dentro de aproximadamente os segundos indicados. O modelo não pode aprovar, e aprovar no chat não conta. Diga ao usuário que está aguardando; não chame a ferramenta de novo; não diga que foi feito. |
| `BLOCK` sem uma | A caixa recusou, com o motivo e o critério que falhou. Diga ao usuário que não foi feito e por quê; não tente de novo com outros argumentos nem procure um jeito de contornar a regra. |
| `REJECTED` | A caixa rejeitou a própria chamada (ferramenta desconhecida ou argumentos fora do seu esquema). Nada foi executado. |

Resultados de outros servidores MCP não são alterados.

## Rótulos de risco

Na inicialização, o NeuroClaw registra `NeuroClaw risk=<name>` para cada capacidade que permite ao modelo ir além do chat, do seu workspace e das ferramentas da caixa:

| Rótulo | Ligado quando |
|:---|:---|
| `shell` | `tools.exec` (padrão ligado) |
| `cron_shell` | `tools.cron.allow_command` com `exec` (padrão ligado) |
| `skill_install` | `tools.install_skill` ou `find_skills`, com `tools.skills` ligado (padrão ligado) |
| `hardware_bus` | `tools.i2c`, `spi` ou `serial` (padrão desligado) |
| `process_hooks` | uma entrada habilitada em `hooks.processes` (padrão desligado) |
| `usb_monitor` | `devices.monitor_usb` com `devices` (padrão desligado) |
| `stdio_mcp` | um servidor stdio em `tools.mcp.servers` (padrão desligado) |

Os rótulos informam; não bloqueiam nada. Significado de cada um: [capabilities](../reference/capabilities.md).

## Na mesma máquina da caixa: feche o cérebro

O gate da caixa só cobre as chamadas que passam por ele. Um shell ou um barramento de hardware na máquina que também roda a caixa pode alcançar dispositivos por fora do gate. Se o cérebro divide a máquina com a caixa, desligue esses recursos:

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

Mescle com a sua configuração; mantenha o bloco `mcp` acima. Inicie o NeuroClaw de novo e confira que nenhuma linha `risk=` para esses itens aparece no log.

Rode também o cérebro com um usuário de SO próprio (veja a unidade systemd no [guia de instalação](install.pt-br.md#systemd)) que não tenha acesso às chaves da caixa além do certificado de cliente do próprio cérebro, e nenhum acesso aos nós de dispositivo.

## Em outra máquina ou no NeuroEdge Cloud

Quando o cérebro roda em outra máquina, ou no NeuroEdge Cloud, o gate continua decidindo cada ação em dispositivos. O shell e os barramentos de hardware alcançam apenas a máquina onde o cérebro roda, então deixe-os como precisar.

## NeuroClaw-ESP

O mesmo cérebro em um único ESP32-S3, com o gate no mesmo chip: [esp32/README.md](../../esp32/README.md). Ele é rotulado `risk=single_chip` porque cérebro e gate compartilham o mesmo espaço de endereços.
