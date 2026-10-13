/*
 * NeuroClaw-ESP decisions without ESP-IDF state (nc_core.h). The guidance text is
 * the Go NeuroClaw's (pkg/tools/integration/neuroedge_result.go), with the
 * approval moved from "the page at home" to the button sent in Telegram.
 */
#include "nc_core.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const NC_SYSTEM_PROMPT =
    "You are NeuroClaw, the assistant of a NeuroEdge home, running on the home's own chip. "
    "You talk with the owner on Telegram. You can act on the home only through the tools "
    "you are given; each one is decided by the home's gate, which may refuse or ask the owner "
    "first. Never say something was done unless the tool result says ALLOW. Answer briefly, "
    "in the owner's language.";

const char *nc_status_name(nc_status status)
{
    switch (status) {
    case NC_ALLOW: return "ALLOW";
    case NC_BLOCK: return "BLOCK";
    case NC_REJECTED: return "REJECTED";
    }
    return "REJECTED";
}

/* `text` in double quotes, `"` and `\` escaped, control bytes dropped. */
static void quote(char *out, size_t cap, const char *text)
{
    size_t n = 0;
    if (cap < 3) {
        if (cap > 0) out[0] = '\0';
        return;
    }
    out[n++] = '"';
    for (const char *c = text ? text : ""; *c != '\0' && n + 3 < cap; c++) {
        const unsigned char byte = (unsigned char)*c;
        if (byte < 0x20u) continue;
        if (*c == '"' || *c == '\\') out[n++] = '\\';
        out[n++] = *c;
    }
    out[n++] = '"';
    out[n] = '\0';
}

int nc_guidance(const nc_tool_result *result, char *out, size_t cap)
{
    if (out == NULL || cap == 0) return -1;
    out[0] = '\0';
    if (result == NULL) return -1;
    int n = 0;
    if (result->status == NC_BLOCK && result->asks_owner) {
        char quoted[sizeof result->confirmation_message + 8];
        quote(quoted, sizeof quoted, result->confirmation_message);
        n = snprintf(out, cap,
                     "[NeuroEdge] The home did NOT do this yet. Its rule asks the owner first: %s. "
                     "NeuroClaw has sent the owner an approval button in Telegram; they must press "
                     "it within about %" PRIu32 " seconds. You cannot approve it, and a \"yes\" "
                     "typed in this chat does not count. Tell the user it is waiting for their "
                     "approval. Do not call the tool again for this request and do not say it is "
                     "done.",
                     quoted, result->expires_in_ms / 1000u);
    } else if (result->status == NC_BLOCK) {
        char why[sizeof result->reason + sizeof result->failed_criterion + 4];
        if (result->failed_criterion[0] != '\0') {
            snprintf(why, sizeof why, "%s (%s)", result->reason, result->failed_criterion);
        } else {
            snprintf(why, sizeof why, "%s", result->reason);
        }
        n = snprintf(out, cap,
                     "[NeuroEdge] The home refused this: %s. %s%sTell the user it was not done "
                     "and why. Do not retry with other arguments or look for another way around "
                     "the rule.",
                     why, result->message, result->message[0] != '\0' ? " " : "");
    } else if (result->status == NC_REJECTED) {
        n = snprintf(out, cap,
                     "[NeuroEdge] The home rejected the call itself (unknown tool or arguments "
                     "outside its schema), so nothing ran. Tell the user it was not done.");
    }
    if (n < 0 || (size_t)n >= cap) {
        out[0] = '\0';
        return -1;
    }
    return n;
}

