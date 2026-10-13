/*
 * Host runner for NeuroClaw-ESP's memory and schedule (nc_workspace.c,
 * nc_cron.c): workspace paths and tools on a real directory, the prompt with
 * the bootstrap files and MEMORY.md, the saved session, cron expressions, the
 * cron tool and due jobs, and the HTTP Date clock. Usage: test_memory_host
 * <empty temp dir>. Exit 0 and `NC_MEMORY OK checks=<n>` only when all passed.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nc_cron.h"
#include "nc_utf8.h"
#include "nc_workspace.h"

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

static const char *s_base;

static char *tool(const char *name, const char *args_json)
{
    cJSON *args = cJSON_Parse(args_json);
    char *out = nc_ws_tool(s_base, name, args);
    cJSON_Delete(args);
    return out;
}

static int starts(char *out, const char *prefix)
{
    const int ok = out != NULL && strncmp(out, prefix, strlen(prefix)) == 0;
    if (!ok) fprintf(stderr, "  got: %s\n", out ? out : "(null)");
    free(out);
    return ok;
}

static int check_paths(void)
{
    char full[256];
    CHECK(nc_ws_path("/ws", "memory/MEMORY.md", true, full, sizeof full));
    CHECK(strcmp(full, "/ws/memory/MEMORY.md") == 0);
    CHECK(nc_ws_path("/ws", "memory/202610/20261013.md", true, full, sizeof full));
    CHECK(!nc_ws_path("/ws", "../etc/passwd", false, full, sizeof full));
    CHECK(!nc_ws_path("/ws", "memory/../../x", false, full, sizeof full));
    CHECK(!nc_ws_path("/ws", "/etc/passwd", false, full, sizeof full));
    CHECK(!nc_ws_path("/ws", ".hidden", false, full, sizeof full));
    CHECK(!nc_ws_path("/ws", "a//b", false, full, sizeof full));
    CHECK(!nc_ws_path("/ws", "a b", false, full, sizeof full));
    CHECK(!nc_ws_path("/ws", "", false, full, sizeof full));
    CHECK(!nc_ws_path("/ws", "a/b/c/d/e", false, full, sizeof full));
    CHECK(!nc_ws_path("/ws", "sessions/telegram.json", true, full, sizeof full));
    CHECK(!nc_ws_path("/ws", "cron/jobs.json", true, full, sizeof full));
    CHECK(!nc_ws_path("/ws", "cron", true, full, sizeof full));
    CHECK(nc_ws_path("/ws", "cron/jobs.json", false, full, sizeof full)); /* readable */
    CHECK(nc_ws_path("/ws", "cronjobs.md", true, full, sizeof full));
    CHECK(nc_ws_path("/ws", "sessionsnotes.md", true, full, sizeof full));
    return 0;
}

