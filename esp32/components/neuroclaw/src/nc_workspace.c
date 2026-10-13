/*
 * NeuroClaw-ESP's workspace (nc_workspace.h). The layout and the prompt follow
 * the Go NeuroClaw (pkg/agent/context.go, pkg/agent/memory.go).
 */
#define _POSIX_C_SOURCE 200809L /* mkdir, stat on the host */
#include "nc_workspace.h"
#include "nc_utf8.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

const char *const NC_WS_TOOLS[5] = {"read_file", "write_file", "append_file", "edit_file",
                                    "list_dir"};

static const char *const BOOTSTRAP[] = {"AGENT.md", "SOUL.md", "USER.md", "IDENTITY.md",
                                        "memory/MEMORY.md"};

/* --- paths --------------------------------------------------------------------------------- */

/* `rel` is `dir` or inside it. */
static bool under(const char *rel, const char *dir)
{
    const size_t n = strlen(dir);
    return strncmp(rel, dir, n) == 0 && (rel[n] == '/' || rel[n] == '\0');
}

bool nc_ws_path(const char *base, const char *rel, bool writable, char *out, size_t cap)
{
    if (base == NULL || rel == NULL || out == NULL || cap == 0) return false;
    out[0] = '\0';
    const size_t len = strlen(rel);
    if (len == 0 || len > NC_PATH_MAX || rel[0] == '/') return false;
    int depth = 0;
    bool segment_start = true;
    for (size_t i = 0; i < len; i++) {
        const char c = rel[i];
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-' || c == '/';
        if (!ok) return false;
        if (c == '/') {
            if (segment_start) return false; /* "//" or a leading "/" */
            segment_start = true;
            continue;
        }
        if (segment_start) {
            if (c == '.') return false; /* hidden names, "." and ".." */
            depth++;
            segment_start = false;
        }
    }
    if (depth > 4) return false;
    if (writable && (under(rel, "sessions") || under(rel, "cron"))) return false;
    const int n = snprintf(out, cap, "%s/%s", base, rel);
    if (n < 0 || (size_t)n >= cap) {
        out[0] = '\0';
        return false;
    }
    if (out[n - 1] == '/') out[n - 1] = '\0';
    return true;
}

/* --- files --------------------------------------------------------------------------------- */

char *nc_ws_slurp(const char *path, size_t max)
{
    /* As much as the file holds, not `max`: on a chip without PSRAM a 256 KB
     * buffer for a 3 KB session does not exist. */
    struct stat st;
    char tmp[200];
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        /* Power lost between the remove and the rename of nc_ws_store. */
        snprintf(tmp, sizeof tmp, "%s.tmp", path);
        if (stat(tmp, &st) != 0 || !S_ISREG(st.st_mode)) return NULL;
        path = tmp;
    }
    if ((size_t)st.st_size < max) max = (size_t)st.st_size;
    FILE *f = fopen(path, "rb");
    if (f == NULL) return NULL;
    char *buf = malloc(max + 1);
    if (buf == NULL) {
        fclose(f);
        return NULL;
    }
    const size_t n = fread(buf, 1, max, f);
    fclose(f);
    buf[n] = '\0';
    nc_utf8_trim(buf); /* a file read only in part may end mid-character */
    return buf;
}

static long file_size(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode) ? (long)st.st_size : -1;
}

static void make_parents(const char *path)
{
    char dir[192];
    snprintf(dir, sizeof dir, "%s", path);
    for (char *p = dir + 1; *p != '\0'; p++) {
        if (*p != '/') continue;
        *p = '\0';
        mkdir(dir, 0755); /* EEXIST is fine */
        *p = '/';
    }
}

int nc_ws_store(const char *path, const char *content, bool append)
{
    make_parents(path);
    if (append) {
        FILE *f = fopen(path, "ab");
        if (f == NULL) return -1;
        const size_t n = strlen(content);
        const int ok = fwrite(content, 1, n, f) == n && fflush(f) == 0;
        return fclose(f) == 0 && ok ? 0 : -1;
    }
    char tmp[200];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (f == NULL) return -1;
    const size_t n = strlen(content);
    const int ok = fwrite(content, 1, n, f) == n && fflush(f) == 0;
    if (fclose(f) != 0 || !ok) {
        remove(tmp);
        return -1;
    }
    /* LittleFS (and POSIX) replace `path` atomically: power lost at any point
     * leaves the old file or the new one. Only a VFS that refuses to rename over
     * a file gets remove-then-rename, and nc_ws_slurp then falls back to .tmp. */
    if (rename(tmp, path) == 0) return 0;
    remove(path);
    return rename(tmp, path) == 0 ? 0 : -1;
}

