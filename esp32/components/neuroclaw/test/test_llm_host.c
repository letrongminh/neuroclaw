/*
 * Host runner for NeuroClaw-ESP's Anthropic Messages format (nc_llm.c): the
 * request built from the chat-completions conversation, and the reply read
 * back into calls or text. Built and run by esp32/scripts/host_test.sh with
 * AddressSanitizer and UBSan; exit 0 and `NC_LLM OK checks=<n>` only when every
 * check passed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nc_llm.h"

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

static const char *str(const cJSON *object, const char *key)
{
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(object, key);
    return cJSON_IsString(v) ? v->valuestring : "";
}

static int check_api_names(void)
{
    CHECK(nc_llm_api_parse(NULL) == NC_API_OPENAI);
    CHECK(nc_llm_api_parse("") == NC_API_OPENAI);
    CHECK(nc_llm_api_parse("openai") == NC_API_OPENAI);
    CHECK(nc_llm_api_parse("anthropic") == NC_API_ANTHROPIC);
    CHECK(nc_llm_api_parse("Anthropic") == -1);
    CHECK(nc_llm_api_parse("gemini") == -1);
    return 0;
}

static int check_request(void)
{
    /* A whole turn, as the brain keeps it: system, the owner, two tool calls, their
     * results, a note, the answer, and the owner again. */
    cJSON *messages = cJSON_Parse(
        "[{\"role\":\"system\",\"content\":\"You are NeuroClaw.\"},"
        "{\"role\":\"user\",\"content\":\"bật đèn và mở cửa\"},"
        "{\"role\":\"assistant\",\"content\":null,\"tool_calls\":["
        "{\"id\":\"call_0\",\"type\":\"function\",\"function\":{\"name\":\"living_light_on\","
        "\"arguments\":\"{}\"}},"
        "{\"id\":\"call:1/x\",\"type\":\"function\",\"function\":{\"name\":\"front_door_unlock\","
        "\"arguments\":\"{\\\"duration_s\\\":10}\"}},"
        "{\"id\":\"call_2\",\"type\":\"function\",\"function\":{\"name\":\"\",\"arguments\":\"{}\"}}]},"
        "{\"role\":\"tool\",\"tool_call_id\":\"call_0\",\"content\":\"{\\\"status\\\":\\\"ALLOW\\\"}\"},"
        "{\"role\":\"tool\",\"tool_call_id\":\"call:1/x\",\"content\":\"BLOCK\"},"
        "{\"role\":\"assistant\",\"content\":\"Đang chờ bạn duyệt.\"},"
        "{\"role\":\"assistant\",\"content\":\"(The owner pressed approve.)\"},"
        "{\"role\":\"user\",\"content\":\"\"},"
        "{\"role\":\"user\",\"content\":\"cảm ơn\"},"
        "{\"role\":\"system\",\"content\":\"Second system.\"}]");
    CHECK(messages != NULL);
    const nc_tool tools[] = {
        {"living_light_on", "Turn the light on.", "{\"type\":\"object\",\"properties\":{}}"},
        {"broken", NULL, "not json"},
    };
    char *body = nc_anthropic_request("claude-x", 512, messages, tools, 2);
    cJSON_Delete(messages);
    CHECK(body != NULL);
    cJSON *req = cJSON_Parse(body);
    free(body);
    CHECK(req != NULL);
    CHECK(strcmp(str(req, "model"), "claude-x") == 0);
    CHECK(cJSON_GetObjectItem(req, "max_tokens")->valueint == 512);
    CHECK(strcmp(str(req, "system"), "You are NeuroClaw.\n\nSecond system.") == 0);
    CHECK(cJSON_GetObjectItem(req, "tool_choice") == NULL);

    const cJSON *out = cJSON_GetObjectItem(req, "messages");
    CHECK(cJSON_GetArraySize(out) == 5); /* user, assistant, results, notes, user */
    const cJSON *m0 = cJSON_GetArrayItem(out, 0);
    CHECK(strcmp(str(m0, "role"), "user") == 0);
    const cJSON *b = cJSON_GetArrayItem(cJSON_GetObjectItem(m0, "content"), 0);
    CHECK(strcmp(str(b, "type"), "text") == 0 && strcmp(str(b, "text"), "bật đèn và mở cửa") == 0);

    const cJSON *m1 = cJSON_GetArrayItem(out, 1);
    CHECK(strcmp(str(m1, "role"), "assistant") == 0);
    const cJSON *uses = cJSON_GetObjectItem(m1, "content");
    CHECK(cJSON_GetArraySize(uses) == 2); /* the nameless call is left out */
    const cJSON *u0 = cJSON_GetArrayItem(uses, 0);
    CHECK(strcmp(str(u0, "type"), "tool_use") == 0 && strcmp(str(u0, "id"), "call_0") == 0);
    CHECK(strcmp(str(u0, "name"), "living_light_on") == 0);
    CHECK(cJSON_IsObject(cJSON_GetObjectItem(u0, "input")));
    const cJSON *u1 = cJSON_GetArrayItem(uses, 1);
    CHECK(strcmp(str(u1, "id"), "call_1_x") == 0);
    CHECK(cJSON_GetObjectItem(cJSON_GetObjectItem(u1, "input"), "duration_s")->valueint == 10);

    const cJSON *m2 = cJSON_GetArrayItem(out, 2);
    CHECK(strcmp(str(m2, "role"), "user") == 0);
    const cJSON *results = cJSON_GetObjectItem(m2, "content");
    CHECK(cJSON_GetArraySize(results) == 2); /* both results in one user message */
    const cJSON *r1 = cJSON_GetArrayItem(results, 1);
    CHECK(strcmp(str(r1, "type"), "tool_result") == 0);
    CHECK(strcmp(str(r1, "tool_use_id"), "call_1_x") == 0); /* the same id as its tool_use */
    CHECK(strcmp(str(r1, "content"), "BLOCK") == 0);

    const cJSON *m3 = cJSON_GetArrayItem(out, 3);
    CHECK(strcmp(str(m3, "role"), "assistant") == 0);
    CHECK(cJSON_GetArraySize(cJSON_GetObjectItem(m3, "content")) == 2); /* two notes merged */
    /* ...then the owner's last words (the empty message is left out): roles alternate. */
    const cJSON *m4 = cJSON_GetArrayItem(out, 4);
    CHECK(strcmp(str(m4, "role"), "user") == 0);
    CHECK(cJSON_GetArraySize(cJSON_GetObjectItem(m4, "content")) == 1);
    cJSON_Delete(req);

    /* Tools: name, description, input_schema; an unparsable schema becomes an empty object. */
    messages = cJSON_Parse("[{\"role\":\"system\",\"content\":\"s\"},"
                           "{\"role\":\"user\",\"content\":\"hi\"}]");
    body = nc_anthropic_request("m", 0, messages, tools, 2);
    cJSON_Delete(messages);
    req = cJSON_Parse(body);
    free(body);
    CHECK(cJSON_GetObjectItem(req, "max_tokens")->valueint == 1024);
    const cJSON *list = cJSON_GetObjectItem(req, "tools");
    CHECK(cJSON_GetArraySize(list) == 2);
    const cJSON *t0 = cJSON_GetArrayItem(list, 0);
    CHECK(strcmp(str(t0, "name"), "living_light_on") == 0);
    CHECK(strcmp(str(t0, "description"), "Turn the light on.") == 0);
    CHECK(strcmp(str(cJSON_GetObjectItem(t0, "input_schema"), "type"), "object") == 0);
    CHECK(cJSON_GetObjectItem(t0, "type") == NULL && cJSON_GetObjectItem(t0, "function") == NULL);
    const cJSON *t1 = cJSON_GetArrayItem(list, 1);
    CHECK(strcmp(str(t1, "description"), "") == 0);
    CHECK(cJSON_IsObject(cJSON_GetObjectItem(t1, "input_schema")));
    cJSON_Delete(req);

    /* A kept history that starts with the assistant (the rest was trimmed). */
    messages = cJSON_Parse("[{\"role\":\"system\",\"content\":\"s\"},"
                           "{\"role\":\"assistant\",\"content\":\"Đã bật đèn.\"},"
                           "{\"role\":\"user\",\"content\":\"tắt đi\"}]");
    body = nc_anthropic_request("m", 64, messages, NULL, 0);
    cJSON_Delete(messages);
    req = cJSON_Parse(body);
    free(body);
    out = cJSON_GetObjectItem(req, "messages");
    CHECK(cJSON_GetArraySize(out) == 3);
    CHECK(strcmp(str(cJSON_GetArrayItem(out, 0), "role"), "user") == 0);
    CHECK(strcmp(str(cJSON_GetArrayItem(out, 1), "role"), "assistant") == 0);
    CHECK(cJSON_GetObjectItem(req, "tools") == NULL);
    cJSON_Delete(req);

    /* An assistant message with only nameless calls is dropped, not sent empty; a
     * tool result whose call was not emitted just before it is dropped. */
    messages = cJSON_Parse(
        "[{\"role\":\"system\",\"content\":\"s\"},"
        "{\"role\":\"user\",\"content\":\"a\"},"
        "{\"role\":\"assistant\",\"content\":\"\",\"tool_calls\":[{\"id\":\"x\","
        "\"type\":\"function\",\"function\":{\"name\":\"\",\"arguments\":\"{}\"}}]},"
        "{\"role\":\"tool\",\"tool_call_id\":\"x\",\"content\":\"orphan\"},"
        "{\"role\":\"assistant\",\"content\":null,\"tool_calls\":[{\"id\":\"k\","
        "\"type\":\"function\",\"function\":{\"name\":\"t\",\"arguments\":\"{}\"}}]},"
        "{\"role\":\"tool\",\"tool_call_id\":\"k\",\"content\":\"kept\"},"
        "{\"role\":\"tool\",\"tool_call_id\":\"other\",\"content\":\"unknown id\"},"
        "{\"role\":\"assistant\",\"content\":\"ok\"},"
        "{\"role\":\"user\",\"content\":\"b\"},"
        "{\"role\":\"tool\",\"tool_call_id\":\"k\",\"content\":\"after the owner\"}]");
    body = nc_anthropic_request("m", 64, messages, NULL, 0);
    cJSON_Delete(messages);
    req = cJSON_Parse(body);
    free(body);
    out = cJSON_GetObjectItem(req, "messages");
    /* user a, assistant [tool_use k], user [tool_result k], assistant ok, user b */
    CHECK(cJSON_GetArraySize(out) == 5);
    const cJSON *uses_k = cJSON_GetObjectItem(cJSON_GetArrayItem(out, 1), "content");
    CHECK(cJSON_GetArraySize(uses_k) == 1 && strcmp(str(cJSON_GetArrayItem(uses_k, 0), "id"), "k") == 0);
    const cJSON *results_k = cJSON_GetObjectItem(cJSON_GetArrayItem(out, 2), "content");
    CHECK(cJSON_GetArraySize(results_k) == 1);
    CHECK(strcmp(str(cJSON_GetArrayItem(results_k, 0), "content"), "kept") == 0);
    const cJSON *last = cJSON_GetObjectItem(cJSON_GetArrayItem(out, 4), "content");
    CHECK(cJSON_GetArraySize(last) == 1 &&
          strcmp(str(cJSON_GetArrayItem(last, 0), "type"), "text") == 0);
    cJSON_Delete(req);

    CHECK(nc_anthropic_request(NULL, 1, messages, NULL, 0) == NULL);
    CHECK(nc_anthropic_request("m", 1, NULL, NULL, 0) == NULL);
    return 0;
}