char *nc_tool_content(const nc_tool_result *result)
{
    if (result == NULL) return NULL;
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) return NULL;
    cJSON_AddStringToObject(json, "status", nc_status_name(result->status));
    if (result->reason[0] != '\0') cJSON_AddStringToObject(json, "reason", result->reason);
    if (result->failed_criterion[0] != '\0') {
        cJSON_AddStringToObject(json, "failed_criterion", result->failed_criterion);
    }
    if (result->message[0] != '\0') cJSON_AddStringToObject(json, "message", result->message);
    if (result->status == NC_BLOCK && result->asks_owner) {
        cJSON *c = cJSON_AddObjectToObject(json, "confirmation");
        if (c != NULL) {
            cJSON_AddStringToObject(c, "id", result->confirmation_id);
            cJSON_AddStringToObject(c, "message", result->confirmation_message);
            cJSON_AddNumberToObject(c, "expires_in_ms", (double)result->expires_in_ms);
            cJSON_AddStringToObject(c, "who", "owner");
        }
    }
    char *body = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (body == NULL) return NULL;

    char guidance[1024];
    if (nc_guidance(result, guidance, sizeof guidance) < 0) guidance[0] = '\0';
    const size_t g = strlen(guidance);
    const size_t b = strlen(body);
    char *out = malloc(g + 1 + b + 1);
    if (out == NULL) {
        cJSON_free(body);
        return NULL;
    }
    size_t n = 0;
    if (g > 0) {
        memcpy(out, guidance, g);
        out[g] = '\n';
        n = g + 1;
    }
    memcpy(out + n, body, b + 1);
    cJSON_free(body);
    return out;
}

/* --- Telegram ------------------------------------------------------------------------------ */

static void copy(char *out, size_t cap, const char *text)
{
    if (cap == 0) return;
    size_t n = text ? strlen(text) : 0;
    if (n >= cap) n = cap - 1;
    if (n > 0) memcpy(out, text, n);
    out[n] = '\0';
}

/* A Telegram id as digits; false when it is not an integer. */
static bool id_text(const cJSON *id, char *out, size_t cap)
{
    if (!cJSON_IsNumber(id)) return false;
    const double v = id->valuedouble;
    if (v != (double)(int64_t)v) return false;
    snprintf(out, cap, "%" PRId64, (int64_t)v);
    return true;
}

static int64_t id_value(const cJSON *id)
{
    return cJSON_IsNumber(id) ? (int64_t)id->valuedouble : 0;
}

static bool is_owner(const char *from, const char *owner_id)
{
    return owner_id != NULL && owner_id[0] != '\0' && strcmp(from, owner_id) == 0;
}

int nc_telegram_parse(const char *body, const char *owner_id, nc_event *events, int max,
                      int64_t *next_offset)
{
    if (body == NULL || events == NULL || next_offset == NULL) return -1;
    cJSON *root = cJSON_Parse(body);
    if (root == NULL) return -1;
    const cJSON *ok = cJSON_GetObjectItemCaseSensitive(root, "ok");
    const cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "result");
    if (!cJSON_IsTrue(ok) || !cJSON_IsArray(result)) {
        cJSON_Delete(root);
        return -1;
    }
    int count = 0;
    const cJSON *update = NULL;
    cJSON_ArrayForEach(update, result)
    {
        const cJSON *update_id = cJSON_GetObjectItemCaseSensitive(update, "update_id");
        if (cJSON_IsNumber(update_id)) {
            const int64_t next = id_value(update_id) + 1;
            if (next > *next_offset) *next_offset = next;
        }
        if (count >= max) continue; /* the offset still moves past it */
        nc_event *e = &events[count];
        memset(e, 0, sizeof *e);
        e->kind = NC_EVENT_IGNORED;

        const cJSON *message = cJSON_GetObjectItemCaseSensitive(update, "message");
        const cJSON *callback = cJSON_GetObjectItemCaseSensitive(update, "callback_query");
        if (cJSON_IsObject(message)) {
            const cJSON *from = cJSON_GetObjectItemCaseSensitive(message, "from");
            const cJSON *chat = cJSON_GetObjectItemCaseSensitive(message, "chat");
            const cJSON *type = cJSON_GetObjectItemCaseSensitive(chat, "type");
            const cJSON *text = cJSON_GetObjectItemCaseSensitive(message, "text");
            id_text(cJSON_GetObjectItemCaseSensitive(from, "id"), e->from, sizeof e->from);
            e->chat_id = id_value(cJSON_GetObjectItemCaseSensitive(chat, "id"));
            if (is_owner(e->from, owner_id) && cJSON_IsString(type) &&
                strcmp(type->valuestring, "private") == 0 && cJSON_IsString(text) &&
                text->valuestring[0] != '\0') {
                e->kind = NC_EVENT_TEXT;
                copy(e->text, sizeof e->text, text->valuestring);
            }
        } else if (cJSON_IsObject(callback)) {
            const cJSON *from = cJSON_GetObjectItemCaseSensitive(callback, "from");
            const cJSON *data = cJSON_GetObjectItemCaseSensitive(callback, "data");
            const cJSON *id = cJSON_GetObjectItemCaseSensitive(callback, "id");
            const cJSON *msg = cJSON_GetObjectItemCaseSensitive(callback, "message");
            const cJSON *chat = cJSON_GetObjectItemCaseSensitive(msg, "chat");
            id_text(cJSON_GetObjectItemCaseSensitive(from, "id"), e->from, sizeof e->from);
            e->chat_id = id_value(cJSON_GetObjectItemCaseSensitive(chat, "id"));
            if (cJSON_IsString(id)) copy(e->callback_id, sizeof e->callback_id, id->valuestring);
            if (is_owner(e->from, owner_id) && cJSON_IsString(data) &&
                nc_button_parse(data->valuestring, &e->approve, e->confirmation_id,
                                sizeof e->confirmation_id)) {
                e->kind = NC_EVENT_BUTTON;
            }
        }
        count++;
    }
    cJSON_Delete(root);
    return count;
}