static int check_tools(void)
{
    CHECK(starts(tool("write_file", "{\"path\":\"memory/MEMORY.md\",\"content\":\"- Chủ nhà tên Minh\\n\"}"),
                 "ok: wrote memory/MEMORY.md"));
    CHECK(starts(tool("append_file", "{\"path\":\"memory/MEMORY.md\",\"content\":\"- Thích đèn ấm\\n\"}"),
                 "ok: appended"));
    char *body = tool("read_file", "{\"path\":\"memory/MEMORY.md\"}");
    CHECK(body != NULL && strcmp(body, "- Chủ nhà tên Minh\n- Thích đèn ấm\n") == 0);
    free(body);
    CHECK(starts(tool("edit_file", "{\"path\":\"memory/MEMORY.md\",\"old_text\":\"đèn ấm\","
                                   "\"new_text\":\"đèn vàng 2700K\"}"),
                 "ok: edited"));
    body = tool("read_file", "{\"path\":\"memory/MEMORY.md\"}");
    CHECK(body != NULL && strstr(body, "đèn vàng 2700K") != NULL && strstr(body, "đèn ấm") == NULL);
    free(body);
    CHECK(starts(tool("edit_file", "{\"path\":\"memory/MEMORY.md\",\"old_text\":\"không có\","
                                   "\"new_text\":\"x\"}"),
                 "error: old_text not found"));
    CHECK(starts(tool("edit_file", "{\"path\":\"memory/MEMORY.md\",\"old_text\":\"- \","
                                   "\"new_text\":\"x\"}"),
                 "error: old_text appears more than once"));
    CHECK(starts(tool("write_file", "{\"path\":\"memory/202610/20261013.md\",\"content\":\"note\"}"),
                 "ok: wrote"));
    body = tool("list_dir", "{\"path\":\"memory\"}");
    CHECK(body != NULL && strstr(body, "\"MEMORY.md\"") != NULL && strstr(body, "\"202610\"") != NULL &&
          strstr(body, "\"dir\"") != NULL && strstr(body, ".tmp") == NULL);
    free(body);
    body = tool("list_dir", "{}");
    CHECK(body != NULL && strstr(body, "\"memory\"") != NULL);
    free(body);

    CHECK(starts(tool("write_file", "{\"path\":\"sessions/telegram.json\",\"content\":\"[]\"}"),
                 "error: sessions/telegram.json is not a workspace path"));
    CHECK(starts(tool("write_file", "{\"path\":\"cron/jobs.json\",\"content\":\"[]\"}"), "error:"));
    CHECK(starts(tool("read_file", "{\"path\":\"../../etc/passwd\"}"), "error:"));
    CHECK(starts(tool("read_file", "{\"path\":\"nope.md\"}"), "error: no file nope.md"));
    CHECK(starts(tool("write_file", "{\"path\":\"x.md\"}"), "error: write_file needs content"));
    CHECK(starts(tool("read_file", "{}"), "error:"));
    CHECK(tool("exec", "{}") == NULL); /* not a workspace tool, and there is no such tool */

    /* 16 KB per file. */
    char *big = malloc(NC_FILE_MAX + 64);
    memset(big, 'a', NC_FILE_MAX + 1);
    big[NC_FILE_MAX + 1] = '\0';
    cJSON *args = cJSON_CreateObject();
    cJSON_AddStringToObject(args, "path", "big.md");
    cJSON_AddStringToObject(args, "content", big);
    CHECK(starts(nc_ws_tool(s_base, "write_file", args), "error: big.md would exceed 16 KB"));
    cJSON_Delete(args);
    free(big);

    for (int i = 0; i < 5; i++) {
        CHECK(nc_ws_tool_schema(NC_WS_TOOLS[i]) != NULL);
        cJSON *schema = cJSON_Parse(nc_ws_tool_schema(NC_WS_TOOLS[i]));
        CHECK(cJSON_IsObject(schema));
        cJSON_Delete(schema);
        CHECK(nc_ws_tool_description(NC_WS_TOOLS[i]) != NULL);
    }
    CHECK(nc_ws_tool_schema("exec") == NULL);
    return 0;
}

