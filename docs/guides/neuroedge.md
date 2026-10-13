# NeuroClaw with NeuroEdge

> Back to [Guides](README.md)

A [NeuroEdge](https://github.com/letrongminh/neuroedge-init) home box runs the home's devices behind a gate. NeuroClaw is the brain: it reads your chat, thinks, and calls the box's tools over MCP. The box decides each call with the home's rules and, for anything that cannot be undone, asks the owner at home.

## Connect to the box

The box serves MCP over HTTPS with `neuroedge mcp serve --http`. It requires a client certificate that it signed. NeuroClaw presents one with the `tls` block of an `http` MCP server, so no bridge is needed.

In `~/.neuroclaw/config.json`:

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

| `tls` field | Meaning |
|:---|:---|
| `ca_file` | PEM CA that signed the box's certificate. Only this CA is trusted. |
| `cert_file`, `key_file` | The brain's client certificate and key. Set both or neither. |
| `server_name` | Name to check in the box's certificate, if it differs from the host in `url`. |

Rules from `pkg/mcp/tls.go`: `url` must be `https` when `tls` is set; the minimum is TLS 1.3; paths accept `~/`. Check it with:

```bash
neuroclaw mcp test neuroedge
```

## What the model is told

A box tool returns a result with a status. NeuroClaw turns a result that is not a plain `ALLOW` into fixed instructions for the model (`pkg/tools/integration/neuroedge_result.go`), so the answer is the same on any model.

| Status | The model is told |
|:---|:---|
| `BLOCK` with a confirmation | The box did not do it yet. The owner must approve on the NeuroEdge page at home within about the stated seconds. The model cannot approve, and approval in chat does not count. Tell the user it is waiting; do not call the tool again; do not say it is done. |
| `BLOCK` without one | The box refused, with the reason and the failed criterion. Tell the user it was not done and why; do not retry with other arguments or look for a way around the rule. |
| `REJECTED` | The box rejected the call itself (unknown tool or arguments outside its schema). Nothing ran. |

Results from other MCP servers are not touched.

## Risk labels

At startup NeuroClaw logs `NeuroClaw risk=<name>` for each capability that lets the model reach beyond chat, its workspace and the box's tools:

| Label | On when |
|:---|:---|
| `shell` | `tools.exec` (default on) |
| `cron_shell` | `tools.cron.allow_command` with `exec` (default on) |
| `skill_install` | `tools.install_skill` or `find_skills`, with `tools.skills` on (default on) |
| `hardware_bus` | `tools.i2c`, `spi` or `serial` (default off) |
| `process_hooks` | an enabled entry in `hooks.processes` (default off) |
| `usb_monitor` | `devices.monitor_usb` with `devices` (default off) |
| `stdio_mcp` | a stdio server in `tools.mcp.servers` (default off) |

Labels inform; they block nothing. Meaning of each: [capabilities](../reference/capabilities.md).

## Same machine as the box: close the brain

The box's gate only covers calls that go through it. A shell or a hardware bus on the machine that also runs the box can reach devices around the gate. If the brain shares a machine with the box, turn those off:

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

Merge it with your config; keep the `mcp` block from above. Start NeuroClaw again and check that no `risk=` line for these appears in the log.

Also run the brain as its own OS user (see the systemd unit in the [install guide](install.md#systemd)) that has no access to the box's keys other than the brain's own client certificate, and none to device nodes.

## On another machine or NeuroEdge Cloud

When the brain runs on a different machine, or on NeuroEdge Cloud, the gate still decides every device action. The shell and hardware buses reach only the machine the brain runs on, so leave them as you need.

## NeuroClaw-ESP

The same brain on one ESP32-S3, with the gate on the same chip: [esp32/README.md](../../esp32/README.md). It is labelled `risk=single_chip` because brain and gate share one address space.