int nc_button_data(char *out, size_t cap, bool approve, const char *confirmation_id)
{
    if (out == NULL || cap == 0 || confirmation_id == NULL) return -1;
    const int n = snprintf(out, cap, "nc:%c:%s", approve ? 'y' : 'n', confirmation_id);
    /* Telegram limits callback_data to 64 bytes. */
    if (n < 0 || (size_t)n >= cap || n > 64) {
        out[0] = '\0';
        return -1;
    }
    return n;
}

bool nc_button_parse(const char *data, bool *approve, char *confirmation_id, size_t cap)
{
    if (data == NULL || approve == NULL || confirmation_id == NULL || cap == 0) return false;
    if (strncmp(data, "nc:", 3) != 0 || (data[3] != 'y' && data[3] != 'n') || data[4] != ':') {
        return false;
    }
    const char *id = data + 5;
    const size_t len = strlen(id);
    if (len == 0 || len >= cap) return false;
    for (const char *c = id; *c != '\0'; c++) {
        const bool ok = (*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'z') ||
                        (*c >= 'A' && *c <= 'Z') || *c == '-' || *c == '_';
        if (!ok) return false;
    }
    *approve = data[3] == 'y';
    memcpy(confirmation_id, id, len + 1);
    return true;
}

char *nc_telegram_message(int64_t chat_id, const char *text, const char *confirmation_id)
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) return NULL;
    cJSON_AddNumberToObject(json, "chat_id", (double)chat_id);
    cJSON_AddStringToObject(json, "text", text ? text : "");
    if (confirmation_id != NULL && confirmation_id[0] != '\0') {
        char yes[72], no[72];
        if (nc_button_data(yes, sizeof yes, true, confirmation_id) < 0 ||
            nc_button_data(no, sizeof no, false, confirmation_id) < 0) {
            cJSON_Delete(json);
            return NULL;
        }
        cJSON *markup = cJSON_AddObjectToObject(json, "reply_markup");
        cJSON *rows = cJSON_AddArrayToObject(markup, "inline_keyboard");
        cJSON *row = cJSON_CreateArray();
        cJSON *b1 = cJSON_CreateObject();
        cJSON_AddStringToObject(b1, "text", "\xE2\x9C\x85 \xC4\x90\xE1\xBB\x93ng \xC3\xBD"); /* ✅ Đồng ý */
        cJSON_AddStringToObject(b1, "callback_data", yes);
        cJSON *b2 = cJSON_CreateObject();
        cJSON_AddStringToObject(b2, "text", "\xE2\x9D\x8C T\xE1\xBB\xAB ch\xE1\xBB\x91i"); /* ❌ Từ chối */
        cJSON_AddStringToObject(b2, "callback_data", no);
        cJSON_AddItemToArray(row, b1);
        cJSON_AddItemToArray(row, b2);
        cJSON_AddItemToArray(rows, row);
    }
    char *body = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    return body;
}

/* --- the LLM ------------------------------------------------------------------------------- */

