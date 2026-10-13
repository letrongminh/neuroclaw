/*
 * NeuroClaw-ESP's serial commands (neuroclaw_console_register), after
 * MimiClaw's: the owner at the console reads and writes the memory, lists and
 * clears the conversation, starts a heartbeat or a job, and reads the chip's
 * health. The app owns esp_console (init, the REPL or its own line reader);
 * this file is linked only when the app calls neuroclaw_console_register.
 *
 * Every reply is one line starting `NC_CONSOLE <command>`, except memory_read,
 * which prints the memory first: it is what the owner asked to see.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "neuroclaw.h"

static int cmd_memory_read(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char *text = neuroclaw_memory_read();
    if (text != NULL) printf("%s%s", text, text[0] && text[strlen(text) - 1] != '\n' ? "\n" : "");
    printf("NC_CONSOLE memory_read bytes=%u\n", text ? (unsigned)strlen(text) : 0u);
    free(text);
    return 0;
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
    if (text == NULL) return 1;
    text[0] = '\0';
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat(text, " ");
        strcat(text, argv[i]);
    }
    strcat(text, "\n");
    const int err = neuroclaw_memory_write(text);
    printf("NC_CONSOLE memory_write bytes=%u result=%s\n", (unsigned)strlen(text),
           err == ESP_OK ? "ok" : esp_err_to_name(err));
    free(text);
    return err == ESP_OK ? 0 : 1;
}

static int cmd_session_list(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char out[96];
    const int err = neuroclaw_session_list(out, sizeof out);
    printf("NC_CONSOLE session_list %s\n", err == ESP_OK ? out : esp_err_to_name(err));
    return err == ESP_OK ? 0 : 1;
}

static int cmd_session_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    const int err = neuroclaw_session_clear();
    printf("NC_CONSOLE session_clear result=%s\n", err == ESP_OK ? "ok" : esp_err_to_name(err));
    return err == ESP_OK ? 0 : 1;
}

static int cmd_heartbeat_trigger(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    neuroclaw_heartbeat_trigger();
    printf("NC_CONSOLE heartbeat_trigger result=queued\n");
    return 0;
}

static int cmd_cron_start(int argc, char **argv)
{
    neuroclaw_cron_start(argc > 1 ? argv[1] : NULL);
    printf("NC_CONSOLE cron_start job=%s result=queued\n", argc > 1 ? argv[1] : "-");
    return 0;
}

static int cmd_diag(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char *text = neuroclaw_diagnostics();
    printf("NC_CONSOLE diag %s\n", text ? text : "{}");
    free(text);
    return 0;
}

static int cmd_llm_limit(int argc, char **argv)
{
    if (argc != 3) {
        printf("NC_CONSOLE llm_limit result=usage: llm_limit <per_hour> <per_day> (0 = none)\n");
        return 1;
    }
    neuroclaw_set_llm_limits(atoi(argv[1]), atoi(argv[2]));
    printf("NC_CONSOLE llm_limit result=ok\n");
    return 0;
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
