/*
 * NeuroClaw-ESP's `message` and `send_file` tools, the parts without ESP-IDF
 * state, as the Go NeuroClaw's (pkg/tools/integration/message.go,
 * pkg/tools/fs/send_file.go): the model sends the owner a message, or a file
 * of the workspace, itself — what lets a reminder or the heartbeat say more
 * than one reply, or deliver a note.
 *
 * The chip talks only to its owner: a `chat_id` other than the owner's, or a
 * channel other than Telegram, is refused before anything is sent.
 *
 * A file goes to Telegram as multipart/form-data streamed from flash, so its
 * size is not bounded by RAM: images as sendPhoto, the rest as sendDocument.
 *
 * Plain C (+ cJSON): tested on the host.
 */
#ifndef NC_SEND_H
#define NC_SEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NC_MESSAGE_MAX 4096 /* Telegram's limit, in characters */

typedef struct {
    char *content;    /* malloc'd */
    int64_t reply_to; /* a message id, 0 for none */
} nc_message;

/* The `message` tool's arguments: content (required, at most NC_MESSAGE_MAX
 * characters), chat_id (the owner's, as text or a number), channel
 * ("telegram"), reply_to_message_id. 0 and `out` filled, or -1 and `err` says
 * why ("error: …"). */
int nc_message_args(const cJSON *args, int64_t owner_chat, nc_message *out, char *err, size_t cap);
void nc_message_free(nc_message *message);

/* sendMessage body, with reply_parameters when `reply_to` is set. malloc'd. */
char *nc_message_body(int64_t chat_id, const char *text, int64_t reply_to);

typedef struct {
    char path[192];    /* full path on the workspace */
    char filename[64]; /* what the owner sees */
    bool photo;        /* .jpg, .jpeg, .png, .webp */
} nc_file;

/* The `send_file` tool's arguments: path (a workspace path, read rules of the
 * file tools), filename (default: the path's last part; characters other than
 * letters, digits, '.', '-', '_' and ' ' become '_'). 0 or -1 with `err`. */
int nc_send_file_args(const char *workspace, const cJSON *args, nc_file *out, char *err,
                      size_t cap);

/* "sendPhoto" or "sendDocument". */
const char *nc_file_method(const nc_file *file);

/* The multipart parts around the file's bytes. Each returns the length written
 * (NUL not counted), or -1 when `cap` is too small. */
int nc_multipart_head(char *out, size_t cap, const char *boundary, int64_t chat_id,
                      const nc_file *file);
int nc_multipart_tail(char *out, size_t cap, const char *boundary);

#ifdef __cplusplus
}
#endif

#endif /* NC_SEND_H */
