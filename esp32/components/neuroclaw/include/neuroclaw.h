/*
 * NeuroClaw-ESP: the NeuroClaw brain on one ESP32-S3 (NeuroEdge Q-79).
 *
 * The same promises as the Go NeuroClaw, on a chip with no Linux and no cloud:
 *
 *   1. The only way to a device is the gate. Every device tool the model sees
 *      is one `nc_gate.tools` lists, and every call to it goes to
 *      `nc_gate.call`. The brain's own tools touch only its workspace on flash
 *      (memory, notes), its schedule and the public web — the Go NeuroClaw's
 *      read_file, write_file, append_file, edit_file, list_dir, cron,
 *      web_search and web_fetch. There is no shell, no GPIO, no I2C here; a
 *      scheduled job is a message to the brain, never a command; and web_fetch
 *      refuses the home's own network (nc_web.h), so it is no way around the
 *      gate either.
 *   2. Only the owner is heard. Telegram messages and button presses from any
 *      other user are dropped before they reach the model.
 *   3. When the gate asks the owner, the model is told plainly that nothing ran
 *      and that it cannot approve (nc_guidance, as pkg/tools/integration/
 *      neuroedge_result.go). The approval buttons are sent by this code, not by
 *      the model, and a press counts only when Telegram reports it from the
 *      owner's user id; the gate then decides again (`nc_gate.answer`).
 *
 * Brain and gate share one chip: an exploitable bug in this code (TLS, JSON)
 * reaches the gate too. NeuroEdge labels this mode `risk=single_chip`.
 *
 * Telegram and LLM client after zclaw (https://github.com/tnm/zclaw, MIT,
 * Copyright (c) 2026 Ted Nyman), rewritten around the gate.
 */
#ifndef NEUROCLAW_H
#define NEUROCLAW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* What the gate answered, shaped like NeuroEdge's tool-result.v1. */
typedef enum {
    NC_ALLOW = 0,
    NC_BLOCK,
    NC_REJECTED, /* unknown tool or arguments outside its schema: nothing ran */
} nc_status;

typedef struct {
    nc_status status;
    char reason[48];           /* e.g. "criterion_failed", "awaiting_confirmation" */
    char failed_criterion[48]; /* "" when none */
    char message[192];         /* the gate's own words, shown to the model */
    bool asks_owner;           /* BLOCK, and the owner may approve this one call */
    char confirmation_id[32];
    char confirmation_message[192];
    uint32_t expires_in_ms;
} nc_tool_result;

typedef struct {
    const char *name;
    const char *description;
    const char *parameters; /* JSON Schema of the arguments, an object */
} nc_tool;

typedef struct {
    void *ctx;
    /* Every tool the model may call. */
    size_t (*tools)(void *ctx, const nc_tool **out);
    /* Decide one call. Must fill `out`; an unknown name is NC_REJECTED. */
    void (*call)(void *ctx, const char *name, const char *arguments_json, nc_tool_result *out);
    /* The owner pressed a button for `confirmation_id`. The gate checks that the
     * id is pending and not expired, and decides the call again. */
    void (*answer)(void *ctx, const char *confirmation_id, bool approve, nc_tool_result *out);
} nc_gate;

typedef struct {
    const char *telegram_token;
    const char *owner_id;     /* the owner's Telegram user id; empty refuses to start */
    const char *telegram_api; /* NULL: https://api.telegram.org */
    const char *llm_url;      /* OpenAI-compatible chat completions URL */
    const char *llm_key;      /* may be empty for a local model */
    const char *llm_model;
    const char *system_prompt; /* NULL: the built-in NeuroClaw prompt */
    const char *workspace;     /* a mounted directory (LittleFS) for memory, sessions, cron */
    const char *tz;            /* POSIX TZ for reminders and notes, e.g. "<+07>-7"; NULL: UTC */
    const char *search_provider; /* "searxng" or "brave"; NULL: no web_search tool */
    const char *search_url;      /* SearXNG base URL */
    const char *search_key;      /* Brave API key */
    const char *fetch_allow;     /* comma-separated private hosts web_fetch may reach anyway */
} nc_config;

/* Copy the configuration and start the brain task. ESP_ERR_INVALID_ARG when a
 * field it cannot work without is missing (token, owner, LLM URL, model). */
int neuroclaw_start(const nc_config *config, const nc_gate *gate);

#ifdef __cplusplus
}
#endif

#endif /* NEUROCLAW_H */
