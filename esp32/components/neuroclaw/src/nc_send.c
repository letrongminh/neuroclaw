/*
 * NeuroClaw-ESP's message and send_file tools, the parts without ESP-IDF state
 * (nc_send.h).
 */
#define _POSIX_C_SOURCE 200809L
#include "nc_send.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "nc_workspace.h"

static int refuse(char *err, size_t cap, const char *why)
{
    if (err != NULL && cap > 0) snprintf(err, cap, "error: %s", why);
    return -1;
}

/* UTF-8 characters in `text` (continuation bytes not counted). */
static size_t characters(const char *text)
{
    size_t n = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p != '\0'; p++) {
        if ((*p & 0xC0) != 0x80) n++;
    }
    return n;
}

/* A chat id given as a number or as digits; false for anything else. */
static bool chat_id_of(const cJSON *item, int64_t *out)
{
    if (cJSON_IsNumber(item)) {
        *out = (int64_t)item->valuedouble;
        return (double)*out == item->valuedouble;
    }
    if (!cJSON_IsString(item) || item->valuestring[0] == '\0') return false;
    const char *p = item->valuestring;
    if (*p == '-') p++;
    if (*p == '\0') return false;
    for (const char *c = p; *c != '\0'; c++) {
        if (*c < '0' || *c > '9') return false;
    }
    *out = strtoll(item->valuestring, NULL, 10);
    return true;
}

int nc_message_args(const cJSON *args, int64_t owner_chat, nc_message *out, char *err, size_t cap)
{
    if (out == NULL) return refuse(err, cap, "no output");
    memset(out, 0, sizeof *out);
    const cJSON *content = cJSON_GetObjectItemCaseSensitive(args, "content");
    if (!cJSON_IsString(content) || content->valuestring[0] == '\0') {
        return refuse(err, cap, "content is required");
    }
    if (characters(content->valuestring) > NC_MESSAGE_MAX) {
        return refuse(err, cap, "content is over 4096 characters, Telegram's limit; split it");
    }
    const cJSON *channel = cJSON_GetObjectItemCaseSensitive(args, "channel");
    if (channel != NULL && !cJSON_IsNull(channel) &&
        !(cJSON_IsString(channel) && (channel->valuestring[0] == '\0' ||
                                      strcasecmp(channel->valuestring, "telegram") == 0))) {
        return refuse(err, cap, "this brain has one channel, telegram");
    }
    const cJSON *chat = cJSON_GetObjectItemCaseSensitive(args, "chat_id");
    if (chat != NULL && !cJSON_IsNull(chat) &&
        !(cJSON_IsString(chat) && chat->valuestring[0] == '\0')) {
        int64_t id = 0;
        if (!chat_id_of(chat, &id) || id != owner_chat) {
            return refuse(err, cap, "this brain sends messages to its owner only");
        }
    }
    const cJSON *reply = cJSON_GetObjectItemCaseSensitive(args, "reply_to_message_id");
    int64_t reply_to = 0;
    if (reply != NULL && !cJSON_IsNull(reply) &&
        !(cJSON_IsString(reply) && reply->valuestring[0] == '\0')) {
        if (!chat_id_of(reply, &reply_to) || reply_to <= 0) {
            return refuse(err, cap, "reply_to_message_id must be a message id");
        }
    }
    out->content = strdup(content->valuestring);
    if (out->content == NULL) return refuse(err, cap, "out of memory");
    out->reply_to = reply_to;
    return 0;
}

void nc_message_free(nc_message *message)
{
    if (message == NULL) return;
    free(message->content);
    message->content = NULL;
}

char *nc_message_body(int64_t chat_id, const char *text, int64_t reply_to)
{
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) return NULL;
    cJSON_AddNumberToObject(json, "chat_id", (double)chat_id);
    cJSON_AddStringToObject(json, "text", text ? text : "");
    if (reply_to > 0) {
        cJSON *reply = cJSON_AddObjectToObject(json, "reply_parameters");
        cJSON_AddNumberToObject(reply, "message_id", (double)reply_to);
        cJSON_AddBoolToObject(reply, "allow_sending_without_reply", true);
    }
    char *body = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    return body;
}

static bool ends_with(const char *name, const char *suffix)
{
    const size_t n = strlen(name), m = strlen(suffix);
    return n >= m && strcasecmp(name + n - m, suffix) == 0;
}

static const char *image_type(const char *name)
{
    if (ends_with(name, ".jpg") || ends_with(name, ".jpeg")) return "image/jpeg";
    if (ends_with(name, ".png")) return "image/png";
    if (ends_with(name, ".webp")) return "image/webp";
    return NULL;
}

int nc_send_file_args(const char *workspace, const cJSON *args, nc_file *out, char *err,
                      size_t cap)
{
    if (out == NULL) return refuse(err, cap, "no output");
    memset(out, 0, sizeof *out);
    const cJSON *path = cJSON_GetObjectItemCaseSensitive(args, "path");
    if (!cJSON_IsString(path) || path->valuestring[0] == '\0') {
        return refuse(err, cap, "path is required");
    }
    if (!nc_ws_path(workspace, path->valuestring, false, out->path, sizeof out->path)) {
        return refuse(err, cap, "path is not a workspace path (relative, no ..)");
    }
    const cJSON *name = cJSON_GetObjectItemCaseSensitive(args, "filename");
    const char *given = cJSON_IsString(name) && name->valuestring[0] != '\0' ? name->valuestring
                                                                              : NULL;
    if (given == NULL) {
        const char *slash = strrchr(path->valuestring, '/');
        given = slash ? slash + 1 : path->valuestring;
    }
    /* It goes into a multipart header: nothing there may end the header. */
    size_t n = 0;
    for (const char *c = given; *c != '\0' && n + 1 < sizeof out->filename; c++) {
        const bool ok = (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') ||
                        (*c >= '0' && *c <= '9') || *c == '.' || *c == '-' || *c == '_' ||
                        *c == ' ';
        out->filename[n++] = ok ? *c : '_';
    }
    out->filename[n] = '\0';
    if (n == 0) return refuse(err, cap, "path names no file");
    out->photo = image_type(out->path) != NULL;
    return 0;
}

const char *nc_file_method(const nc_file *file)
{
    return file != NULL && file->photo ? "sendPhoto" : "sendDocument";
}

int nc_multipart_head(char *out, size_t cap, const char *boundary, int64_t chat_id,
                      const nc_file *file)
{
    if (out == NULL || boundary == NULL || file == NULL) return -1;
    const char *type = file->photo ? image_type(file->path) : "application/octet-stream";
    const int n = snprintf(out, cap,
                           "--%s\r\n"
                           "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n"
                           "%" PRId64 "\r\n"
                           "--%s\r\n"
                           "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
                           "Content-Type: %s\r\n\r\n",
                           boundary, chat_id, boundary, file->photo ? "photo" : "document",
                           file->filename, type);
    return n < 0 || (size_t)n >= cap ? -1 : n;
}

int nc_multipart_tail(char *out, size_t cap, const char *boundary)
{
    if (out == NULL || boundary == NULL) return -1;
    const int n = snprintf(out, cap, "\r\n--%s--\r\n", boundary);
    return n < 0 || (size_t)n >= cap ? -1 : n;
}