static char *text(const char *fmt, const char *a, const char *b)
{
    const int n = snprintf(NULL, 0, fmt, a ? a : "", b ? b : "");
    if (n < 0) return NULL;
    char *out = malloc((size_t)n + 1);
    if (out != NULL) snprintf(out, (size_t)n + 1, fmt, a ? a : "", b ? b : "");
    return out;
}

static const char *arg(const cJSON *args, const char *name)
{
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(args, name);
    return cJSON_IsString(v) ? v->valuestring : NULL;
}

static char *list_dir(const char *full, const char *rel)
{
    DIR *dir = opendir(full);
    if (dir == NULL) return text("error: no directory %s%s", rel, NULL);
    cJSON *list = cJSON_CreateArray();
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL && count < 64) {
        if (entry->d_name[0] == '.') continue;
        const size_t name_len = strlen(entry->d_name);
        if (name_len > 4 && strcmp(entry->d_name + name_len - 4, ".tmp") == 0) continue;
        char child[256];
        const int len = snprintf(child, sizeof child, "%s/%s", full, entry->d_name);
        if (len < 0 || (size_t)len >= sizeof child) continue; /* a name we could not use anyway */
        struct stat st;
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", entry->d_name);
        if (stat(child, &st) == 0 && S_ISDIR(st.st_mode)) {
            cJSON_AddStringToObject(item, "type", "dir");
        } else {
            cJSON_AddStringToObject(item, "type", "file");
            cJSON_AddNumberToObject(item, "size", stat(child, &st) == 0 ? (double)st.st_size : 0);
        }
        cJSON_AddItemToArray(list, item);
        count++;
    }
    closedir(dir);
    char *out = cJSON_PrintUnformatted(list);
    cJSON_Delete(list);
    return out;
}

static char *edit_file(const char *full, const char *rel, const char *old_text,
                       const char *new_text)
{
    if (old_text == NULL || old_text[0] == '\0' || new_text == NULL) {
        return text("error: edit_file needs old_text and new_text%s%s", NULL, NULL);
    }
    char *body = nc_ws_slurp(full, NC_FILE_MAX);
    if (body == NULL) return text("error: no file %s%s", rel, NULL);
    char *at = strstr(body, old_text);
    if (at == NULL || strstr(at + 1, old_text) != NULL) {
        free(body);
        return text(at == NULL ? "error: old_text not found in %s%s"
                               : "error: old_text appears more than once in %s%s",
                    rel, NULL);
    }
    const size_t before = (size_t)(at - body);
    const size_t old_len = strlen(old_text);
    const size_t new_len = strlen(new_text);
    const size_t total = strlen(body) - old_len + new_len;
    if (total > NC_FILE_MAX) {
        free(body);
        return text("error: %s would exceed 16 KB%s", rel, NULL);
    }
    char *out = malloc(total + 1);
    if (out == NULL) {
        free(body);
        return NULL;
    }
    memcpy(out, body, before);
    memcpy(out + before, new_text, new_len);
    memcpy(out + before + new_len, at + old_len, strlen(at + old_len) + 1);
    free(body);
    const int err = nc_ws_store(full, out, false);
    free(out);
    return err == 0 ? text("ok: edited %s%s", rel, NULL) : text("error: cannot write %s%s", rel, NULL);
}

char *nc_ws_tool(const char *base, const char *name, const cJSON *args)
{
    if (name == NULL) return NULL;
    const bool read = strcmp(name, "read_file") == 0;
    const bool write = strcmp(name, "write_file") == 0;
    const bool append = strcmp(name, "append_file") == 0;
    const bool edit = strcmp(name, "edit_file") == 0;
    const bool list = strcmp(name, "list_dir") == 0;
    if (!(read || write || append || edit || list)) return NULL;

    const char *rel = arg(args, "path");
    if (list && (rel == NULL || rel[0] == '\0' || strcmp(rel, ".") == 0)) {
        return list_dir(base, ".");
    }
    char full[192];
    if (!nc_ws_path(base, rel, !(read || list), full, sizeof full)) {
        return text("error: %s is not a workspace path you may use (relative, no .., not "
                    "sessions/ or cron/)%s",
                    rel ? rel : "(none)", NULL);
    }
    if (list) return list_dir(full, rel);
    if (read) {
        const long size = file_size(full);
        if (size < 0) return text("error: no file %s%s", rel, NULL);
        char *body = nc_ws_slurp(full, NC_FILE_MAX);
        return body ? body : text("error: cannot read %s%s", rel, NULL);
    }
    if (edit) return edit_file(full, rel, arg(args, "old_text"), arg(args, "new_text"));

    const char *content = arg(args, "content");
    if (content == NULL) return text("error: %s needs content%s", name, NULL);
    const long size = append ? file_size(full) : 0;
    if ((size_t)(size > 0 ? size : 0) + strlen(content) > NC_FILE_MAX) {
        return text("error: %s would exceed 16 KB%s", rel, NULL);
    }
    if (nc_ws_store(full, content, append) != 0) return text("error: cannot write %s%s", rel, NULL);
    return text(append ? "ok: appended to %s%s" : "ok: wrote %s%s", rel, NULL);
}

