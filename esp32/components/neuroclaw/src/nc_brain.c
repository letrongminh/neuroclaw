/*
 * NeuroClaw-ESP on ESP-IDF (neuroclaw.h): Telegram long polling, the LLM, the
 * workspace (memory, notes, the saved conversation), scheduled jobs and the
 * heartbeat, and the turn loop that sends every device tool call to the gate.
 * Decisions live in nc_core.c, nc_workspace.c, nc_cron.c, nc_web.c, nc_send.c and
 * nc_llm.c. Telegram and chat-completions clients after zclaw main/telegram.c and main/llm.c
 * (https://github.com/tnm/zclaw, commit e3ad271, MIT, Copyright (c) 2026 Ted
 * Nyman).
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
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nc_core.h"
#include "nc_cron.h"
#include "nc_llm.h"
#include "nc_send.h"
#include "nc_web.h"
#include "nc_utf8.h"
#include "nc_workspace.h"
#include "neuroclaw.h"
#include "sdkconfig.h"

static const char *TAG = "neuroclaw";

static nc_config s_cfg;
static nc_gate s_gate;
static cJSON *s_history; /* [0] is the system prompt, rebuilt before every LLM call */
static cJSON *s_jobs;
static nc_tool *s_tools;
static size_t s_tool_count;
static int64_t s_owner_chat;
static const char *s_base_prompt;
static int64_t s_next_heartbeat_us;
static TaskHandle_t s_task;
static bool (*s_wifi_rssi)(int *dbm);

/* The brain's state (history, jobs, files) is the brain task's; the local
 * administration API (neuroclaw_memory_read...) takes this lock, which the
 * brain holds while it handles anything, never while it waits on Telegram. */
static SemaphoreHandle_t s_lock;
static volatile bool s_heartbeat_now; /* neuroclaw_heartbeat_trigger */
static volatile bool s_cron_now;      /* neuroclaw_cron_start */
static char s_cron_job[24];           /* the job to run now; "" = whatever is due */

static nc_rate s_rate = {.per_hour = CONFIG_NEUROCLAW_LLM_PER_HOUR,
                         .per_day = CONFIG_NEUROCLAW_LLM_PER_DAY};
static char s_last_error[64]; /* for get_diagnostics: what last went wrong */

static void note_error(const char *what, const char *name, int status)
{
    snprintf(s_last_error, sizeof s_last_error, "%s %s http=%d", what, name, status);
}

/* The LLM URLs: [0] the configured one, [1] the fallback when there is one. */
typedef struct {
    const char *name; /* for the serial log */
    const char *url;
    const char *key;
    const char *model;
    int failures;     /* in a row, the kind that sends a call to the other URL */
    int64_t until_us; /* passed over until then */
    nc_llm_api api;
    int64_t calls, failures_total;
} nc_llm_target;
static nc_llm_target s_llm[2];
static size_t s_llm_count;

/* This turn: the model sent the owner a message itself (then the final reply
 * is not sent too, as the Go NeuroClaw), and the last such message. */
static bool s_turn_messaged;
static char *s_turn_message;
/* This turn hit an LLM call limit; what the owner is told. */
static bool s_turn_limited;
static char s_limit_text[160];

/* --- HTTP ---------------------------------------------------------------------------------- */

typedef struct {
    char *data;
    size_t len;
    bool overflow;
    char date[40];      /* the Date header */
    char location[256]; /* the Location header of a redirect */
} nc_buf;

