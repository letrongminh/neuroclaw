# NeuroClaw 🦞

[PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1 能做的一切（聊天应用、LLM 提供商、记忆、提醒、shell、技能、MCP、Web 控制台和启动器），再加上 [NeuroEdge](https://github.com/letrongminh/neuroedge-init) 之家的大脑。连接到 NeuroEdge 家用盒子后，它通过盒子的 MCP 工具访问家中的设备；这些工具按家里的规则做决定，对任何无法撤销的操作，会先在家里向你确认。凡是能让模型超出聊天范围的能力（shell、硬件总线、技能安装、hooks、stdio MCP 服务器），都按你的配置开启或关闭，每一项处于开启状态的能力都会在启动时被点名（`risk=…`）。

它可以运行在 Linux 开发板、服务器（NeuroEdge Cloud）、Mac 或 Windows PC 上；发布的平台与 PicoClaw 相同。安装：[安装指南](../guides/install.zh.md)。

也可以直接运行在芯片上：[`esp32/`](../../esp32/) 是 **NeuroClaw-ESP**，同一个大脑用 C 重写，作为 ESP-IDF 组件运行在一颗 ESP32-S3 上——Telegram、LLM（含备用模型）、记忆、提醒、心跳、网页搜索、向主人发送消息和文件，以及网关，全部放在一颗几美元的芯片上，工作区和工具与这里相同（[哪些相同，哪些不同](../../esp32/README.md)）。由于此时大脑和网关共用同一个地址空间，它被标为 `risk=single_chip`。

NeuroClaw 是 [PicoClaw](https://github.com/sipeed/picoclaw) v0.3.1（MIT）的硬分叉。见 [NOTICE](../../NOTICE)。

## 与 PicoClaw 相比有什么变化

| | PicoClaw | NeuroClaw |
|:---|:---|:---|
| HTTP 上的 MCP | 只有请求头 | 还支持双向 TLS：`tls.ca_file`、`cert_file`、`key_file`、`server_name`，TLS 1.3。NeuroEdge 盒子（`neuroedge mcp serve --http`）要求客户端证书，因此不需要桥接。 |
| shell、硬件、技能安装、hooks、stdio MCP | 由配置开启或关闭 | 相同，沿用 PicoClaw 的默认值；每一项处于开启状态的能力都会在启动时记录为 `NeuroClaw risk=<name>`（`shell`、`cron_shell`、`hardware_bus`、`skill_install`、`process_hooks`、`usb_monitor`、`stdio_mcp`；见 [`pkg/config/risk.go`](../../pkg/config/risk.go)）。希望大脑保持封闭的 NeuroEdge 之家，在 `config.json` 里写明即可。 |
| 一个说“问主人”的网关 | 普通的工具结果 | 当盒子工具返回带 `confirmation` 的 `BLOCK` 时，NeuroClaw 会明确告诉模型：什么都没有执行，只有家里的主人才能批准，且不要重试（[`pkg/tools/integration/neuroedge_result.go`](../../pkg/tools/integration/neuroedge_result.go)）。 |
| `update` | 下载 `sipeed/picoclaw` 的发布版本 | 下载本仓库的发布版本，并用其校验和检查 |
| 名称 | `picoclaw`、`~/.picoclaw`、`PICOCLAW_*` | `neuroclaw`、`~/.neuroclaw`、`NEUROCLAW_*` |

其余都和 PicoClaw 一样：聊天渠道（Telegram 等）、LLM 提供商、记忆和会话、cron 和心跳、网页搜索、消息、内置技能、子代理、Web 控制台和启动器（`neuroclaw-launcher`）、Docker 镜像（`ghcr.io/letrongminh/neuroclaw`）、每夜构建、deb/rpm 软件包和 macOS DMG。它的指南在 [`docs/`](../README.md)（[原始 README](PICOCLAW_README.md)）；阅读时请把其中的 `picoclaw` 当作 `neuroclaw`。

**[NeuroClaw 能做什么](../reference/capabilities.md)** 列出了两个大脑的全部能力、默认开启的内容、每个风险标签的含义，以及已知的问题。

## 连接到 NeuroEdge 盒子

指南：[NeuroClaw 与 NeuroEdge](../guides/neuroedge.zh.md)。

盒子运行 `neuroedge mcp serve --http --ui …`；它的密钥来自 NeuroEdge 的 `demo/i2f-chat-den-cua/tao-khoa.py`。在 `~/.neuroclaw/config.json` 中：

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

对演示代理来说，`neuroclaw mcp test neuroedge` 应当输出 `reachable (3 tools)`。

## 构建

```bash
CGO_ENABLED=0 go build -tags goolm,stdjson -o neuroclaw ./cmd/neuroclaw
CGO_ENABLED=0 go test -tags goolm,stdjson ./...
```

`make build-launcher` 构建 Web 控制台（`web/`，需要 pnpm）；`make build-all` 和 goreleaser（`.goreleaser.yaml`）构建所有平台。在 macOS 上有三个 shell 工具测试会失败，与 PicoClaw v0.3.1 中一样；CI 运行在 Linux 上，那里它们会通过。

**从加入风险标签之前的 NeuroClaw 构建升级**（2026-10-13）：那些构建会强制关闭 shell、cron 命令、技能安装和 USB 监控，并且 `onboard` 会把它们以关闭状态保存到 `config.json`。请在其中把它们打开，或用 `neuroclaw config reset` 从默认值重新开始。

## 许可证

MIT，与 PicoClaw 相同。版权所有 (c) 2026 PicoClaw contributors 和 NeuroClaw contributors。