const char *nc_ws_tool_schema(const char *name)
{
    if (name == NULL) return NULL;
    if (strcmp(name, "read_file") == 0 || strcmp(name, "list_dir") == 0) {
        return "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\","
               "\"description\":\"Path relative to the workspace\"}},\"required\":[\"path\"]}";
    }
    if (strcmp(name, "write_file") == 0 || strcmp(name, "append_file") == 0) {
        return "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},"
               "\"content\":{\"type\":\"string\"}},\"required\":[\"path\",\"content\"]}";
    }
    if (strcmp(name, "edit_file") == 0) {
        return "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},"
               "\"old_text\":{\"type\":\"string\",\"description\":\"Exact text to replace; must "
               "appear once\"},\"new_text\":{\"type\":\"string\"}},"
               "\"required\":[\"path\",\"old_text\",\"new_text\"]}";
    }
    return NULL;
}

const char *nc_ws_tool_description(const char *name)
{
    if (name == NULL) return NULL;
    if (strcmp(name, "read_file") == 0) return "Read a file of your workspace.";
    if (strcmp(name, "write_file") == 0) return "Create or replace a file of your workspace.";
    if (strcmp(name, "append_file") == 0) return "Append text to a file of your workspace.";
    if (strcmp(name, "edit_file") == 0) {
        return "Replace one exact passage of a workspace file (e.g. update memory/MEMORY.md).";
    }
    if (strcmp(name, "list_dir") == 0) return "List a directory of your workspace.";
    return NULL;
}

/* --- the prompt ---------------------------------------------------------------------------- */

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} nc_text;

static void add(nc_text *t, const char *s)
{
    if (s == NULL || t->data == NULL) return;
    const size_t n = strlen(s);
    if (t->len + n + 1 > t->cap) {
        size_t cap = t->cap * 2;
        while (cap < t->len + n + 1) cap *= 2;
        char *grown = realloc(t->data, cap);
        if (grown == NULL) return;
        t->data = grown;
        t->cap = cap;
    }
    memcpy(t->data + t->len, s, n + 1);
    t->len += n;
}

/* `key: value` of a SKILL.md front matter line, quotes stripped, into `out`. */
static bool front_matter(const char *body, const char *key, char *out, size_t cap)
{
    const size_t klen = strlen(key);
    for (const char *line = body; line != NULL && *line != '\0';) {
        if (strncmp(line, key, klen) == 0 && line[klen] == ':') {
            const char *v = line + klen + 1;
            while (*v == ' ') v++;
            if (*v == '"') v++;
            size_t n = 0;
            while (v[n] != '\0' && v[n] != '\n' && v[n] != '\r' && n + 1 < cap) n++;
            if (n > 0 && v[n - 1] == '"') n--;
            memcpy(out, v, n);
            out[n] = '\0';
            nc_utf8_trim(out);
            return n > 0;
        }
        const char *end = strchr(line, '\n');
        line = end ? end + 1 : NULL;
    }
    return false;
}

/* The Go NeuroClaw's skills summary: one line per skills/<dir>/SKILL.md, the
 * model reads the file itself when it needs it. */
static void add_skills(nc_text *t, const char *base)
{
    char dir_path[192];
    snprintf(dir_path, sizeof dir_path, "%s/skills", base);
    DIR *dir = opendir(dir_path);
    if (dir == NULL) return;
    bool any = false;
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL && count < 16) {
        if (entry->d_name[0] == '.' || strlen(entry->d_name) > 48) continue;
        char path[256];
        const int len = snprintf(path, sizeof path, "%s/%s/SKILL.md", dir_path, entry->d_name);
        if (len < 0 || (size_t)len >= sizeof path) continue;
        char *body = nc_ws_slurp(path, 1024);
        if (body == NULL) continue;
        char name[64], description[240];
        if (!front_matter(body, "name", name, sizeof name)) {
            memcpy(name, entry->d_name, strlen(entry->d_name) + 1); /* <= 48, checked above */
        }
        if (!front_matter(body, "description", description, sizeof description)) {
            description[0] = '\0';
        }
        free(body);
        if (!any) {
            add(t, "\n## Skills\nRead a skill's file with read_file before using it.\n");
            any = true;
        }
        char line[512];
        const int w = snprintf(line, sizeof line, "- %s: %s (skills/%s/SKILL.md)\n", name,
                               description, entry->d_name);
        if (w < 0 || (size_t)w >= sizeof line) continue;
        add(t, line);
        count++;
    }
    closedir(dir);
}

