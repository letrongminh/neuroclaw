/*
 * NeuroClaw-ESP's workspace: the same files as the Go NeuroClaw's, on the chip's
 * flash (a LittleFS mount the firmware provides), so the brain remembers the
 * same way on a chip, a Linux board or the cloud.
 *
 *   AGENT.md SOUL.md USER.md IDENTITY.md  who it is, who the owner is (prompt)
 *   memory/MEMORY.md                      long-term memory (prompt)
 *   memory/YYYYMM/YYYYMMDD.md             daily notes
 *   HEARTBEAT.md                          what to check periodically
 *   sessions/telegram.json                the conversation, across reboots
 *   sessions/summary.md                   what no longer fits in it, summarized
 *   cron/jobs.json                        scheduled tasks (nc_cron.h)
 *
 * The model reads and writes the workspace with read_file, write_file,
 * append_file, edit_file and list_dir, like the Go tools of the same names but
 * confined: relative paths only, no `..`, no hidden names, at most
 * NC_FILE_MAX bytes a file, and sessions/ and cron/ are the brain's own.
 *
 * Plain POSIX C over stdio and dirent: compiled and tested on the host too.
 */
#ifndef NC_WORKSPACE_H
#define NC_WORKSPACE_H

#include <stdbool.h>
#include <stddef.h>

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NC_FILE_MAX (16u * 1024u)
#define NC_PATH_MAX 96u
#define NC_BOOTSTRAP_MAX 4096u /* each prompt file, as much as is read of it */

/* `rel` is a workspace path the model may use; `writable` excludes the brain's
 * own sessions/ and cron/. Writes the full path into `out`. */
bool nc_ws_path(const char *base, const char *rel, bool writable, char *out, size_t cap);

/* The whole file (at most `max` bytes, NUL-terminated, malloc'd), or NULL. */
char *nc_ws_slurp(const char *path, size_t max);

/* Write `content` to `path` through a temporary file and a rename, creating
 * parent directories. 0 on success. */
int nc_ws_store(const char *path, const char *content, bool append);

/*
 * One workspace tool call: read_file, write_file, append_file, edit_file,
 * list_dir. Returns the text for the model (malloc'd): the file, "ok: ...",
 * or "error: ...". NULL when `name` is not a workspace tool.
 */
char *nc_ws_tool(const char *base, const char *name, const cJSON *args);

/* The JSON Schema of each workspace tool, by name; NULL for any other name. */
const char *nc_ws_tool_schema(const char *name);
const char *nc_ws_tool_description(const char *name);
extern const char *const NC_WS_TOOLS[5];

/*
 * The system prompt: `base_prompt`, the workspace layout, the current time
 * (`now_text`, "" when the clock is not set), the CONTEXT_SUMMARY of older
 * conversation (sessions/summary.md) if any, then each of AGENT.md, SOUL.md,
 * USER.md, IDENTITY.md and memory/MEMORY.md that exists, under its own
 * heading. malloc'd.
 */
char *nc_system_prompt(const char *base, const char *base_prompt, const char *now_text);

/* The heartbeat prompt of the Go NeuroClaw (pkg/heartbeat/service.go) with
 * HEARTBEAT.md's tasks, or NULL when the file is missing or has no task (only
 * headings and blank lines). malloc'd. */
char *nc_heartbeat_prompt(const char *base, const char *now_text);

/* The conversation, saved after every turn and read back at boot. A file that
 * does not parse is ignored (the brain starts a new conversation). */
int nc_session_save(const char *base, const cJSON *messages);
cJSON *nc_session_load(const char *base);

#ifdef __cplusplus
}
#endif

#endif /* NC_WORKSPACE_H */
