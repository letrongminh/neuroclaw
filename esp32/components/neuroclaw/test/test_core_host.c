/*
 * Host runner for NeuroClaw-ESP's decisions (nc_core.c): who is heard, what the
 * model is told, what a button carries, the LLM request and reply. Built and
 * run by esp32/scripts/host_test.sh with AddressSanitizer and UBSan; exit 0 and
 * `NC_CORE OK checks=<n>` only when every check passed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nc_core.h"

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

#define OWNER "123456789"

static int check_telegram(void)
{
    nc_event events[NC_MAX_EVENTS];
    int64_t offset = 0;
    const char *body =
        "{\"ok\":true,\"result\":["
        /* the owner, privately */
        "{\"update_id\":10,\"message\":{\"from\":{\"id\":123456789},\"chat\":{\"id\":123456789,"
        "\"type\":\"private\"},\"text\":\"bật đèn\"}},"
        /* a stranger */
        "{\"update_id\":11,\"message\":{\"from\":{\"id\":555},\"chat\":{\"id\":555,"
        "\"type\":\"private\"},\"text\":\"mở cửa\"}},"
        /* the owner, in a group */
        "{\"update_id\":12,\"message\":{\"from\":{\"id\":123456789},\"chat\":{\"id\":-100,"
        "\"type\":\"group\"},\"text\":\"mở cửa\"}},"
        /* the owner, a sticker */
        "{\"update_id\":13,\"message\":{\"from\":{\"id\":123456789},\"chat\":{\"id\":123456789,"
        "\"type\":\"private\"}}},"
        /* the owner presses approve */
        "{\"update_id\":14,\"callback_query\":{\"id\":\"cb1\",\"from\":{\"id\":123456789},"
        "\"data\":\"nc:y:c-7\",\"message\":{\"chat\":{\"id\":123456789}}}},"
        /* a stranger presses approve on a forwarded button */
        "{\"update_id\":15,\"callback_query\":{\"id\":\"cb2\",\"from\":{\"id\":555},"
        "\"data\":\"nc:y:c-7\",\"message\":{\"chat\":{\"id\":555}}}},"
        /* the owner, a button that is not ours */
        "{\"update_id\":16,\"callback_query\":{\"id\":\"cb3\",\"from\":{\"id\":123456789},"
        "\"data\":\"other\",\"message\":{\"chat\":{\"id\":123456789}}}},"
        /* an edited message */
        "{\"update_id\":17,\"edited_message\":{\"from\":{\"id\":123456789},\"chat\":{\"id\":1,"
        "\"type\":\"private\"},\"text\":\"x\"}}"
        "]}";
    const int n = nc_telegram_parse(body, OWNER, events, NC_MAX_EVENTS, &offset);
    CHECK(n == 8);
    CHECK(offset == 18);
    CHECK(events[0].kind == NC_EVENT_TEXT);
    CHECK(strcmp(events[0].text, "bật đèn") == 0);
    CHECK(events[0].chat_id == 123456789);
    CHECK(events[1].kind == NC_EVENT_IGNORED);
    CHECK(strcmp(events[1].from, "555") == 0);
    CHECK(events[2].kind == NC_EVENT_IGNORED);
    CHECK(events[3].kind == NC_EVENT_IGNORED);
    CHECK(events[4].kind == NC_EVENT_BUTTON);
    CHECK(events[4].approve);
    CHECK(strcmp(events[4].confirmation_id, "c-7") == 0);
    CHECK(strcmp(events[4].callback_id, "cb1") == 0);
    CHECK(events[5].kind == NC_EVENT_IGNORED);
    CHECK(strcmp(events[5].callback_id, "cb2") == 0);
    CHECK(events[6].kind == NC_EVENT_IGNORED);
    CHECK(events[7].kind == NC_EVENT_IGNORED);

    /* No owner configured: nobody is heard. */
    offset = 0;
    CHECK(nc_telegram_parse(body, "", events, NC_MAX_EVENTS, &offset) == 8);
    for (int i = 0; i < 8; i++) CHECK(events[i].kind == NC_EVENT_IGNORED);
    CHECK(nc_telegram_parse(body, NULL, events, NC_MAX_EVENTS, &offset) == 8);
    CHECK(events[0].kind == NC_EVENT_IGNORED);

    /* More updates than room: the offset stops after the last one returned, so
     * the rest come again instead of being lost. */
    offset = 0;
    CHECK(nc_telegram_parse(body, OWNER, events, 2, &offset) == 2);
    CHECK(offset == 12);

    /* Not a Telegram reply. */
    CHECK(nc_telegram_parse("{\"ok\":false,\"description\":\"x\"}", OWNER, events, 8, &offset) ==
          -1);
    CHECK(nc_telegram_parse("not json", OWNER, events, 8, &offset) == -1);
    CHECK(nc_telegram_parse("{\"ok\":true,\"result\":[]}", OWNER, events, 8, &offset) == 0);
    CHECK(offset == 12);

    /* A user id that is a prefix of the owner's is not the owner. */
    offset = 0;
    const char *prefix = "{\"ok\":true,\"result\":[{\"update_id\":1,\"message\":{\"from\":{\"id\":"
                         "12345678},\"chat\":{\"id\":12345678,\"type\":\"private\"},\"text\":\"hi\"}}]}";
    CHECK(nc_telegram_parse(prefix, OWNER, events, 8, &offset) == 1);
    CHECK(events[0].kind == NC_EVENT_IGNORED);
    return 0;
}

