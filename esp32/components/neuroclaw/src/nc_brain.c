/*
 * NeuroClaw-ESP on ESP-IDF (neuroclaw.h): Telegram long polling, the LLM, and the
 * turn loop that sends every tool call to the gate. Decisions live in nc_core.c.
 * Telegram and chat-completions clients after zclaw main/telegram.c and
 * main/llm.c (https://github.com/tnm/zclaw, commit e3ad271, MIT, Copyright (c)
 * 2026 Ted Nyman).
 *
 * Lines at column 0 (`NC_*`) are for tests and the owner's serial log. None of
 * them carries the bot token (it is in Telegram's URL, which is never printed),
 * the LLM key, or what the owner wrote.
 */
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nc_core.h"
#include "neuroclaw.h"
#include "sdkconfig.h"

static const char *TAG = "neuroclaw";

static nc_config s_cfg;
static nc_gate s_gate;
static cJSON *s_history;

/* --- HTTP ---------------------------------------------------------------------------------- */

typedef struct {
    char *data;
    size_t len;
    bool overflow;
} nc_buf;

static esp_err_t on_http_event(esp_http_client_event_t *event)
{
    nc_buf *buf = event->user_data;
    if (event->event_id != HTTP_EVENT_ON_DATA || buf == NULL || buf->overflow) return ESP_OK;
    const size_t max = CONFIG_NEUROCLAW_HTTP_MAX;
    if (buf->len + (size_t)event->data_len + 1 > max) {
        buf->overflow = true;
        return ESP_OK;
    }
    char *grown = realloc(buf->data, buf->len + (size_t)event->data_len + 1);
    if (grown == NULL) {
        buf->overflow = true;
        return ESP_OK;
    }
    buf->data = grown;
    memcpy(buf->data + buf->len, event->data, (size_t)event->data_len);
    buf->len += (size_t)event->data_len;
    buf->data[buf->len] = '\0';
    return ESP_OK;
}

/* POST `body` (or GET when NULL). Returns the HTTP status, -1 on a transport
 * error or a reply over CONFIG_NEUROCLAW_HTTP_MAX; the reply is in `*out`
 * (malloc'd, NUL-terminated) on any status. */
static int http(const char *url, const char *bearer, const char *body, int timeout_ms, char **out)
{
    nc_buf buf = {0};
    *out = NULL;
    esp_http_client_config_t config = {
        .url = url,
        .method = body ? HTTP_METHOD_POST : HTTP_METHOD_GET,
        .timeout_ms = timeout_ms,
        .event_handler = on_http_event,
        .user_data = &buf,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = 2048,
        .buffer_size_tx = 2048,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) return -1;
    if (body != NULL) {
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, body, (int)strlen(body));
    }
    if (bearer != NULL && bearer[0] != '\0') {
        char header[320];
        snprintf(header, sizeof header, "Bearer %s", bearer);
        esp_http_client_set_header(client, "Authorization", header);
        memset(header, 0, sizeof header);
    }
    const esp_err_t err = esp_http_client_perform(client);
    const int status = err == ESP_OK ? esp_http_client_get_status_code(client) : -1;
    esp_http_client_cleanup(client);
    if (buf.overflow) {
        free(buf.data);
        return -1;
    }
    *out = buf.data;
    return status;
}

static void telegram_url(char *out, size_t cap, const char *method)
{
    snprintf(out, cap, "%s/bot%s/%s", s_cfg.telegram_api, s_cfg.telegram_token, method);
}

static void telegram_post(const char *method, const char *body)
{
    char url[256];
    telegram_url(url, sizeof url, method);
    char *reply = NULL;
    const int status = http(url, NULL, body, 15000, &reply);
    memset(url, 0, sizeof url);
    if (status != 200) printf("NC_TELEGRAM_ERROR method=%s http=%d\n", method, status);
    free(reply);
}

static void send_text(int64_t chat_id, const char *text, const char *confirmation_id)
{
    char *body = nc_telegram_message(chat_id, text, confirmation_id);
    if (body == NULL) return;
    telegram_post("sendMessage", body);
    cJSON_free(body);
    printf("NC_REPLY chars=%u%s\n", (unsigned)strlen(text),
           confirmation_id ? " buttons=approve,refuse" : "");
}

static void acknowledge(const char *callback_id)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "callback_query_id", callback_id);
    char *body = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (body != NULL) telegram_post("answerCallbackQuery", body);
    cJSON_free(body);
}

/* --- history ------------------------------------------------------------------------------- */

