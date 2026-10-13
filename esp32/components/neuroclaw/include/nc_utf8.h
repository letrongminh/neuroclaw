/*
 * Cutting text on a chip must not cut a character: a Vietnamese letter is two or
 * three bytes, and half of one makes the whole JSON request invalid UTF-8 (the
 * LLM API then rejects it). Found by esp32/scripts/qemu_e2e.py.
 */
#ifndef NC_UTF8_H
#define NC_UTF8_H

#include <string.h>

/* Drop a trailing incomplete UTF-8 sequence from `s`, in place. */
static inline void nc_utf8_trim(char *s)
{
    size_t n = strlen(s);
    size_t i = n;
    /* Walk back over at most three continuation bytes to the sequence's lead. */
    while (i > 0 && n - i < 4 && ((unsigned char)s[i - 1] & 0xC0u) == 0x80u) i--;
    if (i == 0) return;
    const unsigned char lead = (unsigned char)s[i - 1];
    size_t need = 1;
    if ((lead & 0xE0u) == 0xC0u) {
        need = 2;
    } else if ((lead & 0xF0u) == 0xE0u) {
        need = 3;
    } else if ((lead & 0xF8u) == 0xF0u) {
        need = 4;
    } else if (lead & 0x80u) {
        s[i - 1] = '\0'; /* a stray continuation run or an invalid lead */
        return;
    }
    if (n - (i - 1) < need) s[i - 1] = '\0';
}

#endif /* NC_UTF8_H */
