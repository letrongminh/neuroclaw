/*
 * NeuroClaw-ESP's scheduled tasks: the Go NeuroClaw's `cron` tool on a chip.
 * Same arguments (action add/list/get/update/remove/enable/disable, message,
 * name, at_seconds, every_seconds, cron_expr, job_id), same behaviour: a job
 * that comes due runs the agent with its message and the reply goes to the
 * owner; a one-time job is removed after it ran. `command` is refused, as the
 * Go NeuroClaw's lockdown refuses it: a scheduled job is a message to the
 * brain, never a command, and any device action it leads to goes through the
 * gate like any other.
 *
 * Jobs live in cron/jobs.json of the workspace. Times are the chip's clock in
 * its TZ; nothing is scheduled before the clock is set (NC_CLOCK_VALID).
 * Plain C99 + cJSON + <time.h>: tested on the host.
 */
#ifndef NC_CRON_H
#define NC_CRON_H

#include <stdbool.h>
#include <stdint.h>

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NC_CLOCK_VALID INT64_C(1735689600) /* 2025-01-01T00:00:00Z: earlier is "not set" */
#define NC_CRON_MAX_JOBS 16
#define NC_CRON_MIN_EVERY_S 60

typedef struct {
    uint64_t minutes;  /* bit 0..59 */
    uint32_t hours;    /* bit 0..23 */
    uint32_t days;     /* bit 1..31 */
    uint16_t months;   /* bit 1..12 */
    uint8_t weekdays;  /* bit 0..6, Sunday 0 */
    bool any_day;      /* day-of-month was `*` */
    bool any_weekday;  /* day-of-week was `*` */
} nc_cron_expr;

/* Five fields: minute hour day-of-month month day-of-week; each `*`, a number,
 * a range a-b, a list, and an optional /step. Day of week 0-7 (0 and 7 are
 * Sunday). When both day fields are restricted, either may match (cron's rule). */
bool nc_cron_parse(const char *expr, nc_cron_expr *out);

/* The first minute after `after` that matches, in local time; -1 when none in
 * five years. */
int64_t nc_cron_next(const nc_cron_expr *expr, int64_t after);

/* One `cron` tool call on `jobs` (an array, changed in place; `*changed` says
 * whether it must be saved). The text for the model, malloc'd. */
char *nc_cron_tool(cJSON *jobs, const cJSON *args, int64_t now, bool *changed);

/* The first enabled job that is due at `now`, detached as {id,name,message}
 * (the caller deletes it), with the job rescheduled or, if one-time, removed.
 * NULL when none is due. */
cJSON *nc_cron_take_due(cJSON *jobs, int64_t now, bool *changed);

/* Seconds until the next enabled job is due (0 when one is due), -1 when none. */
int64_t nc_cron_seconds_to_next(const cJSON *jobs, int64_t now);

/* An HTTP Date header ("Tue, 13 Oct 2026 05:45:00 GMT") as epoch seconds, -1
 * when it does not parse. The brain sets the clock from Telegram's (TLS) reply
 * when nothing else has. */
int64_t nc_http_date(const char *text);

#ifdef __cplusplus
}
#endif

#endif /* NC_CRON_H */
