/*
 * Host runner for NeuroClaw-ESP's message and send_file tools (nc_send.c):
 * what the model may send and to whom, which workspace files, the filename the
 * owner sees, and the multipart framing. Exit 0 and `NC_SEND OK checks=<n>`
 * only when every check passed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nc_send.h"

static unsigned s_checks;

static int fail(const char *what, int line)
{
    fprintf(stderr, "FAIL line %d: %s\n", line, what);
    return 1;
}

#define CHECK(cond)                                \
    do {                                           \
        s_checks++;                                \
        if (!(cond)) return fail(#cond, __LINE__); \
    } while (0)

#define OWNER 123456789

/* nc_message_args on a JSON text; 0 or -1, `err` filled on -1. */
static int message(const char *json, nc_message *out, char *err, size_t cap)
{
    cJSON *args = cJSON_Parse(json);
    const int r = nc_message_args(args, OWNER, out, err, cap);
    cJSON_Delete(args);
    return r;
}

static int check_message(void)
{
    nc_message m;
    char err[160];
    CHECK(message("{\"content\":\"Uống nước\"}", &m, err, sizeof err) == 0);
    CHECK(strcmp(m.content, "Uống nước") == 0 && m.reply_to == 0);
    nc_message_free(&m);
    CHECK(m.content == NULL);
    /* The owner, written either way, and Telegram, are fine. */
    CHECK(message("{\"content\":\"a\",\"chat_id\":\"123456789\",\"channel\":\"telegram\"}", &m, err,
                  sizeof err) == 0);
    nc_message_free(&m);
    CHECK(message("{\"content\":\"a\",\"chat_id\":123456789,\"reply_to_message_id\":\"42\"}", &m,
                  err, sizeof err) == 0);
    CHECK(m.reply_to == 42);
    nc_message_free(&m);
    CHECK(message("{\"content\":\"a\",\"chat_id\":\"\",\"channel\":\"\"}", &m, err, sizeof err) ==
          0);
    nc_message_free(&m);
    /* Anyone else, another channel, nothing to say: refused, nothing kept. */
    CHECK(message("{\"content\":\"a\",\"chat_id\":\"555\"}", &m, err, sizeof err) == -1);
    CHECK(strstr(err, "owner only") != NULL && m.content == NULL);
    CHECK(message("{\"content\":\"a\",\"chat_id\":555}", &m, err, sizeof err) == -1);
    CHECK(message("{\"content\":\"a\",\"chat_id\":\"123456789x\"}", &m, err, sizeof err) == -1);
    CHECK(message("{\"content\":\"a\",\"chat_id\":-123456789}", &m, err, sizeof err) == -1);
    CHECK(message("{\"content\":\"a\",\"channel\":\"discord\"}", &m, err, sizeof err) == -1);
    CHECK(strstr(err, "telegram") != NULL);
    CHECK(message("{\"content\":\"\"}", &m, err, sizeof err) == -1);
    CHECK(message("{}", &m, err, sizeof err) == -1);
    CHECK(strncmp(err, "error: ", 7) == 0);
    CHECK(message("{\"content\":\"a\",\"reply_to_message_id\":\"x\"}", &m, err, sizeof err) == -1);
    CHECK(message("{\"content\":\"a\",\"reply_to_message_id\":0}", &m, err, sizeof err) == -1);
    CHECK(nc_message_args(NULL, OWNER, &m, err, sizeof err) == -1);
    /* Telegram's limit is in characters: 4096 two-byte ones pass, one more does not. */
    char *json = malloc(2 * 4097 + 32);
    CHECK(json != NULL);
    strcpy(json, "{\"content\":\"");
    for (int i = 0; i < 4096; i++) strcat(json, "ă");
    strcat(json, "\"}");
    CHECK(message(json, &m, err, sizeof err) == 0);
    nc_message_free(&m);
    strcpy(json + strlen(json) - 2, "ă\"}");
    CHECK(message(json, &m, err, sizeof err) == -1);
    CHECK(strstr(err, "4096") != NULL);
    free(json);

    char *body = nc_message_body(OWNER, "xin chào", 0);
    CHECK(body != NULL && strstr(body, "\"chat_id\":123456789") != NULL);
    CHECK(strstr(body, "reply_parameters") == NULL);
    free(body);
    body = nc_message_body(OWNER, "xin chào", 42);
    cJSON *parsed = cJSON_Parse(body);
    const cJSON *reply = cJSON_GetObjectItem(parsed, "reply_parameters");
    CHECK(cJSON_GetObjectItem(reply, "message_id")->valueint == 42);
    CHECK(strcmp(cJSON_GetObjectItem(parsed, "text")->valuestring, "xin chào") == 0);
    cJSON_Delete(parsed);
    free(body);
    return 0;
}

