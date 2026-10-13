/*
 * NeuroClaw-ESP's second LLM wire format: the Anthropic Messages API, as the Go
 * NeuroClaw's anthropic_messages provider (pkg/providers/anthropic_messages).
 * The conversation stays in the chat-completions shape the rest of the brain
 * uses (nc_core.h); it is converted here on the way out and back, so the
 * primary and the fallback URL can each speak either format.
 *
 * Plain C99 and cJSON; tested on the host (esp32/scripts/host_test.sh).
 */
#ifndef NC_LLM_H
#define NC_LLM_H

#include <stddef.h>

#include "cJSON.h"
#include "nc_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NC_API_OPENAI = 0, /* chat completions: Authorization: Bearer <key> */
    NC_API_ANTHROPIC,  /* messages: x-api-key: <key>, anthropic-version */
} nc_llm_api;

#define NC_ANTHROPIC_VERSION "2023-06-01"

/* "openai" (or NULL, or "") and "anthropic"; -1 for anything else. */
int nc_llm_api_parse(const char *name);

/*
 * The Messages body for chat-completions `messages` (borrowed): every system
 * message joined into `system`; user and assistant text as text blocks;
 * assistant tool_calls as tool_use blocks (arguments parsed into `input`);
 * tool messages as tool_result blocks in a user message. Consecutive messages
 * of one role are merged (the API wants them alternating), empty text is left
 * out, and a conversation whose first kept message is the assistant's starts
 * with a short user note. Tool call ids are kept, with any character outside
 * [A-Za-z0-9_-] replaced by '_', the same way in tool_use and tool_result.
 * Tools as name, description, input_schema. malloc'd.
 */
char *nc_anthropic_request(const char *model, int max_tokens, const cJSON *messages,
                           const nc_tool *tools, size_t tool_count);

/* Parse a Messages reply: text blocks joined, tool_use blocks as calls (their
 * input printed back to JSON text). As nc_llm_parse: text only when there is no
 * call; 0 on success, -1 when there is neither. */
int nc_anthropic_parse(const char *body, nc_llm_reply *reply);

#ifdef __cplusplus
}
#endif

#endif /* NC_LLM_H */
