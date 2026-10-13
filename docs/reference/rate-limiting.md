# Dynamic Rate Limiting

NeuroClaw prevents 429 errors from LLM provider APIs by enforcing configurable per-model request-rate limits **before** sending each request. Unlike the reactive cooldown/fallback system (which activates *after* a 429 is received), rate limiting is **proactive**: it keeps outbound QPS within the provider's free-tier or plan limits.

## How it works

### Token-bucket algorithm

Each rate-limited model gets a token bucket:

- **Capacity** = `rpm` (burst size equals the per-minute limit)
- **Refill rate** = `rpm / 60` tokens per second
- Tokens are consumed one per LLM call; if the bucket is empty, the call blocks until a token refills or the request context is cancelled

### Call chain integration

```
AgentLoop.callLLM()
  └─ FallbackChain.Execute()         ← iterate candidates
       ├─ CooldownTracker.IsAvailable()   ← skip if post-429 cooldown active
       ├─ RateLimiterRegistry.Wait()      ← NEW: block until token available
       └─ provider.Chat()                 ← actual LLM HTTP call
```

The rate limiter runs **after** the cooldown check and **before** the provider call, so:
- Candidates already in cooldown are skipped entirely (no token consumed)
- Candidates that are available get throttled to the configured RPM

The same check applies in `ExecuteImage`.

### Thread safety

`RateLimiterRegistry` is safe for concurrent use. The per-limiter token bucket uses a fine-grained mutex so concurrent goroutines each acquire their own token independently.

## Configuration

Set `rpm` on any model in `model_list`:

```yaml
model_list:
  - model_name: gpt-4o-free
    provider: openai
    model: gpt-4o
    api_base: https://api.openai.com/v1
    rpm: 3          # max 3 requests per minute
    api_keys:
      - sk-...

  - model_name: claude-haiku
    provider: anthropic
    model: claude-haiku-4-5
    rpm: 60         # 60 rpm (Anthropic free tier)
    api_keys:
      - sk-ant-...

  - model_name: local-llm
    provider: ollama
    model: llama3
    api_base: http://localhost:11434/v1
    # no rpm → unrestricted
```

| Field | Type | Default | Description |
|---|---|---|---|
| `rpm` | `int` | `0` | Requests per minute. `0` means no limit. |

### Interaction with fallbacks

When a model has fallbacks configured, each candidate is rate-limited **independently**:

```yaml
model_list:
  - model_name: gpt4-with-fallback
    provider: openai
    model: gpt-4o
    rpm: 5
    fallbacks:
      - gpt-4o-mini   # must also be in model_list; its own rpm applies
```

If the current candidate's bucket is empty and there are more candidates available, NeuroClaw skips the locally saturated candidate and tries the next fallback immediately. Only the last remaining candidate waits for a token to refill. If the context deadline is hit while waiting on that last candidate, the wait error propagates.

For `model_list` aliases that resolve to the same underlying provider/model, rate limiting is keyed by the stable config identity (for example `model_name`) rather than the resolved runtime model string. This preserves distinct RPM settings for multi-key and alias-based configurations.

### Burst behaviour

The bucket starts **full** (burst = RPM). For `rpm: 3`, the first 3 requests fire instantly; subsequent requests are spaced ~20 s apart.

To reduce burstiness for strict APIs, set a lower `rpm` and rely on the steady-state refill.

## LLM call budget

`rpm` smooths how fast one model is called. The LLM call budget caps how many calls NeuroClaw makes
in total, across every model, per hour and per day. It is off by default:

```json
{
  "agents": {
    "defaults": {
      "llm_budget": {
        "max_calls_per_hour": 60,
        "max_calls_per_day": 500
      }
    }
  }
}
```

| Field | Type | Default | Description |
|---|---|---|---|
| `max_calls_per_hour` | `int` | `0` | LLM calls allowed per clock hour. `0` means no limit. |
| `max_calls_per_day` | `int` | `0` | LLM calls allowed per calendar day. `0` means no limit. |

Environment variables: `NEUROCLAW_AGENTS_DEFAULTS_LLM_BUDGET_MAX_CALLS_PER_HOUR`,
`NEUROCLAW_AGENTS_DEFAULTS_LLM_BUDGET_MAX_CALLS_PER_DAY`.

**What counts.** Every request an agent turn sends to a model counts (subagent turns too), retries
included, and so does a `/btw` side question. A request that walks a fallback chain counts once, whichever candidate answers.
Background LLM work that is not a turn (history summarization, seahorse compaction, evolution) is not
counted.

**Windows.** Both windows are fixed and follow local time: the hourly window resets at the top of each
hour (14:00, 15:00, …), the daily one at midnight. A fixed window is simple to explain ("resets at
15:00") but can let up to twice the hourly limit through around a boundary.

**When the budget is used up:**

- The model is not called. The user gets a plain reply instead of an error, for example
  `The hourly LLM call budget is used up (60 of 60 calls), so I am not calling the model now. It resets at 15:00 (local time).`
  A turn that runs out in the middle of its tool loop stops there with the same reply.
- NeuroClaw logs a warning `NeuroClaw llm_budget` with `window`, `used`, `limit` and `resets_at`.
- Cron jobs that run an agent turn, and heartbeat turns, are skipped before they start, with the same
  log line (`action=skipped`, `source=cron|heartbeat`). Cron jobs that run a shell command do not use
  the model and still run.

**Counters** are kept in memory. They restart from zero when the process restarts, and every process
has its own: `neuroclaw agent` or `neuroclaw heartbeat run` in a terminal does not share the
gateway's counters. A config reload keeps the counts and applies the new limits. `get_diagnostics`
shows the current counts and reset times.

The budget is global. There is no per-model budget: use `rpm` on a `model_list` entry to slow one
model down.

## Files changed

| File | What |
|---|---|
| `pkg/providers/ratelimiter.go` | `RateLimiter` (token bucket) + `RateLimiterRegistry` |
| `pkg/providers/ratelimiter_test.go` | Unit tests for limiter and registry |
| `pkg/providers/fallback.go` | `FallbackCandidate.RPM` field; `FallbackChain.rl`; `Wait()` call in `Execute`/`ExecuteImage` |
| `pkg/agent/model_resolution.go` | Resolves candidates from `model_list`, preserving stable config identity and propagating `RPM` into `FallbackCandidate` |
| `pkg/agent/loop.go` | Build `RateLimiterRegistry`, register all agents' candidates, pass to `NewFallbackChain` |
| `pkg/providers/llm_budget.go` | `LLMBudget`: hourly and daily call budget, `LLMBudgetExceededError` |
| `pkg/agent/llm_budget.go` | Budget checks for turns, `/btw`, heartbeat; the `NeuroClaw llm_budget` log line |
| `pkg/tools/cron.go` | Skip agent-turn cron jobs while the budget is used up |