static int check_prompt_and_session(void)
{
    char path[256];
    snprintf(path, sizeof path, "%s/USER.md", s_base);
    CHECK(nc_ws_store(path, "Owner: Minh, Hà Nội.", false) == 0);
    char *prompt = nc_system_prompt(s_base, "BASE", "2026-10-13 07:00 (Mon)");
    CHECK(prompt != NULL && strncmp(prompt, "BASE", 4) == 0);
    CHECK(strstr(prompt, "Current time: 2026-10-13 07:00 (Mon)") != NULL);
    CHECK(strstr(prompt, "## USER.md\n\nOwner: Minh, Hà Nội.") != NULL);
    CHECK(strstr(prompt, "## memory/MEMORY.md") != NULL && strstr(prompt, "2700K") != NULL);
    CHECK(strstr(prompt, "## SOUL.md") == NULL); /* absent files add nothing */
    free(prompt);
    prompt = nc_system_prompt(s_base, "BASE", "");
    CHECK(prompt != NULL && strstr(prompt, "Current time") == NULL);
    CHECK(strstr(prompt, "## Skills") == NULL);
    free(prompt);

    /* Skills: one line each from the SKILL.md front matter. */
    snprintf(path, sizeof path, "%s/skills/weather/SKILL.md", s_base);
    CHECK(nc_ws_store(path, "---\nname: weather\ndescription: \"Thời tiết theo giờ.\"\n---\n# W\n",
                      false) == 0);
    snprintf(path, sizeof path, "%s/skills/plain/SKILL.md", s_base);
    CHECK(nc_ws_store(path, "# no front matter\n", false) == 0);
    prompt = nc_system_prompt(s_base, "BASE", "");
    CHECK(prompt != NULL);
    CHECK(strstr(prompt, "- weather: Thời tiết theo giờ. (skills/weather/SKILL.md)") != NULL);
    CHECK(strstr(prompt, "- plain:  (skills/plain/SKILL.md)") != NULL);
    free(prompt);
    char *read = tool("read_file", "{\"path\":\"skills/weather/SKILL.md\"}");
    CHECK(read != NULL && strstr(read, "# W") != NULL);
    free(read);

    CHECK(nc_session_load(s_base) == NULL);
    cJSON *messages = cJSON_Parse("[{\"role\":\"system\",\"content\":\"s\"},"
                                  "{\"role\":\"user\",\"content\":\"bật đèn\"}]");
    CHECK(nc_session_save(s_base, messages) == 0);
    cJSON *back = nc_session_load(s_base);
    CHECK(back != NULL && cJSON_Compare(messages, back, true));
    cJSON_Delete(back);
    cJSON_Delete(messages);
    snprintf(path, sizeof path, "%s/sessions/telegram.json", s_base);
    CHECK(nc_ws_store(path, "{broken", false) == 0);
    CHECK(nc_session_load(s_base) == NULL); /* a broken file is a new conversation */

    /* Power lost after the old file was removed: the new one is still in .tmp. */
    char tmp[300];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    remove(path);
    FILE *f = fopen(tmp, "wb");
    CHECK(f != NULL);
    fputs("[{\"role\":\"system\",\"content\":\"s\"}]", f);
    fclose(f);
    back = nc_session_load(s_base);
    CHECK(back != NULL && cJSON_GetArraySize(back) == 1);
    cJSON_Delete(back);
    CHECK(nc_ws_store(path, "[]", false) == 0); /* the next save takes the main name again */
    back = nc_session_load(s_base);
    CHECK(back != NULL && cJSON_GetArraySize(back) == 0);
    cJSON_Delete(back);
    return 0;
}

static int64_t at(int y, int mo, int d, int h, int mi)
{
    struct tm tm = {0};
    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_isdst = -1;
    return (int64_t)mktime(&tm);
}