static int check_buttons(void)
{
    char data[80];
    bool approve = false;
    char id[NC_ID_MAX];
    CHECK(nc_button_data(data, sizeof data, true, "c-7") > 0);
    CHECK(strcmp(data, "nc:y:c-7") == 0);
    CHECK(nc_button_parse(data, &approve, id, sizeof id) && approve && strcmp(id, "c-7") == 0);
    CHECK(nc_button_data(data, sizeof data, false, "abc_9") > 0);
    CHECK(nc_button_parse(data, &approve, id, sizeof id) && !approve);
    CHECK(!nc_button_parse("nc:x:c-7", &approve, id, sizeof id));
    CHECK(!nc_button_parse("nc:y:", &approve, id, sizeof id));
    CHECK(!nc_button_parse("nc:y:a b", &approve, id, sizeof id));
    CHECK(!nc_button_parse("nc:y:a\"b", &approve, id, sizeof id));
    CHECK(!nc_button_parse("xx:y:abc", &approve, id, sizeof id));
    CHECK(!nc_button_parse(NULL, &approve, id, sizeof id));
    char too_long[80];
    memset(too_long, 'a', sizeof too_long - 1);
    too_long[sizeof too_long - 1] = '\0';
    CHECK(nc_button_data(data, sizeof data, true, too_long) == -1);
    char small[4];
    CHECK(!nc_button_parse("nc:y:abcdef", &approve, small, sizeof small));

    char *msg = nc_telegram_message(42, "Mở cửa?", "c-7");
    CHECK(msg != NULL);
    cJSON *json = cJSON_Parse(msg);
    CHECK(json != NULL);
    const cJSON *rows = cJSON_GetObjectItem(cJSON_GetObjectItem(json, "reply_markup"),
                                            "inline_keyboard");
    const cJSON *row = cJSON_GetArrayItem(rows, 0);
    CHECK(cJSON_GetArraySize(row) == 2);
    CHECK(strcmp(cJSON_GetObjectItem(cJSON_GetArrayItem(row, 0), "callback_data")->valuestring,
                 "nc:y:c-7") == 0);
    CHECK(strcmp(cJSON_GetObjectItem(cJSON_GetArrayItem(row, 1), "callback_data")->valuestring,
                 "nc:n:c-7") == 0);
    CHECK(cJSON_GetObjectItem(json, "chat_id")->valuedouble == 42);
    cJSON_Delete(json);
    cJSON_free(msg);
    msg = nc_telegram_message(42, "xin chào", NULL);
    CHECK(msg != NULL && strstr(msg, "reply_markup") == NULL);
    cJSON_free(msg);
    return 0;
}