static void remember(const char *role, const char *content)
{
    cJSON *m = cJSON_CreateObject();
    cJSON_AddStringToObject(m, "role", role);
    cJSON_AddStringToObject(m, "content", content);
    cJSON_AddItemToArray(s_history, m);
}

static void remember_calls(const nc_llm_reply *reply)
{
    cJSON *m = cJSON_CreateObject();
    cJSON_AddStringToObject(m, "role", "assistant");
    cJSON_AddNullToObject(m, "content");
    cJSON *calls = cJSON_AddArrayToObject(m, "tool_calls");
    for (int i = 0; i < reply->call_count; i++) {
        cJSON *c = cJSON_CreateObject();
        cJSON_AddStringToObject(c, "id", reply->calls[i].id);
        cJSON_AddStringToObject(c, "type", "function");
        cJSON *fn = cJSON_AddObjectToObject(c, "function");
        cJSON_AddStringToObject(fn, "name", reply->calls[i].name);
        cJSON_AddStringToObject(fn, "arguments", reply->calls[i].arguments);
        cJSON_AddItemToArray(calls, c);
    }
    cJSON_AddItemToArray(s_history, m);
}

static void remember_result(const char *call_id, const char *content)
{
    cJSON *m = cJSON_CreateObject();
    cJSON_AddStringToObject(m, "role", "tool");
    cJSON_AddStringToObject(m, "tool_call_id", call_id);
    cJSON_AddStringToObject(m, "content", content);
    cJSON_AddItemToArray(s_history, m);
}

/* --- one turn ------------------------------------------------------------------------------ */

static void run_tool(int64_t chat_id, const nc_tool_call *call)
{
    nc_tool_result result;
    memset(&result, 0, sizeof result);
    s_gate.call(s_gate.ctx, call->name, call->arguments, &result);
    printf("NC_TOOL name=%s status=%s reason=%s\n", call->name, nc_status_name(result.status),
           result.reason[0] ? result.reason : "-");
    if (result.status == NC_BLOCK && result.asks_owner && result.confirmation_id[0] != '\0') {
        /* The buttons come from here, never from the model. */
        printf("NC_ASK id=%s\n", result.confirmation_id);
        send_text(chat_id,
                  result.confirmation_message[0] ? result.confirmation_message
                                                 : "Cổng hỏi chủ nhà: đồng ý không?",
                  result.confirmation_id);
    }
    char *content = nc_tool_content(&result);
    remember_result(call->id, content ? content : "{\"status\":\"REJECTED\"}");
    free(content);
}

static void run_turn(int64_t chat_id, const char *text)
{
    remember("user", text);
    const nc_tool *tools = NULL;
    const size_t tool_count = s_gate.tools(s_gate.ctx, &tools);
    for (int round = 0; round < CONFIG_NEUROCLAW_MAX_ROUNDS; round++) {
        nc_history_trim(s_history, CONFIG_NEUROCLAW_HISTORY);
        char *request = nc_llm_request(s_cfg.llm_model, s_history, tools, tool_count);
        if (request == NULL) break;
        char *body = NULL;
        const int status = http(s_cfg.llm_url, s_cfg.llm_key, request, 60000, &body);
        cJSON_free(request);
        nc_llm_reply reply;
        const int parsed = status == 200 ? nc_llm_parse(body, &reply) : -1;
        free(body);
        if (parsed != 0) {
            printf("NC_LLM_ERROR http=%d\n", status);
            send_text(chat_id, "Không gọi được LLM, thử lại sau nhé.", NULL);
            return;
        }
        if (reply.text != NULL) {
            remember("assistant", reply.text);
            send_text(chat_id, reply.text, NULL);
            nc_llm_reply_free(&reply);
            return;
        }
        remember_calls(&reply);
        for (int i = 0; i < reply.call_count; i++) run_tool(chat_id, &reply.calls[i]);
        nc_llm_reply_free(&reply);
    }
    send_text(chat_id, "Xin lỗi, tôi chưa làm xong yêu cầu này.", NULL);
}