static int check_cron_expr(void)
{
    nc_cron_expr e;
    CHECK(nc_cron_parse("0 9 * * *", &e));
    const int64_t mon = at(2026, 10, 12, 10, 0); /* Monday 2026-10-12 10:00 */
    CHECK(nc_cron_next(&e, mon) == at(2026, 10, 13, 9, 0));
    CHECK(nc_cron_next(&e, at(2026, 10, 13, 8, 59)) == at(2026, 10, 13, 9, 0));
    CHECK(nc_cron_next(&e, at(2026, 10, 13, 9, 0)) == at(2026, 10, 14, 9, 0));
    CHECK(nc_cron_parse("*/15 * * * *", &e));
    CHECK(nc_cron_next(&e, at(2026, 10, 13, 9, 1)) == at(2026, 10, 13, 9, 15));
    CHECK(nc_cron_parse("30 18 * * 1-5", &e)); /* weekdays */
    CHECK(nc_cron_next(&e, at(2026, 10, 16, 19, 0)) == at(2026, 10, 19, 18, 30)); /* Fri → Mon */
    CHECK(nc_cron_parse("0 0 1 1 *", &e));
    CHECK(nc_cron_next(&e, mon) == at(2027, 1, 1, 0, 0));
    CHECK(nc_cron_parse("0 12 29 2 *", &e));
    CHECK(nc_cron_next(&e, mon) == at(2028, 2, 29, 12, 0));
    CHECK(nc_cron_parse("0 7 * * 0,7", &e)); /* Sunday both ways */
    CHECK(nc_cron_next(&e, mon) == at(2026, 10, 18, 7, 0));
    CHECK(nc_cron_parse("0 8 13 * 5", &e)); /* the 13th or a Friday */
    CHECK(nc_cron_next(&e, mon) == at(2026, 10, 13, 8, 0));
    CHECK(nc_cron_next(&e, at(2026, 10, 13, 9, 0)) == at(2026, 10, 16, 8, 0));
    CHECK(nc_cron_parse("5,35 6-8/2 * * *", &e));
    CHECK(nc_cron_next(&e, at(2026, 10, 13, 6, 40)) == at(2026, 10, 13, 8, 5));
    CHECK(nc_cron_parse("0 0 31 2 *", &e));
    CHECK(nc_cron_next(&e, mon) == -1); /* never */

    CHECK(!nc_cron_parse("0 9 * *", &e));
    CHECK(!nc_cron_parse("0 9 * * * *", &e));
    CHECK(!nc_cron_parse("60 9 * * *", &e));
    CHECK(!nc_cron_parse("0 24 * * *", &e));
    CHECK(!nc_cron_parse("0 9 0 * *", &e));
    CHECK(!nc_cron_parse("0 9 * 13 *", &e));
    CHECK(!nc_cron_parse("0 9 * * 8", &e));
    CHECK(!nc_cron_parse("*/0 9 * * *", &e));
    CHECK(!nc_cron_parse("5-1 9 * * *", &e));
    CHECK(!nc_cron_parse("a 9 * * *", &e));
    CHECK(!nc_cron_parse(NULL, &e));
    return 0;
}

static char *cron(cJSON *jobs, const char *args_json, int64_t now, bool *changed)
{
    cJSON *args = cJSON_Parse(args_json);
    char *out = nc_cron_tool(jobs, args, now, changed);
    cJSON_Delete(args);
    return out;
}

