/*
 * NeuroClaw-ESP's scheduled tasks (nc_cron.h). Arguments and behaviour after
 * the Go NeuroClaw's pkg/tools/cron.go and pkg/cron/service.go.
 */
#define _POSIX_C_SOURCE 200809L /* localtime_r on the host */
#include "nc_cron.h"
#include "nc_utf8.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* --- expressions --------------------------------------------------------------------------- */

static bool number(const char **p, int *out)
{
    if (**p < '0' || **p > '9') return false;
    int v = 0;
    while (**p >= '0' && **p <= '9') {
        v = v * 10 + (**p - '0');
        if (v > 1000) return false;
        (*p)++;
    }
    *out = v;
    return true;
}

/* One field into `bits` (bit i = value i). */
static bool field(const char *text, int lo, int hi, uint64_t *bits, bool *any)
{
    *bits = 0;
    *any = strcmp(text, "*") == 0;
    const char *p = text;
    while (true) {
        int a, b, step = 1;
        if (*p == '*') {
            a = lo;
            b = hi;
            p++;
        } else {
            if (!number(&p, &a)) return false;
            b = a;
            if (*p == '-') {
                p++;
                if (!number(&p, &b)) return false;
            }
        }
        if (*p == '/') {
            p++;
            if (!number(&p, &step) || step < 1) return false;
            if (a == b && text[0] != '*') b = hi; /* "5/15" means 5-max/15 */
        }
        if (a < lo || b > hi || a > b) return false;
        for (int v = a; v <= b; v += step) *bits |= UINT64_C(1) << v;
        if (*p == ',') {
            p++;
            continue;
        }
        return *p == '\0';
    }
}

bool nc_cron_parse(const char *expr, nc_cron_expr *out)
{
    if (expr == NULL || out == NULL) return false;
    char copy[128];
    if (strlen(expr) >= sizeof copy) return false;
    strcpy(copy, expr);
    char *parts[6];
    int n = 0;
    for (char *tok = strtok(copy, " \t"); tok != NULL; tok = strtok(NULL, " \t")) {
        if (n == 6) return false;
        parts[n++] = tok;
    }
    if (n != 5) return false;
    uint64_t bits;
    bool any;
    memset(out, 0, sizeof *out);
    if (!field(parts[0], 0, 59, &bits, &any)) return false;
    out->minutes = bits;
    if (!field(parts[1], 0, 23, &bits, &any)) return false;
    out->hours = (uint32_t)bits;
    if (!field(parts[2], 1, 31, &bits, &out->any_day)) return false;
    out->days = (uint32_t)bits;
    if (!field(parts[3], 1, 12, &bits, &any)) return false;
    out->months = (uint16_t)bits;
    if (!field(parts[4], 0, 7, &bits, &out->any_weekday)) return false;
    if (bits & (UINT64_C(1) << 7)) bits |= 1; /* 7 is Sunday too */
    out->weekdays = (uint8_t)(bits & 0x7f);
    return true;
}

static bool day_matches(const nc_cron_expr *e, const struct tm *tm)
{
    const bool dom = (e->days >> tm->tm_mday) & 1u;
    const bool dow = (e->weekdays >> tm->tm_wday) & 1u;
    if (e->any_day && e->any_weekday) return true;
    if (e->any_day) return dow;
    if (e->any_weekday) return dom;
    return dom || dow;
}

int64_t nc_cron_next(const nc_cron_expr *e, int64_t after)
{
    if (e == NULL) return -1;
    time_t t = (time_t)(after - (after % 60) + 60);
    const time_t limit = (time_t)(after + INT64_C(5) * 366 * 86400);
    while (t <= limit) {
        struct tm tm;
        localtime_r(&t, &tm);
        if (!((e->months >> (tm.tm_mon + 1)) & 1u)) {
            tm.tm_mon++;
            tm.tm_mday = 1;
            tm.tm_hour = 0;
            tm.tm_min = 0;
        } else if (!day_matches(e, &tm)) {
            tm.tm_mday++;
            tm.tm_hour = 0;
            tm.tm_min = 0;
        } else if (!((e->hours >> tm.tm_hour) & 1u)) {
            tm.tm_hour++;
            tm.tm_min = 0;
        } else if (!((e->minutes >> tm.tm_min) & 1u)) {
            t += 60;
            continue;
        } else {
            return (int64_t)t;
        }
        tm.tm_sec = 0;
        tm.tm_isdst = -1;
        const time_t next = mktime(&tm);
        t = next > t ? next : t + 60;
    }
    return -1;
}