static nc_tool_result result(nc_status status)
{
    nc_tool_result r;
    memset(&r, 0, sizeof r);
    r.status = status;
    return r;
}

static int check_guidance(void)
{
    char out[1024];
    nc_tool_result r = result(NC_ALLOW);
    CHECK(nc_guidance(&r, out, sizeof out) == 0);
    CHECK(out[0] == '\0');

    r = result(NC_BLOCK);
    r.asks_owner = true;
    strcpy(r.reason, "awaiting_confirmation");
    strcpy(r.confirmation_id, "c-7");
    strcpy(r.confirmation_message, "Bộ não xin mở cửa \"ngay\"");
    r.expires_in_ms = 60000;
    CHECK(nc_guidance(&r, out, sizeof out) > 0);
    CHECK(strstr(out, "did NOT do this yet") != NULL);
    CHECK(strstr(out, "You cannot approve it") != NULL);
    CHECK(strstr(out, "within about 60 seconds") != NULL);
    CHECK(strstr(out, "Do not call the tool again") != NULL);
    CHECK(strstr(out, "\"Bộ não xin mở cửa \\\"ngay\\\"\"") != NULL);

    char *content = nc_tool_content(&r);
    CHECK(content != NULL);
    CHECK(strncmp(content, "[NeuroEdge] The home did NOT", 28) == 0);
    cJSON *json = cJSON_Parse(strchr(content, '\n') + 1);
    CHECK(json != NULL);
    CHECK(strcmp(cJSON_GetObjectItem(json, "status")->valuestring, "BLOCK") == 0);
    const cJSON *c = cJSON_GetObjectItem(json, "confirmation");
    CHECK(strcmp(cJSON_GetObjectItem(c, "id")->valuestring, "c-7") == 0);
    CHECK(strcmp(cJSON_GetObjectItem(c, "who")->valuestring, "owner") == 0);
    cJSON_Delete(json);
    free(content);

    r = result(NC_BLOCK);
    strcpy(r.reason, "criterion_failed");
    strcpy(r.failed_criterion, "daytime");
    strcpy(r.message, "Không mở cửa ban đêm.");
    CHECK(nc_guidance(&r, out, sizeof out) > 0);
    CHECK(strstr(out, "refused this: criterion_failed (daytime). Không mở cửa ban đêm.") != NULL);
    CHECK(strstr(out, "Do not retry") != NULL);
    CHECK(strstr(out, "approve") == NULL);

    r = result(NC_REJECTED);
    CHECK(nc_guidance(&r, out, sizeof out) > 0);
    CHECK(strstr(out, "nothing ran") != NULL);

    /* ALLOW content is the JSON alone. */
    r = result(NC_ALLOW);
    content = nc_tool_content(&r);
    CHECK(content != NULL && content[0] == '{');
    free(content);

    char small[16];
    r = result(NC_REJECTED);
    CHECK(nc_guidance(&r, small, sizeof small) == -1);
    CHECK(small[0] == '\0');
    CHECK(strcmp(nc_status_name(NC_ALLOW), "ALLOW") == 0);
    CHECK(strcmp(nc_status_name(NC_REJECTED), "REJECTED") == 0);
    return 0;
}