static int check_cron_tool(void)
{
    cJSON *jobs = cJSON_CreateArray();
    bool changed = false;
    const int64_t now = at(2026, 10, 13, 6, 0);

    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"message\":\"x\",\"at_seconds\":60}", 1000, &changed),
                 "error: the chip's clock is not set yet"));
    CHECK(!changed);
    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"message\":\"x\",\"command\":\"rm -rf /\","
                            "\"at_seconds\":60}",
                      now, &changed),
                 "error: NeuroClaw never runs commands"));
    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"message\":\"x\"}", now, &changed),
                 "error: give exactly one"));
    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"message\":\"x\",\"at_seconds\":60,"
                            "\"every_seconds\":60}",
                      now, &changed),
                 "error: give exactly one"));
    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"message\":\"x\",\"every_seconds\":10}", now, &changed),
                 "error: every_seconds must be at least 60"));
    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"message\":\"x\",\"cron_expr\":\"bad\"}", now, &changed),
                 "error: cron_expr is not"));
    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"at_seconds\":60}", now, &changed),
                 "error: message is required"));
    CHECK(cJSON_GetArraySize(jobs) == 0);

    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"message\":\"Nhắc uống thuốc\",\"at_seconds\":600}",
                      now, &changed),
                 "ok: added\n- Nhắc uống thuốc (id: j1, one-time, enabled, next 2026-10-13 06:10)"));
    CHECK(changed);
    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"name\":\"Đèn tối\",\"message\":\"Bật đèn phòng "
                            "khách\",\"cron_expr\":\"0 18 * * *\"}",
                      now, &changed),
                 "ok: added\n- Đèn tối (id: j2, 0 18 * * *, enabled, next 2026-10-13 18:00)"));
    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"message\":\"Kiểm tra\",\"every_seconds\":3600}",
                      now, &changed),
                 "ok: added\n- Kiểm tra (id: j3, every 3600s"));
    CHECK(starts(cron(jobs, "{\"action\":\"list\"}", now, &changed), "Scheduled jobs:\n- Nhắc"));
    CHECK(!changed);
    CHECK(starts(cron(jobs, "{\"action\":\"get\",\"job_id\":\"j2\"}", now, &changed), "{\"id\":\"j2\""));
    CHECK(starts(cron(jobs, "{\"action\":\"get\",\"job_id\":\"j9\"}", now, &changed), "error: no job j9"));
    CHECK(nc_cron_seconds_to_next(jobs, now) == 600);

    /* Due at 06:10: the reminder, then it is gone. */
    CHECK(nc_cron_take_due(jobs, now + 599, &changed) == NULL);
    cJSON *due = nc_cron_take_due(jobs, now + 600, &changed);
    CHECK(due != NULL && changed);
    CHECK(strcmp(cJSON_GetObjectItem(due, "message")->valuestring, "Nhắc uống thuốc") == 0);
    cJSON_Delete(due);
    CHECK(cJSON_GetArraySize(jobs) == 2);
    /* 07:00: the hourly check, rescheduled to 08:00 — a missed hour is not replayed. */
    due = nc_cron_take_due(jobs, at(2026, 10, 13, 7, 30), &changed);
    CHECK(due != NULL && strcmp(cJSON_GetObjectItem(due, "id")->valuestring, "j3") == 0);
    cJSON_Delete(due);
    CHECK(nc_cron_take_due(jobs, at(2026, 10, 13, 7, 30), &changed) == NULL);
    CHECK(nc_cron_seconds_to_next(jobs, at(2026, 10, 13, 7, 30)) == 30 * 60);

    CHECK(starts(cron(jobs, "{\"action\":\"disable\",\"job_id\":\"j3\"}", now, &changed), "ok: disabled"));
    CHECK(nc_cron_take_due(jobs, at(2026, 10, 13, 9, 0), &changed) == NULL);
    CHECK(starts(cron(jobs, "{\"action\":\"enable\",\"job_id\":\"j3\"}", at(2026, 10, 13, 12, 0), &changed),
                 "ok: enabled"));
    CHECK(nc_cron_seconds_to_next(jobs, at(2026, 10, 13, 12, 0)) == 3600);
    CHECK(starts(cron(jobs, "{\"action\":\"update\",\"job_id\":\"j2\",\"cron_expr\":\"30 19 * * *\"}",
                      at(2026, 10, 13, 12, 0), &changed),
                 "ok: updated\n- Đèn tối (id: j2, 30 19 * * *"));
    CHECK(starts(cron(jobs, "{\"action\":\"remove\",\"job_id\":\"j3\"}", now, &changed), "ok: removed"));
    CHECK(cJSON_GetArraySize(jobs) == 1);
    CHECK(starts(cron(jobs, "{\"action\":\"add\",\"message\":\"y\",\"at_seconds\":5}", now, &changed),
                 "ok: added\n- y (id: j3,"));
    CHECK(starts(cron(jobs, "{\"action\":\"fly\",\"job_id\":\"j3\"}", now, &changed), "error: unknown action"));
    CHECK(starts(cron(jobs, "{}", now, &changed), "error: action is required"));

    for (int i = 0; i < NC_CRON_MAX_JOBS; i++) {
        free(cron(jobs, "{\"action\":\"add\",\"message\":\"z\",\"at_seconds\":5}", now, &changed));
    }
    CHECK(cJSON_GetArraySize(jobs) == NC_CRON_MAX_JOBS);
    cJSON_Delete(jobs);
    return 0;
}