static void on_button(const nc_event *event)
{
    acknowledge(event->callback_id);
    nc_tool_result result;
    memset(&result, 0, sizeof result);
    s_gate.answer(s_gate.ctx, event->confirmation_id, event->approve, &result);
    printf("NC_BUTTON id=%s approve=%d status=%s reason=%s\n", event->confirmation_id,
           event->approve ? 1 : 0, nc_status_name(result.status),
           result.reason[0] ? result.reason : "-");
    char text[320];
    if (!event->approve) {
        snprintf(text, sizeof text, "Đã từ chối, không làm gì.");
    } else if (result.status == NC_ALLOW) {
        snprintf(text, sizeof text, "Xong: cổng đã cho phép sau khi bạn đồng ý.");
    } else {
        snprintf(text, sizeof text, "Không làm: %s%s%s", result.reason,
                 result.message[0] ? " — " : "", result.message);
    }
    send_text(event->chat_id, text, NULL);
    /* So the model knows how it ended; it is a note, not a request. */
    char note[400];
    snprintf(note, sizeof note, "(The owner pressed %s for request %s; the home answered %s.)",
             event->approve ? "approve" : "refuse", event->confirmation_id,
             nc_status_name(result.status));
    remember("assistant", note);
}

/* --- the task ------------------------------------------------------------------------------ */

static void brain_task(void *arg)
{
    (void)arg;
    const nc_tool *tools = NULL;
    printf("NC_READY tools=%u\n", (unsigned)s_gate.tools(s_gate.ctx, &tools));
    fflush(stdout);
    int64_t offset = 0;
    nc_event *events = calloc(NC_MAX_EVENTS, sizeof *events);
    if (events == NULL) {
        ESP_LOGE(TAG, "out of memory");
        vTaskDelete(NULL);
        return;
    }
    while (true) {
        char url[320];
        char method[64];
        snprintf(method, sizeof method, "getUpdates?timeout=%d&offset=%" PRId64,
                 CONFIG_NEUROCLAW_POLL_S, offset);
        telegram_url(url, sizeof url, method);
        char *body = NULL;
        const int status = http(url, NULL, NULL, (CONFIG_NEUROCLAW_POLL_S + 10) * 1000, &body);
        memset(url, 0, sizeof url);
        const int n = status == 200 ? nc_telegram_parse(body, s_cfg.owner_id, events,
                                                         NC_MAX_EVENTS, &offset)
                                    : -1;
        free(body);
        if (n < 0) {
            printf("NC_TELEGRAM_ERROR method=getUpdates http=%d\n", status);
            fflush(stdout);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        for (int i = 0; i < n; i++) {
            const nc_event *e = &events[i];
            if (e->kind == NC_EVENT_IGNORED) {
                printf("NC_IGNORED from=%s\n", e->from[0] ? e->from : "unknown");
            } else if (e->kind == NC_EVENT_TEXT) {
                printf("NC_HEARD chars=%u\n", (unsigned)strlen(e->text));
                run_turn(e->chat_id, e->text);
            } else {
                on_button(e);
            }
            fflush(stdout);
        }
        memset(events, 0, NC_MAX_EVENTS * sizeof *events); /* owner text, gone */
    }
}

static char *keep(const char *text)
{
    return strdup(text ? text : "");
}

int neuroclaw_start(const nc_config *config, const nc_gate *gate)
{
    if (config == NULL || gate == NULL || gate->tools == NULL || gate->call == NULL ||
        gate->answer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    const char *missing = NULL;
    if (config->telegram_token == NULL || config->telegram_token[0] == '\0') {
        missing = "telegram.token";
    } else if (config->owner_id == NULL || config->owner_id[0] == '\0') {
        missing = "telegram.owner"; /* no owner: nobody would be heard, or everybody */
    } else if (config->llm_url == NULL || config->llm_url[0] == '\0') {
        missing = "llm.url";
    } else if (config->llm_model == NULL || config->llm_model[0] == '\0') {
        missing = "llm.model";
    }
    if (missing != NULL) {
        printf("NC_OFF missing=%s\n", missing);
        return ESP_ERR_INVALID_ARG;
    }
    s_cfg.telegram_token = keep(config->telegram_token);
    s_cfg.owner_id = keep(config->owner_id);
    s_cfg.telegram_api =
        keep(config->telegram_api && config->telegram_api[0] ? config->telegram_api
                                                             : "https://api.telegram.org");
    s_cfg.llm_url = keep(config->llm_url);
    s_cfg.llm_key = keep(config->llm_key);
    s_cfg.llm_model = keep(config->llm_model);
    s_gate = *gate;
    s_history = cJSON_CreateArray();
    if (s_history == NULL) return ESP_ERR_NO_MEM;
    remember("system", config->system_prompt ? config->system_prompt : NC_SYSTEM_PROMPT);
    if (xTaskCreate(brain_task, "neuroclaw", CONFIG_NEUROCLAW_TASK_STACK, NULL, 4, NULL) !=
        pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
