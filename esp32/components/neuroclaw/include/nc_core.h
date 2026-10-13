/*
 * NeuroClaw-ESP decisions without ESP-IDF state: what the model is told about a
 * gate result, which Telegram updates are heard, what an approval button
 * carries, and the LLM request and reply. Plain C99 and cJSON; compiled on the
 * host with sanitizers by esp32/scripts/host_test.sh.
 */
#ifndef NC_CORE_H
#define NC_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cJSON.h"
#include "neuroclaw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NC_TEXT_MAX 1024u   /* one owner message, NUL included */
#define NC_ID_MAX 32u
#define NC_MAX_EVENTS 8
#define NC_MAX_TOOL_CALLS 4

/* --- what the model is told -------------------------------------------------------------- */

/* "" for ALLOW; for BLOCK and REJECTED the fixed instructions of the Go NeuroClaw. */
int nc_guidance(const nc_tool_result *result, char *out, size_t cap);

/* The tool message content: the guidance (if any), a newline, then the result
 * as JSON (status, reason, failed_criterion, message, confirmation). malloc'd. */
char *nc_tool_content(const nc_tool_result *result);

const char *nc_status_name(nc_status status);

/* --- Telegram ---------------------------------------------------------------------------- */

typedef enum {
    NC_EVENT_TEXT = 0,  /* the owner wrote */
    NC_EVENT_BUTTON,    /* the owner pressed an approval button */
    NC_EVENT_IGNORED,   /* someone else, a group, an edit, a sticker...: dropped */
} nc_event_kind;

typedef struct {
    nc_event_kind kind;
    int64_t chat_id;
    char from[24];               /* the sender's user id, as digits */
    char text[NC_TEXT_MAX];      /* TEXT */
    char callback_id[64];        /* BUTTON: to acknowledge the press */
    bool approve;                /* BUTTON */
    char confirmation_id[NC_ID_MAX]; /* BUTTON */
} nc_event;

/*
 * Parse a getUpdates reply. Heard: a text message in a private chat from
 * `owner_id`, and a press of one of our approval buttons by `owner_id`.
 * Everything else is NC_EVENT_IGNORED, with `from` set when known.
 * `*next_offset` is the offset for the next getUpdates (update_id + 1 of the
 * last update returned in `events`, kept when there is none): updates beyond
 * `max` are not skipped, they come again. Returns the number of events, or -1
 * when the body is not a Telegram reply with ok=true.
 */
int nc_telegram_parse(const char *body, const char *owner_id, nc_event *events, int max,
                      int64_t *next_offset);

/* Approval button data: "nc:y:<id>" / "nc:n:<id>". */
int nc_button_data(char *out, size_t cap, bool approve, const char *confirmation_id);
bool nc_button_parse(const char *data, bool *approve, char *confirmation_id, size_t cap);

/* sendMessage body; with `confirmation_id`, two inline buttons (approve, refuse). malloc'd. */
char *nc_telegram_message(int64_t chat_id, const char *text, const char *confirmation_id);

/* --- the LLM ----------------------------------------------------------------------------- */

typedef struct {
    char id[64];
    char name[64];
    char *arguments; /* malloc'd JSON text */
} nc_tool_call;

typedef struct {
    char *text; /* malloc'd; NULL when the model called tools */
    nc_tool_call calls[NC_MAX_TOOL_CALLS];
    int call_count;
} nc_llm_reply;

/* The chat completions body: model, messages (borrowed), tools from the gate. malloc'd. */
char *nc_llm_request(const char *model, const cJSON *messages, const nc_tool *tools,
                     size_t tool_count);

/* Parse choices[0].message. 0 on success, -1 when there is neither text nor a call. */
int nc_llm_parse(const char *body, nc_llm_reply *reply);
void nc_llm_reply_free(nc_llm_reply *reply);

/* Whether a failed LLM call should go to the other URL, as the Go NeuroClaw's
 * fallback chain decides (pkg/providers/error_classifier.go): the reason
 * ("network", "auth", "billing", "timeout", "rate_limit", "transient"), or NULL
 * for a reply the other model would refuse too (400, an unknown status) and
 * for success. `status` is the HTTP status, -1 when nothing usable came back. */
const char *nc_llm_failover(int status);

/* Seconds a failed URL is passed over, as the Go cooldown: 1 min, 5, 25, then
 * 1 h; for billing 5 h doubling to 24 h. `failures` counts from 1. */
int64_t nc_llm_cooldown_s(int failures, bool billing);

/* LLM call limits per hour and per day, as zclaw's rate limit: fixed windows
 * that start with the first call in them, on the uptime clock (no wall clock
 * needed). 0 = no limit. */
typedef struct {
    int64_t hour_start, day_start; /* uptime seconds; 0 = no window open */
    int hour_count, day_count;
    int per_hour, per_day;
    int64_t total;                 /* calls counted since boot */
    int64_t refused;               /* calls refused since boot */
} nc_rate;

/* Count one LLM call at uptime `now_s` when it fits: NULL. When a limit is
 * reached, nothing is counted and the window that is full, "hour" or "day";
 * `*wait_s` (may be NULL) gets the seconds until that window ends. */
const char *nc_rate_take(nc_rate *rate, int64_t now_s, int64_t *wait_s);

/* What get_diagnostics reports; filled by the brain, printed here. */
typedef struct {
    int64_t uptime_s;
    uint32_t heap_free, heap_min_free, heap_largest_block;
    int32_t stack_free_min; /* the brain task's, bytes; -1 unknown */
    bool has_rssi;
    int rssi_dbm;
    int llm_count; /* 1 or 2 */
    struct {
        const char *name, *api;
        int64_t calls, failures_total;
        int failures; /* in a row */
        int64_t cooldown_s; /* left; 0 when usable */
    } llm[2];
    const nc_rate *rate;
    int tools, jobs, history;
    const char *last_error; /* "" when none */
} nc_diag;

/* The tool result: one JSON object. malloc'd. */
char *nc_diagnostics_text(const nc_diag *diag);

/* --- the conversation -------------------------------------------------------------------- */

/* The built-in system prompt. */
extern const char *const NC_SYSTEM_PROMPT;

/* Keep the system message and at most `keep` messages after it, never starting
 * the kept part on a tool message (its call would be gone). */
void nc_history_trim(cJSON *messages, int keep);

/* How many messages after the system one nc_history_trim(messages, keep) would
 * drop (0 when none). */
int nc_history_overflow(const cJSON *messages, int keep);

/* The instruction and the text of a summary request, as the Go NeuroClaw keeps
 * a CONTEXT_SUMMARY of what no longer fits: the previous summary (may be NULL)
 * and messages[1 .. 1+count) as "role: content" lines, each cut to 300 bytes.
 * malloc'd. */
extern const char *const NC_SUMMARY_INSTRUCTION;
char *nc_summary_input(const char *previous, const cJSON *messages, int count);

#ifdef __cplusplus
}
#endif

#endif /* NC_CORE_H */