char *nc_llm_request(const char *model, const cJSON *messages, const nc_tool *tools,
                     size_t tool_count)
{
    if (model == NULL || messages == NULL) return NULL;
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) return NULL;
    cJSON_AddStringToObject(json, "model", model);
    cJSON_AddItemToObject(json, "messages", cJSON_Duplicate(messages, true));
    if (tool_count > 0 && tools != NULL) {
        cJSON *list = cJSON_AddArrayToObject(json, "tools");
        for (size_t i = 0; i < tool_count; i++) {
            cJSON *tool = cJSON_CreateObject();
            cJSON_AddStringToObject(tool, "type", "function");
            cJSON *fn = cJSON_AddObjectToObject(tool, "function");
            cJSON_AddStringToObject(fn, "name", tools[i].name);
            cJSON_AddStringToObject(fn, "description",
                                    tools[i].description ? tools[i].description : "");
            cJSON *params = tools[i].parameters ? cJSON_Parse(tools[i].parameters) : NULL;
            if (!cJSON_IsObject(params)) {
                cJSON_Delete(params);
                params = cJSON_Parse("{\"type\":\"object\",\"properties\":{}}");
            }
            cJSON_AddItemToObject(fn, "parameters", params);
            cJSON_AddItemToArray(list, tool);
        }
        cJSON_AddStringToObject(json, "tool_choice", "auto");
    }
    char *body = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    return body;
}

static char *dup_text(const char *text)
{
    const size_t n = strlen(text);
    char *out = malloc(n + 1);
    if (out != NULL) memcpy(out, text, n + 1);
    return out;
}

int nc_llm_parse(const char *body, nc_llm_reply *reply)
{
    if (reply == NULL) return -1;
    memset(reply, 0, sizeof *reply);
    if (body == NULL) return -1;
    cJSON *root = cJSON_Parse(body);
    if (root == NULL) return -1;
    const cJSON *choices = cJSON_GetObjectItemCaseSensitive(root, "choices");
    const cJSON *message =
        cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(choices, 0), "message");
    const cJSON *calls = cJSON_GetObjectItemCaseSensitive(message, "tool_calls");
    const cJSON *call = NULL;
    cJSON_ArrayForEach(call, calls)
    {
        if (reply->call_count >= NC_MAX_TOOL_CALLS) break;
        const cJSON *id = cJSON_GetObjectItemCaseSensitive(call, "id");
        const cJSON *fn = cJSON_GetObjectItemCaseSensitive(call, "function");
        const cJSON *name = cJSON_GetObjectItemCaseSensitive(fn, "name");
        const cJSON *args = cJSON_GetObjectItemCaseSensitive(fn, "arguments");
        if (!cJSON_IsString(name) || name->valuestring[0] == '\0') continue;
        nc_tool_call *c = &reply->calls[reply->call_count];
        copy(c->id, sizeof c->id, cJSON_IsString(id) ? id->valuestring : "call");
        copy(c->name, sizeof c->name, name->valuestring);
        c->arguments = dup_text(cJSON_IsString(args) && args->valuestring[0] ? args->valuestring
                                                                              : "{}");
        if (c->arguments == NULL) break;
        reply->call_count++;
    }
    const cJSON *content = cJSON_GetObjectItemCaseSensitive(message, "content");
    if (reply->call_count == 0 && cJSON_IsString(content) && content->valuestring[0] != '\0') {
        reply->text = dup_text(content->valuestring);
    }
    cJSON_Delete(root);
    return (reply->call_count > 0 || reply->text != NULL) ? 0 : -1;
}

void nc_llm_reply_free(nc_llm_reply *reply)
{
    if (reply == NULL) return;
    free(reply->text);
    for (int i = 0; i < reply->call_count; i++) free(reply->calls[i].arguments);
    memset(reply, 0, sizeof *reply);
}

/* --- the conversation ---------------------------------------------------------------------- */

static bool is_role(const cJSON *message, const char *role)
{
    const cJSON *r = cJSON_GetObjectItemCaseSensitive(message, "role");
    return cJSON_IsString(r) && strcmp(r->valuestring, role) == 0;
}

void nc_history_trim(cJSON *messages, int keep)
{
    if (!cJSON_IsArray(messages) || keep < 1) return;
    while (cJSON_GetArraySize(messages) - 1 > keep) cJSON_DeleteItemFromArray(messages, 1);
    while (cJSON_GetArraySize(messages) > 1 && is_role(cJSON_GetArrayItem(messages, 1), "tool")) {
        cJSON_DeleteItemFromArray(messages, 1);
    }
}