/* --- jobs ---------------------------------------------------------------------------------- */

static const char *str(const cJSON *o, const char *k)
{
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k);
    return cJSON_IsString(v) ? v->valuestring : NULL;
}

static bool num(const cJSON *o, const char *k, int64_t *out)
{
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k);
    if (!cJSON_IsNumber(v)) return false;
    *out = (int64_t)v->valuedouble;
    return true;
}

static void set_num(cJSON *o, const char *k, int64_t v)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(o, k);
    if (item != NULL) {
        cJSON_SetNumberValue(item, (double)v);
    } else {
        cJSON_AddNumberToObject(o, k, (double)v);
    }
}

static void set_str(cJSON *o, const char *k, const char *v)
{
    cJSON_DeleteItemFromObjectCaseSensitive(o, k);
    cJSON_AddStringToObject(o, k, v);
}

static bool enabled(const cJSON *job)
{
    return !cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(job, "enabled"));
}

static char *out_text(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static char *out_text(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0) return NULL;
    char *out = malloc((size_t)n + 1);
    if (out == NULL) return NULL;
    va_start(ap, fmt);
    vsnprintf(out, (size_t)n + 1, fmt, ap);
    va_end(ap);
    return out;
}

static void when(char *out, size_t cap, int64_t epoch)
{
    const time_t t = (time_t)epoch;
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(out, cap, "%Y-%m-%d %H:%M", &tm);
}

static cJSON *find(cJSON *jobs, const char *id)
{
    cJSON *job = NULL;
    if (id == NULL) return NULL;
    cJSON_ArrayForEach(job, jobs)
    {
        const char *jid = str(job, "id");
        if (jid != NULL && strcmp(jid, id) == 0) return job;
    }
    return NULL;
}

/* Move a recurring job's next run past `now`; a cron job that never matches
 * again is disabled. Missed runs are not replayed. */
static void roll_forward(cJSON *job, int64_t now)
{
    const char *kind = str(job, "kind");
    int64_t next = 0;
    num(job, "next", &next);
    if (kind != NULL && strcmp(kind, "every") == 0) {
        int64_t every = NC_CRON_MIN_EVERY_S;
        num(job, "every_s", &every);
        if (every < NC_CRON_MIN_EVERY_S) every = NC_CRON_MIN_EVERY_S;
        if (next <= now) next += ((now - next) / every + 1) * every;
        set_num(job, "next", next);
    } else if (kind != NULL && strcmp(kind, "cron") == 0) {
        nc_cron_expr e;
        const int64_t n = nc_cron_parse(str(job, "expr"), &e) ? nc_cron_next(&e, now) : -1;
        if (n < 0) {
            cJSON_DeleteItemFromObjectCaseSensitive(job, "enabled");
            cJSON_AddBoolToObject(job, "enabled", false);
        } else {
            set_num(job, "next", n);
        }
    }
}

/* Schedule `job` from the arguments; NULL on success, else the error. */
static const char *schedule(cJSON *job, const cJSON *args, int64_t now)
{
    int64_t at = 0, every = 0;
    const bool has_at = num(args, "at_seconds", &at);
    const bool has_every = num(args, "every_seconds", &every);
    const char *expr = str(args, "cron_expr");
    const int kinds = (has_at ? 1 : 0) + (has_every ? 1 : 0) + (expr && expr[0] ? 1 : 0);
    if (kinds != 1) return "give exactly one of at_seconds, every_seconds, cron_expr";
    if (has_at) {
        if (at < 1) return "at_seconds must be at least 1";
        set_str(job, "kind", "at");
        set_num(job, "next", now + at);
    } else if (has_every) {
        if (every < NC_CRON_MIN_EVERY_S) return "every_seconds must be at least 60";
        set_str(job, "kind", "every");
        set_num(job, "every_s", every);
        set_num(job, "next", now + every);
    } else {
        nc_cron_expr e;
        if (!nc_cron_parse(expr, &e)) return "cron_expr is not a valid five-field expression";
        const int64_t next = nc_cron_next(&e, now);
        if (next < 0) return "cron_expr never matches";
        set_str(job, "kind", "cron");
        set_str(job, "expr", expr);
        set_num(job, "next", next);
    }
    return NULL;
}