char *nc_system_prompt(const char *base, const char *base_prompt, const char *now_text)
{
    nc_text t = {malloc(2048), 0, 2048};
    if (t.data == NULL) return NULL;
    t.data[0] = '\0';
    add(&t, base_prompt);
    add(&t, "\n\n## Workspace\n"
            "Your workspace is on this chip's flash and survives reboots. Use read_file, "
            "write_file, append_file, edit_file and list_dir on it.\n"
            "- Memory: memory/MEMORY.md — when something seems memorable, update it.\n"
            "- Daily notes: memory/YYYYMM/YYYYMMDD.md\n"
            "- Who you are and who the owner is: AGENT.md, SOUL.md, USER.md, IDENTITY.md\n"
            "- Periodic checks: HEARTBEAT.md\n"
            "- Skills: skills/{skill-name}/SKILL.md\n"
            "- Reminders and scheduled tasks: the cron tool.\n");
    if (now_text != NULL && now_text[0] != '\0') {
        add(&t, "\nCurrent time: ");
        add(&t, now_text);
        add(&t, "\n");
    }
    add_skills(&t, base);
    char summary_path[192];
    snprintf(summary_path, sizeof summary_path, "%s/sessions/summary.md", base);
    char *summary = nc_ws_slurp(summary_path, NC_BOOTSTRAP_MAX);
    if (summary != NULL && summary[0] != '\0') {
        /* The Go NeuroClaw's words for it (pkg/agent/context.go). */
        add(&t, "\nCONTEXT_SUMMARY: The following is an approximate summary of prior "
                "conversation for reference only. It may be incomplete or outdated — always "
                "defer to explicit instructions.\n\n");
        add(&t, summary);
        add(&t, "\n");
    }
    free(summary);
    for (size_t i = 0; i < sizeof BOOTSTRAP / sizeof BOOTSTRAP[0]; i++) {
        char path[192];
        snprintf(path, sizeof path, "%s/%s", base, BOOTSTRAP[i]);
        char *body = nc_ws_slurp(path, NC_BOOTSTRAP_MAX);
        if (body == NULL) continue;
        if (body[0] != '\0') {
            add(&t, "\n## ");
            add(&t, BOOTSTRAP[i]);
            add(&t, "\n\n");
            add(&t, body);
            add(&t, "\n");
        }
        free(body);
    }
    return t.data;
}

char *nc_heartbeat_prompt(const char *base, const char *now_text)
{
    char path[192];
    snprintf(path, sizeof path, "%s/HEARTBEAT.md", base);
    char *tasks = nc_ws_slurp(path, NC_BOOTSTRAP_MAX);
    if (tasks == NULL) return NULL;
    bool has_task = false;
    for (const char *line = tasks; *line != '\0' && !has_task;) {
        while (*line == ' ' || *line == '\t') line++;
        if (*line != '\n' && *line != '\r' && *line != '#' && *line != '\0') has_task = true;
        const char *end = strchr(line, '\n');
        line = end ? end + 1 : line + strlen(line);
    }
    if (!has_task) {
        free(tasks);
        return NULL;
    }
    nc_text t = {malloc(1024), 0, 1024};
    if (t.data == NULL) {
        free(tasks);
        return NULL;
    }
    t.data[0] = '\0';
    add(&t, "# Heartbeat Check\n\nCurrent time: ");
    add(&t, now_text && now_text[0] ? now_text : "unknown");
    add(&t, "\n\nYou are a proactive AI assistant. This is a scheduled heartbeat check.\n"
            "Review the following tasks and execute any necessary actions using available "
            "tools.\nIf there is nothing that requires attention, respond ONLY with: "
            "HEARTBEAT_OK\n\n");
    add(&t, tasks);
    free(tasks);
    return t.data;
}

/* --- the session --------------------------------------------------------------------------- */

int nc_session_save(const char *base, const cJSON *messages)
{
    char *body = cJSON_PrintUnformatted(messages);
    if (body == NULL) return -1;
    char path[192];
    snprintf(path, sizeof path, "%s/sessions/telegram.json", base);
    const int err = nc_ws_store(path, body, false);
    cJSON_free(body);
    return err;
}

cJSON *nc_session_load(const char *base)
{
    char path[192];
    snprintf(path, sizeof path, "%s/sessions/telegram.json", base);
    char *body = nc_ws_slurp(path, 256u * 1024u);
    if (body == NULL) return NULL;
    cJSON *messages = cJSON_Parse(body);
    free(body);
    if (!cJSON_IsArray(messages)) {
        cJSON_Delete(messages);
        return NULL;
    }
    return messages;
}