static int check_parse(void)
{
    nc_llm_reply reply;
    /* Text and calls together: the calls win, as nc_llm_parse. */
    CHECK(nc_anthropic_parse(
              "{\"id\":\"msg_1\",\"type\":\"message\",\"role\":\"assistant\",\"content\":["
              "{\"type\":\"text\",\"text\":\"Tôi sẽ bật đèn.\"},"
              "{\"type\":\"tool_use\",\"id\":\"toolu_01\",\"name\":\"living_light_on\","
              "\"input\":{}},"
              "{\"type\":\"tool_use\",\"id\":\"toolu_02\",\"name\":\"front_door_unlock\","
              "\"input\":{\"duration_s\":10}},"
              "{\"type\":\"tool_use\",\"id\":\"toolu_03\",\"name\":\"\",\"input\":{}}],"
              "\"stop_reason\":\"tool_use\"}",
              &reply) == 0);
    CHECK(reply.text == NULL);
    CHECK(reply.call_count == 2);
    CHECK(strcmp(reply.calls[0].id, "toolu_01") == 0);
    CHECK(strcmp(reply.calls[0].name, "living_light_on") == 0);
    CHECK(strcmp(reply.calls[0].arguments, "{}") == 0);
    CHECK(strcmp(reply.calls[1].arguments, "{\"duration_s\":10}") == 0);
    nc_llm_reply_free(&reply);

    CHECK(nc_anthropic_parse("{\"content\":[{\"type\":\"text\",\"text\":\"Xin \"},"
                             "{\"type\":\"thinking\",\"thinking\":\"...\"},"
                             "{\"type\":\"text\",\"text\":\"chào.\"}],\"stop_reason\":\"end_turn\"}",
                             &reply) == 0);
    CHECK(reply.call_count == 0 && reply.text != NULL && strcmp(reply.text, "Xin chào.") == 0);
    nc_llm_reply_free(&reply);

    /* A call whose input is missing: arguments "{}". More than NC_MAX_TOOL_CALLS: cut. */
    CHECK(nc_anthropic_parse("{\"content\":["
                             "{\"type\":\"tool_use\",\"id\":\"a\",\"name\":\"x\"},"
                             "{\"type\":\"tool_use\",\"id\":\"b\",\"name\":\"x\",\"input\":{}},"
                             "{\"type\":\"tool_use\",\"id\":\"c\",\"name\":\"x\",\"input\":{}},"
                             "{\"type\":\"tool_use\",\"id\":\"d\",\"name\":\"x\",\"input\":{}},"
                             "{\"type\":\"tool_use\",\"id\":\"e\",\"name\":\"x\",\"input\":{}}]}",
                             &reply) == 0);
    CHECK(reply.call_count == NC_MAX_TOOL_CALLS);
    CHECK(strcmp(reply.calls[0].arguments, "{}") == 0);
    nc_llm_reply_free(&reply);

    /* Out of tokens: calls with cut arguments are never run, the parse fails; text alone
     * is kept, marked. */
    CHECK(nc_anthropic_parse("{\"content\":[{\"type\":\"text\",\"text\":\"Mở \"},"
                             "{\"type\":\"tool_use\",\"id\":\"a\",\"name\":\"front_door_unlock\","
                             "\"input\":{\"duration_s\":3}}],\"stop_reason\":\"max_tokens\"}",
                             &reply) == -1);
    CHECK(reply.truncated && reply.call_count == 0 && reply.text == NULL);
    nc_llm_reply_free(&reply);
    CHECK(nc_anthropic_parse("{\"content\":[{\"type\":\"text\",\"text\":\"Một câu bị cắt\"}],"
                             "\"stop_reason\":\"max_tokens\"}",
                             &reply) == 0);
    CHECK(reply.truncated && reply.text != NULL && strcmp(reply.text, "Một câu bị cắt") == 0);
    nc_llm_reply_free(&reply);
    CHECK(nc_anthropic_parse("{\"content\":[{\"type\":\"text\",\"text\":\"x\"}],"
                             "\"stop_reason\":\"end_turn\"}",
                             &reply) == 0);
    CHECK(!reply.truncated);
    nc_llm_reply_free(&reply);

    /* Neither, an error object, not JSON, an OpenAI reply: -1. */
    CHECK(nc_anthropic_parse("{\"content\":[]}", &reply) == -1);
    CHECK(nc_anthropic_parse("{\"type\":\"error\",\"error\":{\"type\":\"overloaded_error\"}}",
                             &reply) == -1);
    CHECK(nc_anthropic_parse("nope", &reply) == -1);
    CHECK(nc_anthropic_parse(NULL, &reply) == -1);
    CHECK(nc_anthropic_parse("{\"choices\":[{\"message\":{\"content\":\"hi\"}}]}", &reply) == -1);
    CHECK(nc_anthropic_parse("{}", NULL) == -1);
    return 0;
}

int main(void)
{
    if (check_api_names() != 0) return 1;
    if (check_request() != 0) return 1;
    if (check_parse() != 0) return 1;
    printf("NC_LLM OK checks=%u\n", s_checks);
    return 0;
}