static esp_err_t on_http_event(esp_http_client_event_t *event)
{
    nc_buf *buf = event->user_data;
    if (buf == NULL) return ESP_OK;
    if (event->event_id == HTTP_EVENT_ON_HEADER && event->header_key != NULL &&
        strcasecmp(event->header_key, "Date") == 0 && event->header_value != NULL) {
        snprintf(buf->date, sizeof buf->date, "%s", event->header_value);
        return ESP_OK;
    }
    if (event->event_id == HTTP_EVENT_ON_HEADER && event->header_key != NULL &&
        strcasecmp(event->header_key, "Location") == 0 && event->header_value != NULL) {
        snprintf(buf->location, sizeof buf->location, "%s", event->header_value);
        return ESP_OK;
    }
    if (event->event_id != HTTP_EVENT_ON_DATA || buf->overflow) return ESP_OK;
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
 * (malloc'd, NUL-terminated) on any status. `date` (may be NULL) gets the
 * Date header. */
typedef struct {
    const char *header_name; /* one more request header, or NULL */
    const char *header_value;
    bool no_redirect;   /* web_fetch follows redirects itself, checking each */
    char *location;     /* out: a redirect's Location */
    size_t location_cap;
    const char *header2_name; /* and one more, or NULL */
    const char *header2_value;
} nc_http_extra;

static int http_ex(const char *url, const char *bearer, const char *body, int timeout_ms,
                   char **out, char *date, size_t date_cap, nc_http_extra *extra)
{
    nc_buf buf = {0};
    *out = NULL;
    esp_http_client_config_t config = {
        .disable_auto_redirect = extra != NULL && extra->no_redirect,
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
    if (extra != NULL && extra->header_name != NULL && extra->header_value != NULL) {
        esp_http_client_set_header(client, extra->header_name, extra->header_value);
    }
    if (extra != NULL && extra->header2_name != NULL && extra->header2_value != NULL) {
        esp_http_client_set_header(client, extra->header2_name, extra->header2_value);
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
    if (date != NULL && date_cap > 0) snprintf(date, date_cap, "%s", buf.date);
    if (extra != NULL && extra->location != NULL && extra->location_cap > 0) {
        snprintf(extra->location, extra->location_cap, "%s", buf.location);
    }
    if (buf.overflow) {
        free(buf.data);
        return -1;
    }
    *out = buf.data;
    return status;
}

static int http(const char *url, const char *bearer, const char *body, int timeout_ms, char **out,
                char *date, size_t date_cap)
{
    return http_ex(url, bearer, body, timeout_ms, out, date, date_cap, NULL);
}

static void telegram_url(char *out, size_t cap, const char *method)
{
    snprintf(out, cap, "%s/bot%s/%s", s_cfg.telegram_api, s_cfg.telegram_token, method);
}

static int telegram_post(const char *method, const char *body)
{
    char url[256];
    telegram_url(url, sizeof url, method);
    char *reply = NULL;
    const int status = http(url, NULL, body, 15000, &reply, NULL, 0);
    memset(url, 0, sizeof url);
    if (status != 200) {
        printf("NC_TELEGRAM_ERROR method=%s http=%d\n", method, status);
        note_error("telegram", method, status);
    }
    free(reply);
    return status;
}

/* One file to the owner, multipart/form-data streamed from flash in 1 KB
 * pieces: the file's size is not bounded by RAM. The HTTP status, -1 on a
 * transport or file error. */
static int telegram_upload(const nc_file *file, long size)
{
    char boundary[40];
    snprintf(boundary, sizeof boundary, "nc%08" PRIx32 "%08" PRIx32, esp_random(), esp_random());
    char *head = malloc(512);
    char *chunk = malloc(1024);
    char tail[64];
    const int head_len = head ? nc_multipart_head(head, 512, boundary, s_owner_chat, file) : -1;
    const int tail_len = nc_multipart_tail(tail, sizeof tail, boundary);
    FILE *in = fopen(file->path, "rb");
    int status = -1;
    if (head_len > 0 && tail_len > 0 && chunk != NULL && in != NULL) {
        char url[256];
        telegram_url(url, sizeof url, nc_file_method(file));
        esp_http_client_config_t config = {
            .url = url,
            .method = HTTP_METHOD_POST,
            .timeout_ms = 60000,
            .crt_bundle_attach = esp_crt_bundle_attach,
            .buffer_size = 2048,
            .buffer_size_tx = 2048,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        memset(url, 0, sizeof url);
        char type[80];
        snprintf(type, sizeof type, "multipart/form-data; boundary=%s", boundary);
        bool ok = client != NULL && esp_http_client_set_header(client, "Content-Type", type) ==
                                        ESP_OK;
        ok = ok && esp_http_client_open(client, head_len + (int)size + tail_len) == ESP_OK;
        ok = ok && esp_http_client_write(client, head, head_len) == head_len;
        long left = size;
        while (ok && left > 0) {
            const size_t n = fread(chunk, 1, left < 1024 ? (size_t)left : 1024u, in);
            ok = n > 0 && esp_http_client_write(client, chunk, (int)n) == (int)n;
            left -= (long)n;
        }
        ok = ok && esp_http_client_write(client, tail, tail_len) == tail_len;
        if (ok && esp_http_client_fetch_headers(client) >= 0) {
            status = esp_http_client_get_status_code(client);
            esp_http_client_flush_response(client, NULL);
        }
        if (client != NULL) {
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
        }
    }
    if (in != NULL) fclose(in);
    free(head);
    free(chunk);
    return status;
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

/* --- the clock ----------------------------------------------------------------------------- */

static int64_t now_s(void)
{
    return (int64_t)time(NULL);
}

static void now_text(char *out, size_t cap)
{
    const time_t t = time(NULL);
    if ((int64_t)t < NC_CLOCK_VALID) {
        out[0] = '\0';
        return;
    }
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(out, cap, "%Y-%m-%d %H:%M (%A)", &tm);
}

/* Telegram's reply came over TLS from api.telegram.org: its Date header is good
 * enough to start reminders when nothing else (SNTP) set the clock. */
static void clock_from(const char *date)
{
    if (now_s() >= NC_CLOCK_VALID) return;
    const int64_t t = nc_http_date(date);
    if (t < NC_CLOCK_VALID) return;
    const struct timeval tv = {.tv_sec = (time_t)t, .tv_usec = 0};
    settimeofday(&tv, NULL);
    printf("NC_CLOCK set_from=telegram\n");
}

/* --- workspace state ----------------------------------------------------------------------- */

static void save_session(void)
{
    if (nc_session_save(s_cfg.workspace, s_history) != 0) printf("NC_MEMORY_ERROR what=session\n");
}

static void save_jobs(void)
{
    char *body = cJSON_PrintUnformatted(s_jobs);
    char path[192];
    snprintf(path, sizeof path, "%s/cron/jobs.json", s_cfg.workspace);
    if (body == NULL || nc_ws_store(path, body, false) != 0) printf("NC_MEMORY_ERROR what=cron\n");
    cJSON_free(body);
}

/* The Telegram offset is saved before the updates are handled: after a crash or
 * a reboot an owner's message is acted on at most once, never twice. */
static void save_offset(int64_t offset)
{
    char path[192], text[24];
    snprintf(path, sizeof path, "%s/sessions/telegram.offset", s_cfg.workspace);
    snprintf(text, sizeof text, "%" PRId64, offset);
    if (nc_ws_store(path, text, false) != 0) printf("NC_MEMORY_ERROR what=offset\n");
}

static int64_t load_offset(void)
{
    char path[192];
    snprintf(path, sizeof path, "%s/sessions/telegram.offset", s_cfg.workspace);
    char *text = nc_ws_slurp(path, 24);
    const int64_t offset = text ? strtoll(text, NULL, 10) : 0;
    free(text);
    return offset > 0 ? offset : 0;
}

static void load_jobs(void)
{
    char path[192];
    snprintf(path, sizeof path, "%s/cron/jobs.json", s_cfg.workspace);
    char *body = nc_ws_slurp(path, 64u * 1024u);
    s_jobs = body ? cJSON_Parse(body) : NULL;
    free(body);
    if (!cJSON_IsArray(s_jobs)) {
        cJSON_Delete(s_jobs);
        s_jobs = cJSON_CreateArray();
    }
}

static void refresh_system_prompt(cJSON *history)
{
    char now[48];
    now_text(now, sizeof now);
    char *prompt = nc_system_prompt(s_cfg.workspace, s_base_prompt, now);
    cJSON *system = cJSON_CreateObject();
    cJSON_AddStringToObject(system, "role", "system");
    cJSON_AddStringToObject(system, "content", prompt ? prompt : s_base_prompt);
    free(prompt);
    if (cJSON_GetArraySize(history) > 0) {
        cJSON_ReplaceItemInArray(history, 0, system);
    } else {
        cJSON_AddItemToArray(history, system);
    }
}

/* --- history ------------------------------------------------------------------------------- */

static void remember(cJSON *history, const char *role, const char *content)
{
    cJSON *m = cJSON_CreateObject();
    cJSON_AddStringToObject(m, "role", role);
    cJSON_AddStringToObject(m, "content", content);
    cJSON_AddItemToArray(history, m);
}

static void remember_calls(cJSON *history, const nc_llm_reply *reply)
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
    cJSON_AddItemToArray(history, m);
}

static void remember_result(cJSON *history, const char *call_id, const char *content)
{
    cJSON *m = cJSON_CreateObject();
    cJSON_AddStringToObject(m, "role", "tool");
    cJSON_AddStringToObject(m, "tool_call_id", call_id);
    cJSON_AddStringToObject(m, "content", content);
    cJSON_AddItemToArray(history, m);
}

/* --- tools --------------------------------------------------------------------------------- */

static bool is_builtin(const char *name)
{
    for (size_t i = 0; i < sizeof NC_WS_TOOLS / sizeof NC_WS_TOOLS[0]; i++) {
        if (strcmp(name, NC_WS_TOOLS[i]) == 0) return true;
    }
    return strcmp(name, "cron") == 0 || strcmp(name, "web_search") == 0 ||
           strcmp(name, "web_fetch") == 0 || strcmp(name, "message") == 0 ||
           strcmp(name, "send_file") == 0 || strcmp(name, "get_diagnostics") == 0;
}

static char *dup_text(const char *text)
{
    return strdup(text);
}

static char *web_search(const cJSON *args)
{
    const cJSON *q = cJSON_GetObjectItemCaseSensitive(args, "query");
    const cJSON *c = cJSON_GetObjectItemCaseSensitive(args, "count");
    const cJSON *r = cJSON_GetObjectItemCaseSensitive(args, "range");
    if (!cJSON_IsString(q) || q->valuestring[0] == '\0') return dup_text("error: query is required");
    int count = cJSON_IsNumber(c) ? c->valueint : 5;
    if (count < 1) count = 1;
    if (count > 10) count = 10;
    char *url = nc_search_url(s_cfg.search_provider, s_cfg.search_url, q->valuestring, count,
                              cJSON_IsString(r) ? r->valuestring : NULL);
    if (url == NULL) return dup_text("error: web search is not configured");
    const bool brave = strcmp(s_cfg.search_provider, "brave") == 0;
    nc_http_extra extra = {brave ? "X-Subscription-Token" : NULL, s_cfg.search_key, false, NULL, 0,
                           NULL, NULL};
    /* Tavily is a POST with the key in the body; the others a GET. */
    char *request = nc_search_body(s_cfg.search_provider, s_cfg.search_key, q->valuestring, count,
                                   cJSON_IsString(r) ? r->valuestring : NULL);
    char *body = NULL;
    const int status = http_ex(url, NULL, request, 20000, &body, NULL, 0, &extra);
    free(url);
    cJSON_free(request);
    char *out = status == 200 ? nc_search_text(s_cfg.search_provider, body, q->valuestring, count)
                              : NULL;
    free(body);
    if (out == NULL) {
        char text[64];
        snprintf(text, sizeof text, "error: the search provider answered HTTP %d", status);
        return dup_text(text);
    }
    return out;
}

/* Every address `host` resolves to is public, or the owner allowed the host. */
static bool reachable(const char *host)
{
    if (nc_host_allowed(host, s_cfg.fetch_allow)) return true;
    struct addrinfo hints = {0};
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *found = NULL;
    if (getaddrinfo(host, NULL, &hints, &found) != 0 || found == NULL) return false;
    bool ok = true;
    for (struct addrinfo *a = found; a != NULL && ok; a = a->ai_next) {
        char text[64] = "";
        if (a->ai_family == AF_INET) {
            inet_ntop(AF_INET, &((struct sockaddr_in *)a->ai_addr)->sin_addr, text, sizeof text);
        } else if (a->ai_family == AF_INET6) {
            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)a->ai_addr)->sin6_addr, text, sizeof text);
        }
        ok = !nc_ip_is_private(text);
    }
    freeaddrinfo(found);
    return ok;
}

static char *web_fetch(const cJSON *args)
{
    const cJSON *u = cJSON_GetObjectItemCaseSensitive(args, "url");
    if (!cJSON_IsString(u)) return dup_text("error: url is required");
    char url[256];
    snprintf(url, sizeof url, "%s", u->valuestring);
    for (int hop = 0; hop < 4; hop++) {
        bool https = false;
        char host[128];
        int port = 0;
        if (!nc_url_parts(url, &https, host, sizeof host, &port)) {
            return dup_text("error: only http(s) URLs without credentials can be fetched");
        }
        if (!reachable(host)) {
            printf("NC_FETCH refused=private host_len=%u\n", (unsigned)strlen(host));
            char text[320];
            const int w = snprintf(text, sizeof text,
                                   "error: %s is a private or local address (or does not "
                                   "resolve); web_fetch does not reach the home's own network",
                                   host);
            return dup_text(w > 0 ? text : "error: private address");
        }
        char location[256] = "";
        nc_http_extra extra = {NULL, NULL, true, location, sizeof location, NULL, NULL};
        char *body = NULL;
        const int status = http_ex(url, NULL, NULL, 20000, &body, NULL, 0, &extra);
        if (status >= 300 && status < 400 && location[0] != '\0') {
            free(body);
            if (strncmp(location, "http", 4) != 0) {
                return dup_text("error: a relative redirect was not followed");
            }
            snprintf(url, sizeof url, "%s", location);
            continue; /* checked again at the top */
        }
        if (status != 200 || body == NULL) {
            free(body);
            char text[80];
            snprintf(text, sizeof text, "error: HTTP %d (or the page is over the size limit)",
                     status);
            return dup_text(text);
        }
        char *page = nc_html_text(body, NC_FETCH_TEXT_MAX);
        free(body);
        printf("NC_FETCH ok chars=%u\n", page ? (unsigned)strlen(page) : 0u);
        return page ? page : dup_text("error: out of memory");
    }
    return dup_text("error: too many redirects");
}

/* As the Go message tool: the owner gets it now; the final reply of this turn
 * is then not sent too. */
static char *message_tool(const cJSON *args)
{
    nc_message message;
    char err[160];
    if (nc_message_args(args, s_owner_chat, &message, err, sizeof err) != 0) return dup_text(err);
    char *body = nc_message_body(s_owner_chat, message.content, message.reply_to);
    const int status = body ? telegram_post("sendMessage", body) : -1;
    cJSON_free(body);
    printf("NC_MESSAGE chars=%u http=%d\n", (unsigned)strlen(message.content), status);
    if (status != 200) {
        nc_message_free(&message);
        snprintf(err, sizeof err, "error: Telegram answered HTTP %d; the owner did not get it",
                 status);
        return dup_text(err);
    }
    s_turn_messaged = true;
    free(s_turn_message);
    s_turn_message = message.content; /* kept for the main conversation */
    snprintf(err, sizeof err, "Message sent to telegram:%" PRId64, s_owner_chat);
    return dup_text(err);
}

/* As the Go send_file: a workspace file to the owner, a photo when it is an
 * image (a document if Telegram refuses the photo). `*handled` when it went. */
static char *send_file_tool(const cJSON *args, bool *handled)
{
    char err[160];
    nc_file *file = malloc(sizeof *file);
    if (file == NULL) return dup_text("error: out of memory");
    if (nc_send_file_args(s_cfg.workspace, args, file, err, sizeof err) != 0) {
        free(file);
        return dup_text(err);
    }
    struct stat st;
    if (stat(file->path, &st) != 0 || !S_ISREG(st.st_mode)) {
        free(file);
        return dup_text("error: no such file in the workspace");
    }
    if (st.st_size > CONFIG_NEUROCLAW_SEND_FILE_MAX) {
        snprintf(err, sizeof err, "error: file too large: %ld bytes (max %d)", (long)st.st_size,
                 CONFIG_NEUROCLAW_SEND_FILE_MAX);
        free(file);
        return dup_text(err);
    }
    int status = telegram_upload(file, (long)st.st_size);
    if (status != 200 && file->photo) {
        file->photo = false;
        status = telegram_upload(file, (long)st.st_size);
    }
    printf("NC_SEND_FILE kind=%s bytes=%ld http=%d\n", file->photo ? "photo" : "document",
           (long)st.st_size, status);
    if (status == 200) {
        *handled = true;
        snprintf(err, sizeof err, "File \"%s\" sent to user", file->filename);
    } else {
        snprintf(err, sizeof err, "error: Telegram answered HTTP %d; the file was not sent",
                 status);
    }
    free(file);
    return dup_text(err);
}

static const char *api_name(nc_llm_api api)
{
    return api == NC_API_ANTHROPIC ? "anthropic" : "openai";
}

/* get_diagnostics, as zclaw's: the chip's health, read-only; no key, token or
 * owner text in it. Also neuroclaw_diagnostics() and the console's `diag`. */
static char *diagnostics_text(void)
{
    nc_diag d;
    memset(&d, 0, sizeof d);
    const int64_t now_us = esp_timer_get_time();
    d.uptime_s = now_us / 1000000;
    d.heap_free = esp_get_free_heap_size();
    d.heap_min_free = esp_get_minimum_free_heap_size();
    d.heap_largest_block = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    d.stack_free_min = s_task != NULL ? (int32_t)uxTaskGetStackHighWaterMark(s_task) : -1;
    int dbm = 0;
    if (s_wifi_rssi != NULL && s_wifi_rssi(&dbm)) {
        d.has_rssi = true;
        d.rssi_dbm = dbm;
    }
    d.llm_count = (int)s_llm_count;
    for (size_t i = 0; i < s_llm_count && i < 2; i++) {
        const nc_llm_target *t = &s_llm[i];
        d.llm[i].name = t->name;
        d.llm[i].api = api_name(t->api);
        d.llm[i].calls = t->calls;
        d.llm[i].failures_total = t->failures_total;
        d.llm[i].failures = t->failures;
        d.llm[i].cooldown_s = t->until_us > now_us ? (t->until_us - now_us + 999999) / 1000000 : 0;
    }
    d.rate = &s_rate;
    d.tools = (int)s_tool_count;
    d.jobs = cJSON_GetArraySize(s_jobs);
    d.history = s_history != NULL ? cJSON_GetArraySize(s_history) - 1 : 0;
    d.last_error = s_last_error;
    return nc_diagnostics_text(&d);
}

static const char CRON_SCHEMA[] =
    "{\"type\":\"object\",\"properties\":{"
    "\"action\":{\"type\":\"string\",\"enum\":[\"add\",\"list\",\"get\",\"update\",\"remove\","
    "\"enable\",\"disable\"]},"
    "\"name\":{\"type\":\"string\"},"
    "\"message\":{\"type\":\"string\",\"description\":\"What you will be told when it runs\"},"
    "\"at_seconds\":{\"type\":\"integer\",\"description\":\"One-time: seconds from now\"},"
    "\"every_seconds\":{\"type\":\"integer\",\"description\":\"Recurring interval, at least 60\"},"
    "\"cron_expr\":{\"type\":\"string\",\"description\":\"Five-field cron, local time, e.g. "
    "'0 9 * * *'\"},"
    "\"job_id\":{\"type\":\"string\"}},\"required\":[\"action\"]}";

/* The workspace tools, cron, then the gate's (a gate tool named like one of
 * ours is left out: the model must never mistake one for the other). */
static void build_tools(void)
{
    const nc_tool *gate_tools = NULL;
    const size_t gate_count = s_gate.tools(s_gate.ctx, &gate_tools);
    const size_t ws_count = sizeof NC_WS_TOOLS / sizeof NC_WS_TOOLS[0];
    s_tools = calloc(ws_count + 6 + gate_count, sizeof *s_tools);
    if (s_tools == NULL) return;
    for (size_t i = 0; i < ws_count; i++) {
        s_tools[s_tool_count++] = (nc_tool){NC_WS_TOOLS[i], nc_ws_tool_description(NC_WS_TOOLS[i]),
                                            nc_ws_tool_schema(NC_WS_TOOLS[i])};
    }
    s_tools[s_tool_count++] = (nc_tool){"cron",
                                        "Schedule reminders and recurring tasks; when one runs "
                                        "you get its message and your reply goes to the owner.",
                                        CRON_SCHEMA};
    if (s_cfg.search_provider[0] != '\0') {
        s_tools[s_tool_count++] = (nc_tool){
            "web_search",
            "Search the web for current information. Returns titles, URLs and snippets.",
            "{\"type\":\"object\",\"properties\":{\"query\":{\"type\":\"string\"},"
            "\"count\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":10},"
            "\"range\":{\"type\":\"string\",\"enum\":[\"d\",\"w\",\"m\",\"y\"]}},"
            "\"required\":[\"query\"]}"};
    }
    s_tools[s_tool_count++] = (nc_tool){
        "web_fetch",
        "Fetch a public web page as text. It cannot reach the home's own network.",
        "{\"type\":\"object\",\"properties\":{\"url\":{\"type\":\"string\"}},"
        "\"required\":[\"url\"]}"};
    s_tools[s_tool_count++] = (nc_tool){
        "message",
        "Send a text message to the owner on Telegram now. Your final reply is then not sent too.",
        "{\"type\":\"object\",\"properties\":{"
        "\"content\":{\"type\":\"string\",\"description\":\"The message text\"},"
        "\"channel\":{\"type\":\"string\",\"description\":\"Optional: telegram\"},"
        "\"chat_id\":{\"type\":\"string\",\"description\":\"Optional: the owner's chat ID\"},"
        "\"reply_to_message_id\":{\"type\":\"string\",\"description\":\"Optional: the "
        "message to reply to\"}},\"required\":[\"content\"]}"};
    s_tools[s_tool_count++] = (nc_tool){
        "send_file",
        "Send a file of the workspace (image, document, etc.) to the owner on Telegram.",
        "{\"type\":\"object\",\"properties\":{"
        "\"path\":{\"type\":\"string\",\"description\":\"Path relative to the workspace\"},"
        "\"filename\":{\"type\":\"string\",\"description\":\"Optional display filename. "
        "Defaults to the basename of path.\"}},\"required\":[\"path\"]}"};
    s_tools[s_tool_count++] = (nc_tool){
        "get_diagnostics",
        "The chip's health: uptime, free memory, the brain's stack, Wi-Fi signal, LLM calls, "
        "fallback and limits, the last error. Read-only.",
        "{\"type\":\"object\",\"properties\":{}}"};
    for (size_t i = 0; i < gate_count; i++) {
        if (is_builtin(gate_tools[i].name)) {
            printf("NC_TOOL_CONFLICT name=%s\n", gate_tools[i].name);
            continue;
        }
        s_tools[s_tool_count++] = gate_tools[i];
    }
}

/* One tool call; its result goes into `history`. True when the call itself
 * delivered what the owner asked for (send_file), as the Go ResponseHandled. */
static bool run_tool(cJSON *history, int64_t chat_id, const nc_tool_call *call)
{
    if (is_builtin(call->name)) {
        cJSON *args = cJSON_Parse(call->arguments);
        char *out = NULL;
        bool handled = false;
        if (strcmp(call->name, "message") == 0) {
            out = message_tool(args);
        } else if (strcmp(call->name, "send_file") == 0) {
            out = send_file_tool(args, &handled);
        } else if (strcmp(call->name, "cron") == 0) {
            bool changed = false;
            out = nc_cron_tool(s_jobs, args, now_s(), &changed);
            if (changed) save_jobs();
        } else if (strcmp(call->name, "web_search") == 0) {
            out = web_search(args);
        } else if (strcmp(call->name, "web_fetch") == 0) {
            out = web_fetch(args);
        } else if (strcmp(call->name, "get_diagnostics") == 0) {
            out = diagnostics_text();
        } else {
            out = nc_ws_tool(s_cfg.workspace, call->name, args);
        }
        cJSON_Delete(args);
        printf("NC_TOOL name=%s result=%s\n", call->name,
               out && strncmp(out, "error", 5) == 0 ? "error" : "ok");
        remember_result(history, call->id, out ? out : "error: out of memory");
        free(out);
        return handled;
    }
    nc_tool_result result;
    memset(&result, 0, sizeof result);
    s_gate.call(s_gate.ctx, call->name, call->arguments, &result);
    printf("NC_TOOL name=%s status=%s reason=%s\n", call->name, nc_status_name(result.status),
           result.reason[0] ? result.reason : "-");
    if (result.status == NC_BLOCK && result.asks_owner && result.confirmation_id[0] != '\0') {
        /* The buttons come from here, never from the model, and only to the owner. */
        printf("NC_ASK id=%s\n", result.confirmation_id);
        send_text(s_owner_chat,
                  result.confirmation_message[0] ? result.confirmation_message
                                                 : "Cổng hỏi chủ nhà: đồng ý không?",
                  result.confirmation_id);
    }
    (void)chat_id;
    char *content = nc_tool_content(&result);
    remember_result(history, call->id, content ? content : "{\"status\":\"REJECTED\"}");
    free(content);
    return false;
}

/* --- the LLM, with its fallback ------------------------------------------------------------ */

/* Returned by llm_call when a limit (nc_rate) refused the call. */
#define NC_LLM_LIMITED (-2)

/* One LLM call: the configured URL first, unless it failed recently and waits
 * out its cooldown; when one fails in a way the other may not
 * (nc_llm_failover), the other is tried. Each URL in its own format (openai or
 * anthropic). The last HTTP status, NC_LLM_LIMITED when a limit refused it
 * (s_limit_text then says so); `reply` holds the answer when it is 200 and
 * parsed (text or calls), and is empty otherwise. */
static int llm_call(const cJSON *messages, const nc_tool *tools, size_t tool_count,
                    nc_llm_reply *reply)
{
    memset(reply, 0, sizeof *reply);
    int64_t wait_s = 0;
    const char *full = nc_rate_take(&s_rate, esp_timer_get_time() / 1000000, &wait_s);
    if (full != NULL) {
        const bool day = strcmp(full, "day") == 0;
        const int limit = day ? s_rate.per_day : s_rate.per_hour;
        printf("NC_LLM_LIMIT window=%s limit=%d wait_s=%" PRId64 "\n", full, limit, wait_s);
        snprintf(s_limit_text, sizeof s_limit_text,
                 "Đã dùng hết %d lượt gọi LLM %s; thử lại sau khoảng %d phút nhé.", limit,
                 day ? "trong ngày" : "trong giờ này", (int)((wait_s + 59) / 60));
        snprintf(s_last_error, sizeof s_last_error, "llm limit window=%s", full);
        return NC_LLM_LIMITED;
    }
    const bool primary_cooling = s_llm_count == 2 && esp_timer_get_time() < s_llm[0].until_us;
    int status = -1;
    for (size_t k = 0; k < s_llm_count; k++) {
        nc_llm_target *t = &s_llm[primary_cooling ? 1 - k : k];
        const bool anthropic = t->api == NC_API_ANTHROPIC;
        char *request = anthropic ? nc_anthropic_request(t->model, CONFIG_NEUROCLAW_MAX_TOKENS,
                                                         messages, tools, tool_count)
                                  : nc_llm_request(t->model, messages, tools, tool_count);
        if (request == NULL) return -1;
        char *body = NULL;
        t->calls++;
        if (anthropic) {
            nc_http_extra extra = {t->key[0] ? "x-api-key" : NULL, t->key, false, NULL, 0,
                                   "anthropic-version", NC_ANTHROPIC_VERSION};
            status = http_ex(t->url, NULL, request, 60000, &body, NULL, 0, &extra);
        } else {
            status = http(t->url, t->key, request, 60000, &body, NULL, 0);
        }
        cJSON_free(request);
        const char *reason = nc_llm_failover(status);
        if (reason == NULL) {
            if (status == 200) {
                t->failures = 0;
                t->until_us = 0;
                const int parsed = anthropic ? nc_anthropic_parse(body, reply)
                                             : nc_llm_parse(body, reply);
                if (parsed != 0) note_error("llm", t->name, -3); /* a 200 with nothing in it */
            } else {
                t->failures_total++;
                note_error("llm", t->name, status);
            }
            free(body);
            return status;
        }
        free(body);
        t->failures_total++;
        note_error("llm", t->name, status);
        if (s_llm_count < 2) return status;
        t->failures++;
        const int64_t cool = nc_llm_cooldown_s(t->failures, strcmp(reason, "billing") == 0);
        t->until_us = esp_timer_get_time() + cool * 1000000;
        printf("NC_LLM_FALLBACK from=%s reason=%s http=%d cooldown_s=%" PRId64 "\n", t->name,
               reason, status, cool);
    }
    return status;
}

/* --- the summary --------------------------------------------------------------------------- */

/* Before the main conversation drops messages, the LLM folds them into
 * sessions/summary.md, which the system prompt carries as CONTEXT_SUMMARY (as
 * the Go NeuroClaw summarizes). If the LLM cannot be reached they are dropped. */
static void summarize_overflow(cJSON *history)
{
    const int drop = nc_history_overflow(history, CONFIG_NEUROCLAW_HISTORY);
    if (drop == 0) return;
    char path[192];
    snprintf(path, sizeof path, "%s/sessions/summary.md", s_cfg.workspace);
    char *previous = nc_ws_slurp(path, NC_BOOTSTRAP_MAX);
    char *input = nc_summary_input(previous, history, drop);
    free(previous);
    cJSON *messages = cJSON_CreateArray();
    remember(messages, "system", NC_SUMMARY_INSTRUCTION);
    remember(messages, "user", input ? input : "");
    free(input);
    nc_llm_reply reply;
    const int status = llm_call(messages, NULL, 0, &reply);
    cJSON_Delete(messages);
    if (status == 200 && reply.text != NULL) {
        if (strlen(reply.text) >= NC_BOOTSTRAP_MAX) reply.text[NC_BOOTSTRAP_MAX - 1] = '\0';
        nc_utf8_trim(reply.text);
        const int err = nc_ws_store(path, reply.text, false);
        printf("NC_SUMMARY folded=%d result=%s\n", drop, err == 0 ? "ok" : "error");
    } else {
        printf("NC_SUMMARY folded=0 result=%s http=%d\n",
               status == NC_LLM_LIMITED ? "limited" : "llm_error", status);
    }
    nc_llm_reply_free(&reply);
}

/* --- one turn ------------------------------------------------------------------------------ */

/* As the Go NeuroClaw notes a turn its tools delivered. */
static const char HANDLED_SUMMARY[] = "Requested output delivered via tool attachment.";

/* The model's final answer for `text` in `history`, malloc'd; NULL when the LLM
 * could not be reached. `*quiet`: the owner already has it (a `message` this
 * turn, or every tool of the last round delivered, as send_file), so the
 * answer is not sent again. */
static char *run_turn(cJSON *history, int64_t chat_id, const char *text, bool *quiet)
{
    *quiet = false;
    s_turn_messaged = false;
    s_turn_limited = false;
    free(s_turn_message);
    s_turn_message = NULL;
    remember(history, "user", text);
    if (history == s_history) summarize_overflow(history);
    for (int round = 0; round < CONFIG_NEUROCLAW_MAX_ROUNDS; round++) {
        nc_history_trim(history, CONFIG_NEUROCLAW_HISTORY);
        refresh_system_prompt(history);
        nc_llm_reply reply;
        const int status = llm_call(history, s_tools, s_tool_count, &reply);
        if (status == NC_LLM_LIMITED) {
            /* Said plainly, as an answer: the owner's message stays in the history. */
            s_turn_limited = true;
            remember(history, "assistant", s_limit_text);
            return strdup(s_limit_text);
        }
        if (reply.text == NULL && reply.call_count == 0) {
            printf("NC_LLM_ERROR http=%d\n", status);
            return NULL;
        }
        if (reply.text != NULL) {
            remember(history, "assistant", reply.text);
            char *answer = reply.text;
            reply.text = NULL;
            nc_llm_reply_free(&reply);
            *quiet = s_turn_messaged;
            return answer;
        }
        remember_calls(history, &reply);
        bool all_handled = reply.call_count > 0;
        for (int i = 0; i < reply.call_count; i++) {
            if (!run_tool(history, chat_id, &reply.calls[i])) all_handled = false;
        }
        nc_llm_reply_free(&reply);
        if (all_handled) {
            remember(history, "assistant", HANDLED_SUMMARY);
            *quiet = true;
            return strdup(HANDLED_SUMMARY);
        }
    }
    return strdup("Xin lỗi, tôi chưa làm xong yêu cầu này.");
}

static void on_text(const nc_event *event)
{
    bool quiet = false;
    char *answer = run_turn(s_history, event->chat_id, event->text, &quiet);
    if (answer != NULL && quiet) {
        printf("NC_REPLY skipped=sent_by_tool\n");
    } else {
        send_text(event->chat_id, answer ? answer : "Không gọi được LLM, thử lại sau nhé.", NULL);
    }
    free(answer);
    save_session();
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
    remember(s_history, "assistant", note);
    save_session();
}

/* A scheduled job or a heartbeat runs in a session of its own, as in the Go
 * NeuroClaw; what it says to the owner is noted in the main conversation so a
 * follow-up ("turn it off") makes sense. `quiet` drops a bare HEARTBEAT_OK. */
static void run_background(const char *label, const char *message, bool quiet)
{
    cJSON *history = cJSON_CreateArray();
    refresh_system_prompt(history);
    bool delivered = false;
    char *answer = run_turn(history, s_owner_chat, message, &delivered);
    cJSON_Delete(history);
    if (answer == NULL) {
        printf("NC_%s result=llm_error\n", label);
        return;
    }
    if (s_turn_limited) {
        free(answer);
        printf("NC_%s result=limited\n", label);
        if (quiet) return; /* a heartbeat waits for the next one */
        /* A reminder still reaches the owner, as it was written, with the reason. */
        const char *prefix = "[Scheduled task] ";
        const char *what = strncmp(message, prefix, strlen(prefix)) == 0 ? message + strlen(prefix)
                                                                         : message;
        const size_t cap = strlen(what) + sizeof s_limit_text + 16;
        char *text = malloc(cap);
        if (text == NULL) return;
        snprintf(text, cap, "⏰ %s\n(%s)", what, s_limit_text);
        send_text(s_owner_chat, text, NULL);
        remember(s_history, "assistant", text);
        free(text);
        save_session();
        return;
    }
    if (delivered) {
        /* The model sent it itself: what the owner got goes into the main
         * conversation, so a follow-up makes sense. */
        printf("NC_%s result=sent_by_tool\n", label);
        remember(s_history, "assistant", s_turn_message ? s_turn_message : answer);
        free(answer);
        save_session();
        return;
    }
    if (quiet && strstr(answer, "HEARTBEAT_OK") != NULL) {
        printf("NC_%s result=ok\n", label);
        free(answer);
        return;
    }
    send_text(s_owner_chat, answer, NULL);
    printf("NC_%s result=sent\n", label);
    remember(s_history, "assistant", answer);
    free(answer);
    save_session();
}

static void run_due_jobs(void)
{
    bool changed = false;
    cJSON *due;
    while (now_s() >= NC_CLOCK_VALID && (due = nc_cron_take_due(s_jobs, now_s(), &changed))) {
        save_jobs();
        const cJSON *id = cJSON_GetObjectItem(due, "id");
        const cJSON *msg = cJSON_GetObjectItem(due, "message");
        printf("NC_CRON_RUN id=%s\n", cJSON_IsString(id) ? id->valuestring : "?");
        char text[1200];
        snprintf(text, sizeof text, "[Scheduled task] %s",
                 cJSON_IsString(msg) ? msg->valuestring : "");
        run_background("CRON", text, false);
        cJSON_Delete(due);
    }
}

/* A job run now by neuroclaw_cron_start(id), its schedule unchanged. */
static void run_job_now(const char *id)
{
    const cJSON *job = NULL;
    cJSON_ArrayForEach(job, s_jobs)
    {
        const cJSON *jid = cJSON_GetObjectItem(job, "id");
        if (cJSON_IsString(jid) && strcmp(jid->valuestring, id) == 0) break;
    }
    const cJSON *msg = cJSON_GetObjectItem(job, "message");
    if (job == NULL || !cJSON_IsString(msg)) {
        printf("NC_CRON_RUN id=%s result=unknown_job\n", id);
        return;
    }
    printf("NC_CRON_RUN id=%s now=1\n", id);
    char text[1200];
    snprintf(text, sizeof text, "[Scheduled task] %s", msg->valuestring);
    run_background("CRON", text, false);
}

static void run_heartbeat(void)
{
    const bool asked = s_heartbeat_now;
    s_heartbeat_now = false;
    if (CONFIG_NEUROCLAW_HEARTBEAT_MIN == 0 && !asked) return;
    const int64_t now_us = esp_timer_get_time();
    if (now_us < s_next_heartbeat_us && !asked) return;
    s_next_heartbeat_us = now_us + (int64_t)CONFIG_NEUROCLAW_HEARTBEAT_MIN * 60 * 1000000;
    char now[48];
    now_text(now, sizeof now);
    char *prompt = nc_heartbeat_prompt(s_cfg.workspace, now);
    if (prompt == NULL) { /* no HEARTBEAT.md, or nothing in it */
        if (asked) printf("NC_HEARTBEAT result=nothing_to_check\n");
        return;
    }
    run_background("HEARTBEAT", prompt, true);
    free(prompt);
}

/* --- the task ------------------------------------------------------------------------------ */

static void brain_task(void *arg)
{
    (void)arg;
    printf("NC_READY tools=%u fallback=%u\n", (unsigned)s_tool_count,
           (unsigned)(s_llm_count - 1));
    fflush(stdout);
    int64_t offset = load_offset();
    nc_event *events = calloc(NC_MAX_EVENTS, sizeof *events);
    if (events == NULL) {
        ESP_LOGE(TAG, "out of memory");
        vTaskDelete(NULL);
        return;
    }
    while (true) {
        xSemaphoreTakeRecursive(s_lock, portMAX_DELAY);
        if (s_cron_now) {
            s_cron_now = false;
            if (s_cron_job[0] != '\0') run_job_now(s_cron_job);
            s_cron_job[0] = '\0';
        }
        run_due_jobs();
        run_heartbeat();
        xSemaphoreGiveRecursive(s_lock);
        fflush(stdout);
        int poll_s = CONFIG_NEUROCLAW_POLL_S;
        const int64_t next = now_s() >= NC_CLOCK_VALID ? nc_cron_seconds_to_next(s_jobs, now_s())
                                                       : -1;
        if (next >= 0 && next < poll_s) poll_s = next < 1 ? 1 : (int)next;

        char url[320];
        char method[64];
        snprintf(method, sizeof method, "getUpdates?timeout=%d&offset=%" PRId64, poll_s, offset);
        telegram_url(url, sizeof url, method);
        char *body = NULL;
        char date[40] = "";
        const int status = http(url, NULL, NULL, (poll_s + 10) * 1000, &body, date, sizeof date);
        memset(url, 0, sizeof url);
        if (status == 200) clock_from(date);
        const int64_t before = offset;
        const int n = status == 200 ? nc_telegram_parse(body, s_cfg.owner_id, events,
                                                         NC_MAX_EVENTS, &offset)
                                    : -1;
        free(body);
        xSemaphoreTakeRecursive(s_lock, portMAX_DELAY);
        if (n > 0 && offset != before) save_offset(offset);
        if (n < 0) {
            xSemaphoreGiveRecursive(s_lock);
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
                on_text(e);
            } else {
                on_button(e);
            }
            fflush(stdout);
        }
        xSemaphoreGiveRecursive(s_lock);
        memset(events, 0, NC_MAX_EVENTS * sizeof *events); /* owner text, gone */
        if (n > 0) printf("NC_STACK free_min=%u\n", (unsigned)uxTaskGetStackHighWaterMark(NULL));
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
    } else if (config->workspace == NULL || config->workspace[0] == '\0') {
        missing = "workspace"; /* no memory is not NeuroClaw */
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
    s_cfg.workspace = keep(config->workspace);
    s_cfg.search_provider = keep(config->search_provider);
    s_cfg.search_url = keep(config->search_url);
    s_cfg.search_key = keep(config->search_key);
    s_cfg.fetch_allow = keep(config->fetch_allow);
    s_llm[0] = (nc_llm_target){"primary", s_cfg.llm_url, s_cfg.llm_key, s_cfg.llm_model, 0, 0,
                               NC_API_OPENAI, 0, 0};
    s_llm_count = 1;
    if (config->llm_fallback_url != NULL && config->llm_fallback_url[0] != '\0') {
        s_cfg.llm_fallback_url = keep(config->llm_fallback_url);
        s_cfg.llm_fallback_key = keep(config->llm_fallback_key);
        s_cfg.llm_fallback_model =
            keep(config->llm_fallback_model && config->llm_fallback_model[0]
                     ? config->llm_fallback_model
                     : config->llm_model);
        s_llm[1] = (nc_llm_target){"fallback", s_cfg.llm_fallback_url, s_cfg.llm_fallback_key,
                                   s_cfg.llm_fallback_model, 0, 0, NC_API_OPENAI, 0, 0};
        s_llm_count = 2;
    }
    const char *search = s_cfg.search_provider;
    if (search[0] != '\0' && strcmp(search, "searxng") != 0 && strcmp(search, "brave") != 0 &&
        strcmp(search, "tavily") != 0) {
        printf("NC_OFF unknown_search_provider\n");
        return ESP_ERR_INVALID_ARG;
    }
    const int api = nc_llm_api_parse(config->llm_api);
    const int fallback_api = nc_llm_api_parse(config->llm_fallback_api);
    if (api < 0 || fallback_api < 0) {
        printf("NC_OFF unknown_llm_api\n");
        return ESP_ERR_INVALID_ARG;
    }
    s_llm[0].api = (nc_llm_api)api;
    s_llm[1].api = (nc_llm_api)fallback_api;
    s_wifi_rssi = config->wifi_rssi;
    s_base_prompt = keep(config->system_prompt ? config->system_prompt : NC_SYSTEM_PROMPT);
    s_owner_chat = strtoll(s_cfg.owner_id, NULL, 10); /* a private chat's id is the user's */
    s_gate = *gate;
    setenv("TZ", config->tz && config->tz[0] ? config->tz : "UTC0", 1);
    tzset();

    build_tools();
    load_jobs();
    s_history = nc_session_load(s_cfg.workspace);
    if (s_history != NULL) {
        printf("NC_SESSION restored=%d jobs=%d\n", cJSON_GetArraySize(s_history) - 1,
               cJSON_GetArraySize(s_jobs));
    } else {
        s_history = cJSON_CreateArray();
        printf("NC_SESSION new jobs=%d\n", cJSON_GetArraySize(s_jobs));
    }
    if (s_history == NULL || s_jobs == NULL || s_tools == NULL) return ESP_ERR_NO_MEM;
    refresh_system_prompt(s_history);
    s_next_heartbeat_us = (int64_t)CONFIG_NEUROCLAW_HEARTBEAT_MIN * 60 * 1000000;
    s_lock = xSemaphoreCreateRecursiveMutex();
    if (s_lock == NULL) return ESP_ERR_NO_MEM;
#if CONFIG_NEUROCLAW_TASK_CORE >= 0 && CONFIG_NEUROCLAW_TASK_CORE < CONFIG_FREERTOS_NUMBER_OF_CORES
    const BaseType_t core = CONFIG_NEUROCLAW_TASK_CORE;
#else
    const BaseType_t core = tskNO_AFFINITY;
#endif
    if (xTaskCreatePinnedToCore(brain_task, "neuroclaw", CONFIG_NEUROCLAW_TASK_STACK, NULL, 4,
                                &s_task, core) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

/* --- local administration ------------------------------------------------------------------ */

#define NC_ADMIN_WAIT pdMS_TO_TICKS(120000) /* the longest turn: rounds of 60 s calls */

static bool admin_lock(void)
{
    return s_lock != NULL && xSemaphoreTakeRecursive(s_lock, NC_ADMIN_WAIT) == pdTRUE;
}

static void memory_path(char *out, size_t cap)
{
    snprintf(out, cap, "%s/memory/MEMORY.md", s_cfg.workspace);
}

char *neuroclaw_memory_read(void)
{
    if (!admin_lock()) return NULL;
    char path[192];
    memory_path(path, sizeof path);
    char *text = nc_ws_slurp(path, 64u * 1024u);
    xSemaphoreGiveRecursive(s_lock);
    return text;
}

int neuroclaw_memory_write(const char *text)
{
    if (text == NULL) return ESP_ERR_INVALID_ARG;
    if (!admin_lock()) return ESP_ERR_TIMEOUT;
    char path[192];
    memory_path(path, sizeof path);
    const int err = nc_ws_store(path, text, false);
    xSemaphoreGiveRecursive(s_lock);
    return err == 0 ? ESP_OK : ESP_FAIL;
}

int neuroclaw_session_list(char *out, size_t cap)
{
    if (out == NULL || cap == 0) return ESP_ERR_INVALID_ARG;
    if (!admin_lock()) return ESP_ERR_TIMEOUT;
    char path[192];
    snprintf(path, sizeof path, "%s/sessions/summary.md", s_cfg.workspace);
    struct stat st;
    const long summary = stat(path, &st) == 0 ? (long)st.st_size : 0;
    snprintf(out, cap, "telegram=%d summary_bytes=%ld cron_jobs=%d",
             cJSON_GetArraySize(s_history) - 1, summary, cJSON_GetArraySize(s_jobs));
    xSemaphoreGiveRecursive(s_lock);
    return ESP_OK;
}

int neuroclaw_session_clear(void)
{
    if (!admin_lock()) return ESP_ERR_TIMEOUT;
    while (cJSON_GetArraySize(s_history) > 1) cJSON_DeleteItemFromArray(s_history, 1);
    char path[192];
    snprintf(path, sizeof path, "%s/sessions/summary.md", s_cfg.workspace);
    remove(path);
    refresh_system_prompt(s_history);
    save_session();
    xSemaphoreGiveRecursive(s_lock);
    printf("NC_SESSION cleared\n");
    return ESP_OK;
}

void neuroclaw_heartbeat_trigger(void)
{
    s_heartbeat_now = true;
}

void neuroclaw_cron_start(const char *job_id)
{
    if (!admin_lock()) return;
    snprintf(s_cron_job, sizeof s_cron_job, "%s", job_id != NULL ? job_id : "");
    s_cron_now = true;
    xSemaphoreGiveRecursive(s_lock);
}

char *neuroclaw_diagnostics(void)
{
    if (!admin_lock()) return NULL;
    char *text = diagnostics_text();
    xSemaphoreGiveRecursive(s_lock);
    return text;
}

void neuroclaw_set_llm_limits(int per_hour, int per_day)
{
    if (!admin_lock()) return;
    s_rate.per_hour = per_hour > 0 ? per_hour : 0;
    s_rate.per_day = per_day > 0 ? per_day : 0;
    xSemaphoreGiveRecursive(s_lock);
    printf("NC_LLM_LIMITS per_hour=%d per_day=%d\n", s_rate.per_hour, s_rate.per_day);
}
