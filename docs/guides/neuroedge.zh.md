# NeuroClaw 与 NeuroEdge

> 返回 [指南](README.md)

[NeuroEdge](https://github.com/letrongminh/neuroedge-init) 家用盒子在一道网关之后运行家中的设备。NeuroClaw 是大脑：它读取你的聊天，进行思考，并通过 MCP 调用盒子的工具。盒子按家里的规则决定每一次调用，对任何无法撤销的操作，会在家里询问主人。

## 连接到盒子

盒子用 `neuroedge mcp serve --http` 通过 HTTPS 提供 MCP。它要求客户端出示由它签发的证书。NeuroClaw 通过 `http` 类型 MCP 服务器的 `tls` 块出示证书，因此不需要桥接。

在 `~/.neuroclaw/config.json` 中：

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

| `tls` 字段 | 含义 |
|:---|:---|
| `ca_file` | 签发盒子证书的 PEM 格式 CA。只信任这一个 CA。 |
| `cert_file`、`key_file` | 大脑的客户端证书和密钥。要么都设置，要么都不设置。 |
| `server_name` | 在盒子证书中校验的名称（若与 `url` 中的主机不同）。 |

来自 `pkg/mcp/tls.go` 的规则：设置了 `tls` 时，`url` 必须是 `https`；最低为 TLS 1.3；路径接受 `~/`。用下面的命令检查：

```bash
neuroclaw mcp test neuroedge
```

## 模型会被告知什么

盒子工具返回的结果带有一个状态。NeuroClaw 会把不是普通 `ALLOW` 的结果转换成固定的、给模型的指示（`pkg/tools/integration/neuroedge_result.go`），因此在任何模型上得到的回答都一样。

| 状态 | 模型被告知的内容 |
|:---|:---|
| 带确认的 `BLOCK` | 盒子还没有执行。主人必须在家里的 NeuroEdge 页面上，在大约所述的秒数内批准。模型不能批准，在聊天中批准也不算数。告诉用户正在等待；不要再次调用该工具；不要说已经完成。 |
| 不带确认的 `BLOCK` | 盒子拒绝了，并给出原因和未通过的标准。告诉用户没有执行以及原因；不要换参数重试，也不要寻找绕过规则的办法。 |
| `REJECTED` | 盒子拒绝了这次调用本身（未知的工具，或参数超出其模式）。什么都没有执行。 |

来自其他 MCP 服务器的结果不会被改动。

## 风险标签

启动时，NeuroClaw 会为每一项让模型超出聊天、其工作区和盒子工具范围的能力记录 `NeuroClaw risk=<name>`：

| 标签 | 开启条件 |
|:---|:---|
| `shell` | `tools.exec`（默认开启） |
| `cron_shell` | `tools.cron.allow_command` 且 `exec` 开启（默认开启） |
| `skill_install` | `tools.install_skill` 或 `find_skills`，且 `tools.skills` 开启（默认开启） |
| `hardware_bus` | `tools.i2c`、`spi` 或 `serial`（默认关闭） |
| `process_hooks` | `hooks.processes` 中已启用的条目（默认关闭） |
| `usb_monitor` | `devices.monitor_usb` 且 `devices` 开启（默认关闭） |
| `stdio_mcp` | `tools.mcp.servers` 中的 stdio 服务器（默认关闭） |

标签只起提示作用，不阻止任何操作。每个标签的含义：[能力说明](../reference/capabilities.md)。

## 与盒子在同一台机器上：封闭大脑

盒子的网关只覆盖经过它的调用。在同时运行盒子的机器上，shell 或硬件总线可以绕过网关访问设备。如果大脑与盒子共用一台机器，请把这些关掉：

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

把它合并到你的配置中；保留上面的 `mcp` 块。重新启动 NeuroClaw，并检查日志中没有这些能力对应的 `risk=` 行。

另外，请让大脑以独立的操作系统用户运行（见[安装指南](install.zh.md#systemd)中的 systemd 单元），该用户除大脑自己的客户端证书外，不能访问盒子的任何密钥，也不能访问设备节点。

## 在另一台机器或 NeuroEdge Cloud 上

当大脑运行在另一台机器上，或运行在 NeuroEdge Cloud 上时，网关仍然决定每一个设备操作。shell 和硬件总线只能访问大脑所在的那台机器，所以按需要保留即可。

## NeuroClaw-ESP

同一个大脑运行在一颗 ESP32-S3 上，网关在同一颗芯片上：[esp32/README.md](../../esp32/README.md)。由于大脑和网关共用同一个地址空间，它被标为 `risk=single_chip`。
