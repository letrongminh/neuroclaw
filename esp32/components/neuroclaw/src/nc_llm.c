/*
 * The Anthropic Messages format for NeuroClaw-ESP (nc_llm.h). The conversion
 * follows the Go NeuroClaw's pkg/providers/anthropic_messages/provider.go.
 */
#include "nc_llm.h"

#include <stdlib.h>
#include <string.h>

int nc_llm_api_parse(const char *name)
{
    if (name == NULL || name[0] == '\0' || strcmp(name, "openai") == 0) return NC_API_OPENAI;
    if (strcmp(name, "anthropic") == 0) return NC_API_ANTHROPIC;
    return -1;
}

static void copy(char *out, size_t cap, const char *text)
{
    size_t n = strlen(text);
    if (n >= cap) n = cap - 1;
    memcpy(out, text, n);
    out[n] = '\0';
}

/* An id the API accepts: [A-Za-z0-9_-], never empty. */
static void safe_id(char *out, size_t cap, const char *id)
{
    copy(out, cap, id != NULL && id[0] != '\0' ? id : "call");
    for (char *c = out; *c; c++) {
        const char ch = *c;
        const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                        (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
        if (!ok) *c = '_';
    }
}

static const char *text_of(const cJSON *message)
{
    const cJSON *content = cJSON_GetObjectItemCaseSensitive(message, "content");
    return cJSON_IsString(content) ? content->valuestring : NULL;
}

/* The content array of the last message when it has `role`, else a new message. */
static cJSON *blocks_for(cJSON *out, const char *role)
{
    const int n = cJSON_GetArraySize(out);
    cJSON *last = n > 0 ? cJSON_GetArrayItem(out, n - 1) : NULL;
    const cJSON *r = cJSON_GetObjectItemCaseSensitive(last, "role");
    if (cJSON_IsString(r) && strcmp(r->valuestring, role) == 0) {
        return cJSON_GetObjectItemCaseSensitive(last, "content");
    }
    cJSON *m = cJSON_CreateObject();
    cJSON_AddStringToObject(m, "role", role);
    cJSON *content = cJSON_AddArrayToObject(m, "content");
    cJSON_AddItemToArray(out, m);
    return content;
}

static void add_text(cJSON *blocks, const char *text)
{
    cJSON *b = cJSON_CreateObject();
    cJSON_AddStringToObject(b, "type", "text");
    cJSON_AddStringToObject(b, "text", text);
    cJSON_AddItemToArray(blocks, b);
}

char *nc_anthropic_request(const char *model, int max_tokens, const cJSON *messages,
                           const nc_tool *tools, size_t tool_count)
{
    if (model == NULL || !cJSON_IsArray(messages)) return NULL;
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) return NULL;
    cJSON_AddStringToObject(json, "model", model);
    cJSON_AddNumberToObject(json, "max_tokens", max_tokens > 0 ? max_tokens : 1024);

    size_t system_len = 0;
    const cJSON *m = NULL;
    cJSON_ArrayForEach(m, messages)
    {
        const cJSON *role = cJSON_GetObjectItemCaseSensitive(m, "role");
        const char *text = text_of(m);
        if (cJSON_IsString(role) && strcmp(role->valuestring, "system") == 0 && text != NULL) {
            system_len += strlen(text) + 2;
        }
    }
    char *system = system_len > 0 ? malloc(system_len + 1) : NULL;
    if (system != NULL) system[0] = '\0';

    cJSON *out = cJSON_AddArrayToObject(json, "messages");
    cJSON_ArrayForEach(m, messages)
    {
        const cJSON *role = cJSON_GetObjectItemCaseSensitive(m, "role");
        if (!cJSON_IsString(role)) continue;
        const char *r = role->valuestring;
        const char *text = text_of(m);
        if (strcmp(r, "system") == 0) {
            if (system != NULL && text != NULL && text[0] != '\0') {
                if (system[0] != '\0') strcat(system, "\n\n");
                strcat(system, text);
            }
        } else if (strcmp(r, "user") == 0) {
            if (text != NULL && text[0] != '\0') add_text(blocks_for(out, "user"), text);
        } else if (strcmp(r, "assistant") == 0) {
            const cJSON *calls = cJSON_GetObjectItemCaseSensitive(m, "tool_calls");
            const bool has_text = text != NULL && text[0] != '\0';
            if (!has_text && cJSON_GetArraySize(calls) == 0) continue;
            if (cJSON_GetArraySize(out) == 0) {
                /* The API wants the user first: what came before was trimmed. */
                add_text(blocks_for(out, "user"), "(Earlier conversation omitted.)");
            }
            cJSON *blocks = blocks_for(out, "assistant");
            if (has_text) add_text(blocks, text);
            const cJSON *call = NULL;
            cJSON_ArrayForEach(call, calls)
            {
                const cJSON *fn = cJSON_GetObjectItemCaseSensitive(call, "function");
                const cJSON *name = cJSON_GetObjectItemCaseSensitive(fn, "name");
                const cJSON *args = cJSON_GetObjectItemCaseSensitive(fn, "arguments");
                const cJSON *id = cJSON_GetObjectItemCaseSensitive(call, "id");
                if (!cJSON_IsString(name) || name->valuestring[0] == '\0') continue;
                char sid[64];
                safe_id(sid, sizeof sid, cJSON_IsString(id) ? id->valuestring : NULL);
                cJSON *input = cJSON_IsString(args) ? cJSON_Parse(args->valuestring) : NULL;
                if (!cJSON_IsObject(input)) {
                    cJSON_Delete(input);
                    input = cJSON_CreateObject();
                }
                cJSON *b = cJSON_CreateObject();
                cJSON_AddStringToObject(b, "type", "tool_use");
                cJSON_AddStringToObject(b, "id", sid);
                cJSON_AddStringToObject(b, "name", name->valuestring);
                cJSON_AddItemToObject(b, "input", input);
                cJSON_AddItemToArray(blocks, b);
            }
        } else if (strcmp(r, "tool") == 0) {
            const cJSON *id = cJSON_GetObjectItemCaseSensitive(m, "tool_call_id");
            char sid[64];
            safe_id(sid, sizeof sid, cJSON_IsString(id) ? id->valuestring : NULL);
            cJSON *b = cJSON_CreateObject();
            cJSON_AddStringToObject(b, "type", "tool_result");
            cJSON_AddStringToObject(b, "tool_use_id", sid);
            cJSON_AddStringToObject(b, "content", text != NULL ? text : "");
            cJSON_AddItemToArray(blocks_for(out, "user"), b);
        }
    }
    if (system != NULL && system[0] != '\0') cJSON_AddStringToObject(json, "system", system);
    free(system);

    if (tool_count > 0 && tools != NULL) {
        cJSON *list = cJSON_AddArrayToObject(json, "tools");
        for (size_t i = 0; i < tool_count; i++) {
            cJSON *tool = cJSON_CreateObject();
            cJSON_AddStringToObject(tool, "name", tools[i].name);
            cJSON_AddStringToObject(tool, "description",
                                    tools[i].description ? tools[i].description : "");
            cJSON *schema = tools[i].parameters ? cJSON_Parse(tools[i].parameters) : NULL;
            if (!cJSON_IsObject(schema)) {
                cJSON_Delete(schema);
                schema = cJSON_Parse("{\"type\":\"object\",\"properties\":{}}");
            }
            cJSON_AddItemToObject(tool, "input_schema", schema);
            cJSON_AddItemToArray(list, tool);
        }
    }
    char *body = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    return body;
}