static void describe(char *out, size_t cap, const cJSON *job)
{
    const char *kind = str(job, "kind");
    int64_t next = 0, every = 0;
    num(job, "next", &next);
    num(job, "every_s", &every);
    char next_text[24];
    when(next_text, sizeof next_text, next);
    char sched[96];
    if (kind && strcmp(kind, "every") == 0) {
        snprintf(sched, sizeof sched, "every %" PRId64 "s", every);
    } else if (kind && strcmp(kind, "cron") == 0) {
        snprintf(sched, sizeof sched, "%s", str(job, "expr") ? str(job, "expr") : "?");
    } else {
        snprintf(sched, sizeof sched, "one-time");
    }
    snprintf(out, cap, "- %s (id: %s, %s, %s, next %s)", str(job, "name") ? str(job, "name") : "",
             str(job, "id") ? str(job, "id") : "?", sched, enabled(job) ? "enabled" : "disabled",
             next_text);
}

char *nc_cron_tool(cJSON *jobs, const cJSON *args, int64_t now, bool *changed)
{
    *changed = false;
    const char *action = str(args, "action");
    if (action == NULL) return out_text("error: action is required");
    const char *command = str(args, "command");
    if (command != NULL && command[0] != '\0') {
        return out_text("error: NeuroClaw never runs commands; schedule a message instead");
    }

    if (strcmp(action, "list") == 0) {
        if (cJSON_GetArraySize(jobs) == 0) return out_text("No scheduled jobs.");
        size_t cap = 32 + (size_t)cJSON_GetArraySize(jobs) * 200;
        char *out = malloc(cap);
        if (out == NULL) return NULL;
        size_t n = (size_t)snprintf(out, cap, "Scheduled jobs:\n");
        const cJSON *job = NULL;
        cJSON_ArrayForEach(job, jobs)
        {
            char line[200];
            describe(line, sizeof line, job);
            n += (size_t)snprintf(out + n, cap - n, "%s\n", line);
        }
        return out;
    }

    if (strcmp(action, "add") == 0) {
        const char *message = str(args, "message");
        if (message == NULL || message[0] == '\0') return out_text("error: message is required");
        if (now < NC_CLOCK_VALID) {
            return out_text("error: the chip's clock is not set yet; try again in a minute");
        }
        if (cJSON_GetArraySize(jobs) >= NC_CRON_MAX_JOBS) {
            return out_text("error: at most %d jobs; remove one first", NC_CRON_MAX_JOBS);
        }
        int max_id = 0;
        const cJSON *j = NULL;
        cJSON_ArrayForEach(j, jobs)
        {
            const char *id = str(j, "id");
            if (id && id[0] == 'j' && atoi(id + 1) > max_id) max_id = atoi(id + 1);
        }
        cJSON *job = cJSON_CreateObject();
        char id[16];
        snprintf(id, sizeof id, "j%d", max_id + 1);
        cJSON_AddStringToObject(job, "id", id);
        const char *name = str(args, "name");
        char short_name[40];
        snprintf(short_name, sizeof short_name, "%.30s", message);
        nc_utf8_trim(short_name);
        cJSON_AddStringToObject(job, "name", name && name[0] ? name : short_name);
        cJSON_AddStringToObject(job, "message", message);
        cJSON_AddBoolToObject(job, "enabled", true);
        const char *error = schedule(job, args, now);
        if (error != NULL) {
            cJSON_Delete(job);
            return out_text("error: %s", error);
        }
        cJSON_AddItemToArray(jobs, job);
        *changed = true;
        char line[200];
        describe(line, sizeof line, job);
        return out_text("ok: added\n%s", line);
    }

    cJSON *job = find(jobs, str(args, "job_id"));
    if (job == NULL) return out_text("error: no job %s", str(args, "job_id") ? str(args, "job_id") : "(none)");
    if (strcmp(action, "get") == 0) {
        char *body = cJSON_PrintUnformatted(job);
        char *out = out_text("%s", body ? body : "{}");
        cJSON_free(body);
        return out;
    }
    if (strcmp(action, "remove") == 0) {
        cJSON_DetachItemViaPointer(jobs, job);
        cJSON_Delete(job);
        *changed = true;
        return out_text("ok: removed");
    }
    if (strcmp(action, "enable") == 0 || strcmp(action, "disable") == 0) {
        const bool on = strcmp(action, "enable") == 0;
        cJSON_DeleteItemFromObjectCaseSensitive(job, "enabled");
        cJSON_AddBoolToObject(job, "enabled", on);
        const char *kind = str(job, "kind");
        if (on && kind != NULL && strcmp(kind, "at") != 0) roll_forward(job, now);
        *changed = true;
        return out_text("ok: %sd", action);
    }
    if (strcmp(action, "update") == 0) {
        const char *name = str(args, "name");
        const char *message = str(args, "message");
        if (name && name[0]) set_str(job, "name", name);
        if (message && message[0]) set_str(job, "message", message);
        int64_t ignored;
        const bool reschedule = num(args, "at_seconds", &ignored) ||
                                num(args, "every_seconds", &ignored) ||
                                (str(args, "cron_expr") && str(args, "cron_expr")[0]);
        if (reschedule) {
            if (now < NC_CLOCK_VALID) return out_text("error: the chip's clock is not set yet");
            const char *error = schedule(job, args, now);
            if (error != NULL) return out_text("error: %s", error);
        }
        *changed = true;
        char line[200];
        describe(line, sizeof line, job);
        return out_text("ok: updated\n%s", line);
    }
    return out_text("error: unknown action %s", action);
}

