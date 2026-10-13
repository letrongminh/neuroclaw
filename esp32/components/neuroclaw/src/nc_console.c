/*
 * NeuroClaw-ESP's serial commands (neuroclaw_console_register), after
 * MimiClaw's: the owner at the console reads and writes the memory, lists and
 * clears the conversation, starts a heartbeat or a job, and reads the chip's
 * health. The app owns esp_console (init, the REPL or its own line reader);
 * this file is linked only when the app calls neuroclaw_console_register.
 *
 * Every reply is one line `NC_CONSOLE <command> ... result=<r>` (diag on success:
 * the JSON instead), where busy means the brain stayed in a turn and nothing was
 * done, empty that there is no memory yet. memory_read prints the memory first:
 * it is what the owner asked to see.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "neuroclaw.h"

/* What a reply says about an API result: ok, busy (the brain stayed in a turn;
 * nothing was done), not_started, or the ESP-IDF name. */
static const char *result_name(int err)
{
    switch (err) {
    case ESP_OK:
        return "ok";
    case ESP_ERR_TIMEOUT:
        return "busy";
    case ESP_ERR_INVALID_STATE:
        return "not_started";
    case ESP_ERR_NOT_FOUND:
        return "empty";
    default:
        return esp_err_to_name(err);
    }
}

static int cmd_memory_read(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char *text = NULL;
    const int err = neuroclaw_memory_read(&text);
    if (err == ESP_OK) {
        const size_t n = strlen(text);
        printf("%s%s", text, n > 0 && text[n - 1] != '\n' ? "\n" : "");
        printf("NC_CONSOLE memory_read bytes=%u result=ok\n", (unsigned)n);
    } else {
        printf("NC_CONSOLE memory_read result=%s\n", result_name(err));
    }
    free(text);
    return err == ESP_OK ? 0 : 1;
}

/* memory_write <text...>: the words joined by spaces replace MEMORY.md. */
static int cmd_memory_write(int argc, char **argv)
{
    if (argc < 2) {
        printf("NC_CONSOLE memory_write result=usage: memory_write <text>\n");
        return 1;
    }
    size_t len = 2;
    for (int i = 1; i < argc; i++) len += strlen(argv[i]) + 1;
    char *text = malloc(len);
    if (text == NULL) {
        printf("NC_CONSOLE memory_write result=%s\n", result_name(ESP_ERR_NO_MEM));
        return 1;
    }
    text[0] = '\0';
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat(text, " ");
        strcat(text, argv[i]);
    }
    strcat(text, "\n");
    const int err = neuroclaw_memory_write(text);
    printf("NC_CONSOLE memory_write bytes=%u result=%s\n", (unsigned)strlen(text),
           result_name(err));
    free(text);
    return err == ESP_OK ? 0 : 1;
}

static int cmd_session_list(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char out[96];
    const int err = neuroclaw_session_list(out, sizeof out);
    printf("NC_CONSOLE session_list %s%sresult=%s\n", err == ESP_OK ? out : "",
           err == ESP_OK ? " " : "", result_name(err));
    return err == ESP_OK ? 0 : 1;
}

static int cmd_session_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    const int err = neuroclaw_session_clear();
    printf("NC_CONSOLE session_clear result=%s\n", result_name(err));
    return err == ESP_OK ? 0 : 1;
}

static int cmd_heartbeat_trigger(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    const int err = neuroclaw_heartbeat_trigger();
    printf("NC_CONSOLE heartbeat_trigger result=%s\n", err == ESP_OK ? "queued" : result_name(err));
    return err == ESP_OK ? 0 : 1;
}

static int cmd_cron_start(int argc, char **argv)
{
    const int err = neuroclaw_cron_start(argc > 1 ? argv[1] : NULL);
    printf("NC_CONSOLE cron_start result=%s\n",
           err == ESP_OK                ? "queued"
           : err == ESP_ERR_INVALID_ARG ? "bad_job_id"
                                        : result_name(err));
    return err == ESP_OK ? 0 : 1;
}

static int cmd_diag(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char *text = NULL;
    const int err = neuroclaw_diagnostics(&text);
    if (err == ESP_OK) {
        printf("NC_CONSOLE diag %s\n", text);
    } else {
        printf("NC_CONSOLE diag result=%s\n", result_name(err));
    }
    free(text);
    return err == ESP_OK ? 0 : 1;
}

static int cmd_llm_limit(int argc, char **argv)
{
    if (argc != 3) {
        printf("NC_CONSOLE llm_limit result=usage: llm_limit <per_hour> <per_day> (0 = none)\n");
        return 1;
    }
    const int err = neuroclaw_set_llm_limits(atoi(argv[1]), atoi(argv[2]));
    printf("NC_CONSOLE llm_limit result=%s\n", result_name(err));
    return err == ESP_OK ? 0 : 1;
}

int neuroclaw_console_register(void)
{
    static const esp_console_cmd_t cmds[] = {
        {.command = "memory_read", .help = "Print memory/MEMORY.md", .func = cmd_memory_read},
        {.command = "memory_write",
         .help = "Replace memory/MEMORY.md with the text",
         .hint = "<text>",
         .func = cmd_memory_write},
        {.command = "session_list",
         .help = "Messages kept, summary size, scheduled jobs",
         .func = cmd_session_list},
        {.command = "session_clear",
         .help = "Forget the conversation and its summary (memory stays)",
         .func = cmd_session_clear},
        {.command = "heartbeat_trigger",
         .help = "Check HEARTBEAT.md now",
         .func = cmd_heartbeat_trigger},
        {.command = "cron_start",
         .help = "Run a scheduled job now, its schedule unchanged",
         .hint = "[job_id]",
         .func = cmd_cron_start},
        {.command = "diag", .help = "The chip's health (get_diagnostics)", .func = cmd_diag},
        {.command = "llm_limit",
         .help = "LLM calls per hour and per day, 0 = no limit",
         .hint = "<per_hour> <per_day>",
         .func = cmd_llm_limit},
    };
    for (size_t i = 0; i < sizeof cmds / sizeof cmds[0]; i++) {
        const esp_err_t err = esp_console_cmd_register(&cmds[i]);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}