int nc_anthropic_parse(const char *body, nc_llm_reply *reply)
{
    if (reply == NULL) return -1;
    memset(reply, 0, sizeof *reply);
    if (body == NULL) return -1;
    cJSON *root = cJSON_Parse(body);
    if (root == NULL) return -1;
    const cJSON *content = cJSON_GetObjectItemCaseSensitive(root, "content");
    size_t text_len = 0;
    const cJSON *b = NULL;
    cJSON_ArrayForEach(b, content)
    {
        const cJSON *type = cJSON_GetObjectItemCaseSensitive(b, "type");
        if (!cJSON_IsString(type)) continue;
        if (strcmp(type->valuestring, "text") == 0) {
            const cJSON *text = cJSON_GetObjectItemCaseSensitive(b, "text");
            if (cJSON_IsString(text)) text_len += strlen(text->valuestring);
        } else if (strcmp(type->valuestring, "tool_use") == 0 &&
                   reply->call_count < NC_MAX_TOOL_CALLS) {
            const cJSON *id = cJSON_GetObjectItemCaseSensitive(b, "id");
            const cJSON *name = cJSON_GetObjectItemCaseSensitive(b, "name");
            const cJSON *input = cJSON_GetObjectItemCaseSensitive(b, "input");
            if (!cJSON_IsString(name) || name->valuestring[0] == '\0') continue;
            nc_tool_call *c = &reply->calls[reply->call_count];
            copy(c->id, sizeof c->id, cJSON_IsString(id) ? id->valuestring : "call");
            copy(c->name, sizeof c->name, name->valuestring);
            char *args = cJSON_IsObject(input) ? cJSON_PrintUnformatted(input) : NULL;
            c->arguments = malloc(args != NULL ? strlen(args) + 1 : 3);
            if (c->arguments == NULL) {
                cJSON_free(args);
                break;
            }
            strcpy(c->arguments, args != NULL ? args : "{}");
            cJSON_free(args);
            reply->call_count++;
        }
    }
    if (reply->call_count == 0 && text_len > 0) {
        reply->text = malloc(text_len + 1);
        if (reply->text != NULL) {
            reply->text[0] = '\0';
            cJSON_ArrayForEach(b, content)
            {
                const cJSON *type = cJSON_GetObjectItemCaseSensitive(b, "type");
                const cJSON *text = cJSON_GetObjectItemCaseSensitive(b, "text");
                if (cJSON_IsString(type) && strcmp(type->valuestring, "text") == 0 &&
                    cJSON_IsString(text)) {
                    strcat(reply->text, text->valuestring);
                }
            }
        }
    }
    cJSON_Delete(root);
    return (reply->call_count > 0 || reply->text != NULL) ? 0 : -1;
}
