# 调试 NeuroClaw

> 返回 [README](../project/README.zh.md)

NeuroClaw 在处理每一个请求时，都会在后台执行多个复杂的交互操作——从消息路由和复杂度评估，到工具执行和模型故障适配。能够准确地看到正在发生什么至关重要，这不仅有助于排查潜在问题，也有助于真正理解代理的运作方式。

## 以调试模式启动 NeuroClaw

要获取代理运行的详细信息（LLM 请求、工具调用、消息路由），可以使用调试标志启动 NeuroClaw 网关：

```bash
neuroclaw gateway --debug
# or
neuroclaw gateway -d
```

在此模式下，系统会对日志进行详细格式化，并显示系统提示词和工具执行结果的预览。

## 启动时的风险标签

无论是否设置 `--debug`，`gateway` 和 `agent` 都会为每一项已启用的高风险能力记录一条警告 `NeuroClaw risk=<name>`（例如 `risk=shell`）。需要了解模型在这台机器上被允许做什么时，请先看这些日志。名称与含义见 [capabilities.md 第 2 节](../reference/capabilities.md#2-risk-labels)。

## 禁用日志截断（完整日志）

默认情况下，NeuroClaw 会在调试日志中截断过长的字符串（例如*系统提示词*或大型 JSON 输出结果），以保持控制台的可读性。

如果你需要检查某个命令的完整输出，或发送给 LLM 模型的确切载荷，可以使用 `--no-truncate` 标志。

**注意：** 此标志*仅*在与 `--debug` 模式组合使用时有效。

```bash
neuroclaw gateway --debug --no-truncate

```

当此标志激活时，全局截断功能将被禁用。这在以下场景中非常有用：

* 验证发送给提供商的消息的确切语法。
* 读取 `exec`、`web_fetch` 或 `read_file` 等工具的完整输出。
* 调试保存在内存中的会话历史。

## 调试日志中的工具调用可见性

调试模式启用后，代理会在工具执行生命周期的每个阶段输出结构化日志。这些日志带有 `component=agent` 标签，根据详细程度使用 `INFO` 或 `DEBUG` 级别：

| 日志消息 | 级别 | 关键字段 | 说明 |
|---|---|---|---|
| `LLM requested tool calls` | INFO | `tools`, `count`, `iteration` | 模型决定调用的工具名称列表 |
| `Tool call: <name>(<args>)` | INFO | `tool`, `iteration` | 工具名称及其参数预览（截断为 200 个字符） |
| `Sent tool result to user` | DEBUG | `tool`, `content_len` | 工具结果被转发到聊天渠道时记录 |
| `TTL tick after tool execution` | DEBUG | `agent_id`, `iteration` | 每轮工具执行后，MCP 工具发现 TTL 递减 |
| `Async tool completed, publishing result` | INFO | `tool`, `content_len`, `channel` | 仅用于在后台异步运行的工具 |

### 读取工具调用日志

一次典型的同步工具调用会在控制台产生连续两行：

```
[...] [INFO] agent: LLM requested tool calls {tools=[web_search], count=1, iteration=1}
[...] [INFO] agent: Tool call: web_search({"query":"neuroclaw release notes"}) {tool=web_search, iteration=1}
```

参数预览在日志中被硬性限制为 **200 个字符**，与 `--no-truncate` 标志无关，因为它走的是 `INFO` 级别的路径。若要查看 `Full LLM request` 这条 DEBUG 日志里完整的 `tools_json` 字段（包含发送给模型的每个工具定义），请同时使用 `--no-truncate` 和 `--debug`。

## 聊天中的实时工具反馈（tool_feedback）

调试日志只存在于服务端。如果希望代理每次执行工具时直接在聊天渠道里发送一条可见的通知（在与其他用户共用机器人，或需要透明度时很有用），请在 `config.json` 中启用 `tool_feedback`：

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

当 `enabled` 为 `true` 时，每次工具调用都会在工具结果返回给模型之前，向聊天发送一条简短消息，形如：

```bash
🔧 `web_search`
{"query": "neuroclaw release notes"}
```

### 选项

| 字段 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `enabled` | bool | `false` | 为每次工具调用发送聊天通知 |
| `separate_messages` | bool | `false` | 将每次工具反馈更新保留为单独的聊天消息，而不是复用同一条占位/进度消息 |
| `max_args_length` | int | `300` | 通知中包含的序列化参数的最大字符数 |

### 环境变量

这些字段也可以通过环境变量设置：

```bash
NEUROCLAW_AGENTS_DEFAULTS_TOOL_FEEDBACK_ENABLED=true
NEUROCLAW_AGENTS_DEFAULTS_TOOL_FEEDBACK_MAX_ARGS_LENGTH=300
```

> **注意：** `tool_feedback` 与 `--debug` 模式相互独立。它可以在生产环境中使用，不需要用任何特殊标志启动网关。
