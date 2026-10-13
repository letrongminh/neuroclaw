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

**What counts.** Every request sent to a model counts as one call. The count is taken where the
request leaves NeuroClaw, in a wrapper around every provider the agents use
([`pkg/providers/llm_budget_provider.go`](../../pkg/providers/llm_budget_provider.go)), so the code
path does not matter:

- each request of an agent turn: every tool-loop round, every retry, and every candidate a fallback
  chain actually calls (a candidate skipped for cooldown or `rpm` is not called and not counted);
- streaming requests, and the plain request made when a stream fails and falls back;
- subagent turns (`spawn`, `subagent`), including the legacy subagent tool loop;
- `/btw` side questions;
- maintenance calls that use the agent's model: history summarization, seahorse compaction,
  evolution drafts and success judging. They are counted, not exempt, and they are refused like any
  other call when the budget is used up;
- each voice transcription, one per audio clip, whatever the backend (Whisper-compatible,
  ElevenLabs or an audio-capable model). When the budget is used up the clip is not transcribed and
  the message goes on without its transcript.

Not counted: web search requests (`web_search` providers, including Perplexity) and speech made by
`send_tts`.

**Windows.** Both windows are fixed. The hourly window is an absolute clock hour: it resets on the
hour (14:00, 15:00, …; in a zone with a half-hour offset, at :30). The daily window resets at local
midnight and is keyed by the calendar date, so a day with a daylight-saving change (23 or 25 hours) is
still one window, and the hour repeated when clocks go back is its own hourly window. Wall-clock time
is only used to tell the user when the budget resets. A fixed window is simple to explain ("resets at
15:00") but can let up to twice the hourly limit through around a boundary.

**When the budget is used up:**

- The model is not called. The user gets a plain reply instead of an error, for example
  `The hourly LLM call budget is used up (60 of 60 calls), so I am not calling the model now. It resets at 15:00 (local time).`
  or, for the daily window,
  `… It resets tomorrow at 00:00 (local time, 2026-10-15).`
  A turn that runs out in the middle of its tool loop stops there with the same reply. A refused
  call does not put a model in cooldown and does not move the fallback chain to the next model.
- NeuroClaw logs a warning `NeuroClaw llm_budget` with `window`, `used`, `limit`, `resets_at` and the
  model.
- A cron job that would run an agent turn is not lost: its message is sent to its chat as written,
  followed by a note that it was not processed by the model and why, and the job completes as usual
  (a one-time reminder is delivered once and removed). The log line has `action=delivered_as_written`.
  Cron jobs that run a shell command do not use the model and run as usual.
- Heartbeat turns are skipped before they start, with the log line `action=skipped`,
  `source=heartbeat`. `neuroclaw heartbeat run` exits with an error saying the heartbeat was skipped.

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
| `pkg/providers/llm_budget_provider.go` | `WithLLMBudget`: the provider wrapper that counts every request; the `NeuroClaw llm_budget` log line |
| `pkg/providers/fallback.go` | A budget refusal stops the chain without cooldown |
| `pkg/agent/llm_budget.go` | Wraps every agent provider; heartbeat skip |
| `pkg/agent/agent_transcribe.go` | Counts each voice transcription |
| `pkg/tools/cron.go` | Agent-turn cron jobs delivered as written while the budget is used up |