static int send_file(const char *json, nc_file *out, char *err, size_t cap)
{
    cJSON *args = cJSON_Parse(json);
    const int r = nc_send_file_args("/ws", args, out, err, cap);
    cJSON_Delete(args);
    return r;
}

static int check_send_file(void)
{
    nc_file f;
    char err[160];
    CHECK(send_file("{\"path\":\"memory/MEMORY.md\"}", &f, err, sizeof err) == 0);
    CHECK(strcmp(f.path, "/ws/memory/MEMORY.md") == 0);
    CHECK(strcmp(f.filename, "MEMORY.md") == 0 && !f.photo);
    CHECK(strcmp(nc_file_method(&f), "sendDocument") == 0);
    /* Reading the brain's own files is allowed, as read_file; writing is not the point. */
    CHECK(send_file("{\"path\":\"sessions/summary.md\"}", &f, err, sizeof err) == 0);
    CHECK(send_file("{\"path\":\"photos/cua.JPG\",\"filename\":\"Cửa \\\"trước\\\".jpg\"}", &f,
                    err, sizeof err) == 0);
    CHECK(f.photo && strcmp(nc_file_method(&f), "sendPhoto") == 0);
    CHECK(strchr(f.filename, '"') == NULL && strchr(f.filename, '\r') == NULL);
    CHECK(strncmp(f.filename, "C__", 3) == 0); /* "ử" is two bytes, each replaced */
    CHECK(send_file("{\"path\":\"a.png\",\"filename\":\"x\\r\\nContent-Type: evil\"}", &f, err,
                    sizeof err) == 0);
    CHECK(strchr(f.filename, '\r') == NULL && strchr(f.filename, '\n') == NULL &&
          strchr(f.filename, ':') == NULL);
    CHECK(send_file("{\"path\":\"b.webp\"}", &f, err, sizeof err) == 0 && f.photo);
    CHECK(send_file("{\"path\":\"c.gif\"}", &f, err, sizeof err) == 0 && !f.photo);
    /* Outside the workspace, hidden, or nothing. */
    CHECK(send_file("{\"path\":\"../secret\"}", &f, err, sizeof err) == -1);
    CHECK(strstr(err, "workspace") != NULL);
    CHECK(send_file("{\"path\":\"/etc/passwd\"}", &f, err, sizeof err) == -1);
    CHECK(send_file("{\"path\":\".env\"}", &f, err, sizeof err) == -1);
    CHECK(send_file("{\"path\":\"\"}", &f, err, sizeof err) == -1);
    CHECK(send_file("{}", &f, err, sizeof err) == -1);
    return 0;
}

static int check_multipart(void)
{
    nc_file f;
    char err[160];
    CHECK(send_file("{\"path\":\"notes/a.md\"}", &f, err, sizeof err) == 0);
    char head[512];
    const int n = nc_multipart_head(head, sizeof head, "ncb0undary", OWNER, &f);
    CHECK(n > 0 && (size_t)n == strlen(head));
    CHECK(strncmp(head, "--ncb0undary\r\n", 14) == 0);
    CHECK(strstr(head, "name=\"chat_id\"\r\n\r\n123456789\r\n--ncb0undary\r\n") != NULL);
    CHECK(strstr(head, "name=\"document\"; filename=\"a.md\"\r\n") != NULL);
    CHECK(strstr(head, "Content-Type: application/octet-stream\r\n\r\n") != NULL);
    CHECK(strcmp(head + n - 4, "\r\n\r\n") == 0);
    CHECK(send_file("{\"path\":\"p.png\"}", &f, err, sizeof err) == 0);
    CHECK(nc_multipart_head(head, sizeof head, "b", OWNER, &f) > 0);
    CHECK(strstr(head, "name=\"photo\"; filename=\"p.png\"\r\nContent-Type: image/png") != NULL);
    char tail[64];
    CHECK(nc_multipart_tail(tail, sizeof tail, "ncb0undary") == 18);
    CHECK(strcmp(tail, "\r\n--ncb0undary--\r\n") == 0);
    CHECK(nc_multipart_head(head, 16, "b", OWNER, &f) == -1);
    CHECK(nc_multipart_tail(tail, 4, "b") == -1);
    return 0;
}

int main(void)
{
    if (check_message() != 0) return 1;
    if (check_send_file() != 0) return 1;
    if (check_multipart() != 0) return 1;
    printf("NC_SEND OK checks=%u\n", s_checks);
    return 0;
}