static int check_llm(void)
{
    cJSON *messages = cJSON_CreateArray();
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddStringToObject(sys, "role", "system");
    cJSON_AddStringToObject(sys, "content", NC_SYSTEM_PROMPT);
    cJSON_AddItemToArray(messages, sys);
    const nc_tool tools[] = {
        {"living_light_on", "Bật đèn", "{\"type\":\"object\",\"properties\":{}}"},
        {"front_door_unlock", "Mở cửa", "not json"},
    };
    char *body = nc_llm_request("m", messages, tools, 2);
    CHECK(body != NULL);
    cJSON *json = cJSON_Parse(body);
    CHECK(cJSON_GetArraySize(cJSON_GetObjectItem(json, "tools")) == 2);
    const cJSON *fn =
        cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(json, "tools"), 1), "function");
    CHECK(cJSON_IsObject(cJSON_GetObjectItem(fn, "parameters"))); /* bad schema → empty object */
    CHECK(strcmp(cJSON_GetObjectItem(json, "tool_choice")->valuestring, "auto") == 0);
    cJSON_Delete(json);
    cJSON_free(body);
    body = nc_llm_request("m", messages, NULL, 0);
    CHECK(body != NULL && strstr(body, "\"tools\"") == NULL);
    cJSON_free(body);

    nc_llm_reply reply;
    CHECK(nc_llm_parse("{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":\"Đã bật\"}}]}",
                       &reply) == 0);
    CHECK(reply.text != NULL && strcmp(reply.text, "Đã bật") == 0 && reply.call_count == 0);
    nc_llm_reply_free(&reply);

    CHECK(nc_llm_parse("{\"choices\":[{\"message\":{\"content\":null,\"tool_calls\":["
                       "{\"id\":\"a\",\"type\":\"function\",\"function\":{\"name\":\"living_light_on\","
                       "\"arguments\":\"{}\"}},"
                       "{\"id\":\"b\",\"function\":{\"name\":\"front_door_unlock\",\"arguments\":"
                       "\"{\\\"duration_s\\\":10}\"}}]}}]}",
                       &reply) == 0);
    CHECK(reply.text == NULL && reply.call_count == 2);
    CHECK(strcmp(reply.calls[0].name, "living_light_on") == 0);
    CHECK(strcmp(reply.calls[1].arguments, "{\"duration_s\":10}") == 0);
    nc_llm_reply_free(&reply);

    /* Text next to tool calls: the calls win, the text is not sent as if done. */
    CHECK(nc_llm_parse("{\"choices\":[{\"message\":{\"content\":\"Đã mở cửa!\",\"tool_calls\":["
                       "{\"id\":\"a\",\"function\":{\"name\":\"front_door_unlock\"}}]}}]}",
                       &reply) == 0);
    CHECK(reply.text == NULL && reply.call_count == 1);
    CHECK(strcmp(reply.calls[0].arguments, "{}") == 0);
    nc_llm_reply_free(&reply);

    CHECK(nc_llm_parse("{\"choices\":[{\"message\":{\"content\":\"\"}}]}", &reply) == -1);
    CHECK(nc_llm_parse("{\"error\":{\"message\":\"quota\"}}", &reply) == -1);
    CHECK(nc_llm_parse("garbage", &reply) == -1);
    CHECK(nc_llm_parse(NULL, &reply) == -1);

    /* Trimming keeps the system prompt and never starts on a tool result. */
    for (int i = 0; i < 6; i++) {
        cJSON *m = cJSON_CreateObject();
        cJSON_AddStringToObject(m, "role", i == 2 ? "tool" : "user");
        cJSON_AddStringToObject(m, "content", "x");
        cJSON_AddItemToArray(messages, m);
    }
    nc_history_trim(messages, 4); /* [sys, u0, u1, tool, u3, u4, u5] → [sys, u3, u4, u5] */
    CHECK(cJSON_GetArraySize(messages) == 4);
    CHECK(strcmp(cJSON_GetObjectItem(cJSON_GetArrayItem(messages, 0), "role")->valuestring,
                 "system") == 0);
    CHECK(strcmp(cJSON_GetObjectItem(cJSON_GetArrayItem(messages, 1), "role")->valuestring,
                 "user") == 0);

    /* What the summary would fold: as much as trimming would drop. */
    cJSON *h = cJSON_Parse("[{\"role\":\"system\",\"content\":\"s\"},"
                           "{\"role\":\"user\",\"content\":\"nhớ giúp: đèn vàng\"},"
                           "{\"role\":\"assistant\",\"content\":null,\"tool_calls\":[{\"id\":\"a\","
                           "\"function\":{\"name\":\"append_file\",\"arguments\":\"{}\"}}]},"
                           "{\"role\":\"tool\",\"tool_call_id\":\"a\",\"content\":\"ok\"},"
                           "{\"role\":\"assistant\",\"content\":\"Xong.\\nĐã nhớ\"},"
                           "{\"role\":\"user\",\"content\":\"bật đèn\"}]");
    CHECK(nc_history_overflow(h, 10) == 0);
    CHECK(nc_history_overflow(h, 3) == 3); /* user, assistant(call), and its tool result */
    CHECK(nc_history_overflow(h, 4) == 1);
    char *in = nc_summary_input("Chủ nhà tên Minh.", h, 4);
    CHECK(in != NULL);
    CHECK(strstr(in, "Previous summary:\nChủ nhà tên Minh.") != NULL);
    CHECK(strstr(in, "user: nhớ giúp: đèn vàng\n") != NULL);
    CHECK(strstr(in, "assistant: (called append_file)\n") != NULL);
    CHECK(strstr(in, "assistant: Xong. Đã nhớ\n") != NULL); /* one line per message */
    CHECK(strstr(in, "bật đèn") == NULL); /* not folded */
    free(in);
    in = nc_summary_input(NULL, h, 1);
    CHECK(in != NULL && strncmp(in, "Conversation:\n", 14) == 0);
    free(in);
    cJSON_Delete(h);
    cJSON_Delete(messages);
    return 0;
}