static int check_clock(void)
{
    CHECK(nc_http_date("Tue, 13 Oct 2026 05:45:00 GMT") == INT64_C(1791870300));
    CHECK(nc_http_date("Thu, 01 Jan 1970 00:00:00 GMT") == 0);
    CHECK(nc_http_date("Tue, 29 Feb 2028 12:00:00 GMT") == INT64_C(1835438400));
    CHECK(nc_http_date("Tue, 13 Oct 2026 05:45:00 UTC") == -1);
    CHECK(nc_http_date("Tue, 13 Foo 2026 05:45:00 GMT") == -1);
    CHECK(nc_http_date("Tue, 13 Oct 2026 25:45:00 GMT") == -1);
    CHECK(nc_http_date("garbage") == -1);
    CHECK(nc_http_date(NULL) == -1);
    CHECK(nc_http_date("Tue, 13 Oct 2026 05:45:00 GMT") > NC_CLOCK_VALID);
    return 0;
}

static int check_utf8(void)
{
    char a[] = "Nh\xE1\xBA\xAF"; /* "Nhắ": whole */
    nc_utf8_trim(a);
    CHECK(strcmp(a, "Nh\xE1\xBA\xAF") == 0);
    char b[] = "Nh\xE1\xBA"; /* the last character cut after two of three bytes */
    nc_utf8_trim(b);
    CHECK(strcmp(b, "Nh") == 0);
    char c[] = "Nh\xE1"; /* after one byte */
    nc_utf8_trim(c);
    CHECK(strcmp(c, "Nh") == 0);
    char d[] = "x\xC3"; /* a two-byte character cut */
    nc_utf8_trim(d);
    CHECK(strcmp(d, "x") == 0);
    char e[] = "x\xC3\xA0";
    nc_utf8_trim(e);
    CHECK(strcmp(e, "x\xC3\xA0") == 0);
    char f[] = "ascii";
    nc_utf8_trim(f);
    CHECK(strcmp(f, "ascii") == 0);
    char g[] = "";
    nc_utf8_trim(g);
    CHECK(g[0] == '\0');
    char h[] = "\xF0\x9F\x98"; /* an emoji cut */
    nc_utf8_trim(h);
    CHECK(h[0] == '\0');

    /* A job named from a long Vietnamese message is still valid UTF-8. */
    cJSON *jobs = cJSON_CreateArray();
    bool changed = false;
    char *out = cron(jobs, "{\"action\":\"add\",\"message\":\"Nhắc chủ nhà uống thuốc buổi sáng\","
                           "\"at_seconds\":5}",
                     INT64_C(1791870300), &changed);
    CHECK(out != NULL);
    const char *name = cJSON_GetObjectItem(cJSON_GetArrayItem(jobs, 0), "name")->valuestring;
    char copy[64];
    snprintf(copy, sizeof copy, "%s", name);
    nc_utf8_trim(copy);
    CHECK(strcmp(copy, name) == 0); /* nothing left to trim: it was cut on a boundary */
    CHECK(strlen(name) <= 30);
    free(out);
    cJSON_Delete(jobs);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    s_base = argv[1];
    setenv("TZ", "ICT-7", 1); /* Asia/Ho_Chi_Minh, no DST: the tests read local times */
    tzset();
    if (check_paths() != 0) return 1;
    if (check_tools() != 0) return 1;
    if (check_prompt_and_session() != 0) return 1;
    if (check_cron_expr() != 0) return 1;
    if (check_cron_tool() != 0) return 1;
    if (check_clock() != 0) return 1;
    if (check_utf8() != 0) return 1;
    printf("NC_MEMORY OK checks=%u\n", s_checks);
    return 0;
}