cJSON *nc_cron_take_due(cJSON *jobs, int64_t now, bool *changed)
{
    *changed = false;
    cJSON *job = NULL;
    cJSON_ArrayForEach(job, jobs)
    {
        int64_t next = 0;
        if (!enabled(job) || !num(job, "next", &next) || next > now) continue;
        cJSON *due = cJSON_CreateObject();
        cJSON_AddStringToObject(due, "id", str(job, "id") ? str(job, "id") : "?");
        cJSON_AddStringToObject(due, "name", str(job, "name") ? str(job, "name") : "");
        cJSON_AddStringToObject(due, "message", str(job, "message") ? str(job, "message") : "");
        const char *kind = str(job, "kind");
        if (kind == NULL || strcmp(kind, "at") == 0) {
            cJSON_DetachItemViaPointer(jobs, job); /* one-time: removed after it ran */
            cJSON_Delete(job);
        } else {
            roll_forward(job, now);
        }
        *changed = true;
        return due;
    }
    return NULL;
}

int64_t nc_cron_seconds_to_next(const cJSON *jobs, int64_t now)
{
    int64_t best = -1;
    const cJSON *job = NULL;
    cJSON_ArrayForEach(job, jobs)
    {
        int64_t next = 0;
        if (!enabled(job) || !num(job, "next", &next)) continue;
        const int64_t wait = next > now ? next - now : 0;
        if (best < 0 || wait < best) best = wait;
    }
    return best;
}

/* --- the clock ----------------------------------------------------------------------------- */

static int64_t days_from_civil(int64_t y, int m, int d)
{
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const int64_t yoe = y - era * 400;
    const int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

int64_t nc_http_date(const char *text)
{
    static const char *const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (text == NULL) return -1;
    char wday[4], mon[4], zone[4];
    int day, year, hh, mm, ss;
    if (sscanf(text, "%3[A-Za-z], %d %3s %d %d:%d:%d %3s", wday, &day, mon, &year, &hh, &mm, &ss,
               zone) != 8) {
        return -1;
    }
    if (strcmp(zone, "GMT") != 0) return -1;
    int month = 0;
    for (int i = 0; i < 12; i++) {
        if (strcmp(mon, MONTHS[i]) == 0) month = i + 1;
    }
    if (month == 0 || day < 1 || day > 31 || year < 1970 || hh > 23 || mm > 59 || ss > 60 ||
        hh < 0 || mm < 0 || ss < 0) {
        return -1;
    }
    return days_from_civil(year, month, day) * 86400 + hh * 3600 + mm * 60 + ss;
}