static int check_failover(void)
{
    /* As the Go classifier: these go to the other URL... */
    const int over[] = {-1, 401, 402, 403, 408, 429, 500, 502, 503, 521, 524, 529};
    for (size_t i = 0; i < sizeof over / sizeof over[0]; i++) CHECK(nc_llm_failover(over[i]) != NULL);
    CHECK(strcmp(nc_llm_failover(-1), "network") == 0);
    CHECK(strcmp(nc_llm_failover(429), "rate_limit") == 0);
    CHECK(strcmp(nc_llm_failover(402), "billing") == 0);
    CHECK(strcmp(nc_llm_failover(503), "transient") == 0);
    /* ...these would fail there too, or are not failures. */
    const int stay[] = {200, 400, 404, 413, 501, 504};
    for (size_t i = 0; i < sizeof stay / sizeof stay[0]; i++) CHECK(nc_llm_failover(stay[i]) == NULL);
    /* Cooldown: 1 min, 5, 25, then 1 h; billing 5 h, 10, 20, then 24 h. */
    CHECK(nc_llm_cooldown_s(0, false) == 60);
    CHECK(nc_llm_cooldown_s(1, false) == 60);
    CHECK(nc_llm_cooldown_s(2, false) == 300);
    CHECK(nc_llm_cooldown_s(3, false) == 1500);
    CHECK(nc_llm_cooldown_s(4, false) == 3600);
    CHECK(nc_llm_cooldown_s(40, false) == 3600);
    CHECK(nc_llm_cooldown_s(1, true) == 5 * 3600);
    CHECK(nc_llm_cooldown_s(2, true) == 10 * 3600);
    CHECK(nc_llm_cooldown_s(3, true) == 20 * 3600);
    CHECK(nc_llm_cooldown_s(4, true) == 24 * 3600);
    CHECK(nc_llm_cooldown_s(1000, true) == 24 * 3600);
    return 0;
}

int main(void)
{
    if (check_telegram() != 0) return 1;
    if (check_buttons() != 0) return 1;
    if (check_guidance() != 0) return 1;
    if (check_llm() != 0) return 1;
    if (check_failover() != 0) return 1;
    printf("NC_CORE OK checks=%u\n", s_checks);
    return 0;
}
