/*
 * NeuroClaw-ESP's web tools, the parts without ESP-IDF state (nc_web.h). The
 * address rule follows the Go NeuroClaw's isPrivateOrRestrictedIP.
 */
#define _POSIX_C_SOURCE 200809L
#include "nc_web.h"

#ifdef ESP_PLATFORM
#include "lwip/inet.h"
#include "lwip/sockets.h"
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "nc_utf8.h"

/* --- addresses ----------------------------------------------------------------------------- */

static bool v4_private(const uint8_t a[4])
{
    return a[0] == 0 || a[0] == 10 || a[0] == 127 ||          /* this host, RFC 1918, loopback */
           (a[0] == 169 && a[1] == 254) ||                    /* link-local */
           (a[0] == 172 && (a[1] & 0xF0) == 16) ||            /* RFC 1918 */
           (a[0] == 192 && a[1] == 168) ||                    /* RFC 1918 */
           (a[0] == 100 && (a[1] & 0xC0) == 64) ||            /* CGNAT */
           (a[0] == 192 && a[1] == 0 && a[2] == 0) ||         /* IETF */
           (a[0] == 198 && (a[1] & 0xFE) == 18) ||            /* benchmarking */
           a[0] >= 224;                                       /* multicast, reserved */
}

bool nc_ip_is_private(const char *address)
{
    if (address == NULL) return true;
    uint8_t v4[4];
    if (inet_pton(AF_INET, address, v4) == 1) return v4_private(v4);
    uint8_t v6[16];
    if (inet_pton(AF_INET6, address, v6) != 1) return true;
    static const uint8_t zero[16] = {0};
    if (memcmp(v6, zero, 15) == 0 && (v6[15] == 0 || v6[15] == 1)) return true; /* ::, ::1 */
    if ((v6[0] & 0xFE) == 0xFC) return true;                  /* ULA fc00::/7 */
    if (v6[0] == 0xFE && (v6[1] & 0xC0) == 0x80) return true; /* link-local fe80::/10 */
    if (v6[0] == 0xFF) return true;                           /* multicast */
    /* IPv4-mapped ::ffff:a.b.c.d */
    static const uint8_t mapped[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};
    if (memcmp(v6, mapped, 12) == 0) return v4_private(v6 + 12);
    return false;
}

bool nc_url_parts(const char *url, bool *https, char *host, size_t host_cap, int *port)
{
    if (url == NULL || host == NULL || host_cap == 0) return false;
    const char *p;
    if (strncasecmp(url, "https://", 8) == 0) {
        *https = true;
        p = url + 8;
        *port = 443;
    } else if (strncasecmp(url, "http://", 7) == 0) {
        *https = false;
        p = url + 7;
        *port = 80;
    } else {
        return false;
    }
    const size_t authority = strcspn(p, "/?#");
    if (authority == 0 || memchr(p, '@', authority) != NULL) return false;
    const char *end = p + authority;
    const char *host_start = p;
    const char *host_end = end;
    if (*p == '[') { /* [v6]:port */
        const char *close = memchr(p, ']', authority);
        if (close == NULL) return false;
        host_start = p + 1;
        host_end = close;
        if (close + 1 < end) {
            if (close[1] != ':') return false;
            *port = atoi(close + 2);
        }
    } else {
        const char *colon = memchr(p, ':', authority);
        if (colon != NULL) {
            host_end = colon;
            *port = atoi(colon + 1);
        }
    }
    const size_t n = (size_t)(host_end - host_start);
    if (n == 0 || n >= host_cap || *port <= 0 || *port > 65535) return false;
    memcpy(host, host_start, n);
    host[n] = '\0';
    return true;
}

bool nc_host_allowed(const char *host, const char *allow)
{
    if (host == NULL || allow == NULL) return false;
    const size_t n = strlen(host);
    for (const char *p = allow; *p != '\0';) {
        while (*p == ' ' || *p == ',') p++;
        const size_t len = strcspn(p, ", ");
        if (len == n && len > 0 && strncasecmp(p, host, n) == 0) return true;
        p += len;
    }
    return false;
}

/* --- search -------------------------------------------------------------------------------- */

char *nc_url_encode(const char *text)
{
    static const char hex[] = "0123456789ABCDEF";
    const size_t n = text ? strlen(text) : 0;
    char *out = malloc(n * 3 + 1);
    if (out == NULL) return NULL;
    size_t o = 0;
    for (size_t i = 0; i < n; i++) {
        const unsigned char c = (unsigned char)text[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out[o++] = (char)c;
        } else {
            out[o++] = '%';
            out[o++] = hex[c >> 4];
            out[o++] = hex[c & 15];
        }
    }
    out[o] = '\0';
    return out;
}

char *nc_search_url(const char *provider, const char *base_url, const char *query, int count,
                    const char *range)
{
    if (provider == NULL || query == NULL) return NULL;
    char *q = nc_url_encode(query);
    if (q == NULL) return NULL;
    char buf[512];
    int n = -1;
    if (strcmp(provider, "searxng") == 0 && base_url != NULL && base_url[0] != '\0') {
        const char *time = range == NULL           ? NULL
                           : strcmp(range, "d") == 0 ? "day"
                           : strcmp(range, "w") == 0 ? "week"
                           : strcmp(range, "m") == 0 ? "month"
                           : strcmp(range, "y") == 0 ? "year"
                                                     : NULL;
        n = snprintf(buf, sizeof buf, "%s/search?q=%s&format=json%s%s", base_url, q,
                     time ? "&time_range=" : "", time ? time : "");
    } else if (strcmp(provider, "brave") == 0) {
        const char *fresh = range == NULL           ? NULL
                            : strcmp(range, "d") == 0 ? "pd"
                            : strcmp(range, "w") == 0 ? "pw"
                            : strcmp(range, "m") == 0 ? "pm"
                            : strcmp(range, "y") == 0 ? "py"
                                                      : NULL;
        n = snprintf(buf, sizeof buf,
                     "https://api.search.brave.com/res/v1/web/search?q=%s&count=%d%s%s", q, count,
                     fresh ? "&freshness=" : "", fresh ? fresh : "");
    }
    free(q);
    if (n < 0 || (size_t)n >= sizeof buf) return NULL;
    return strdup(buf);
}

static char *dup_printf_result(char *buf, size_t n)
{
    buf[n] = '\0';
    nc_utf8_trim(buf);
    return buf;
}

char *nc_search_text(const char *provider, const char *json, const char *query, int count)
{
    const size_t cap = 4096;
    char *out = malloc(cap + 1);
    if (out == NULL) return NULL;
    cJSON *root = cJSON_Parse(json);
    const cJSON *list = NULL;
    const char *snippet_key = "content";
    const char *label = "SearXNG";
    if (provider != NULL && strcmp(provider, "brave") == 0) {
        list = cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(root, "web"),
                                                "results");
        snippet_key = "description";
        label = "Brave";
    } else {
        list = cJSON_GetObjectItemCaseSensitive(root, "results");
    }
    if (!cJSON_IsArray(list)) {
        cJSON_Delete(root);
        snprintf(out, cap, "error: the search provider's reply could not be read");
        return out;
    }
    size_t n = (size_t)snprintf(out, cap, "Results for: %s (via %s)", query ? query : "", label);
    int i = 0;
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, list)
    {
        if (i >= count || n >= cap) break;
        const cJSON *title = cJSON_GetObjectItemCaseSensitive(item, "title");
        const cJSON *url = cJSON_GetObjectItemCaseSensitive(item, "url");
        const cJSON *snippet = cJSON_GetObjectItemCaseSensitive(item, snippet_key);
        if (!cJSON_IsString(url)) continue;
        char *clean = cJSON_IsString(snippet) ? nc_html_text(snippet->valuestring, 300) : NULL;
        char *clean_title = cJSON_IsString(title) ? nc_html_text(title->valuestring, 160) : NULL;
        const int w = snprintf(out + n, cap - n, "\n%d. %s\n   %s%s%s", i + 1,
                               clean_title ? clean_title : "", url->valuestring,
                               clean && clean[0] ? "\n   " : "", clean ? clean : "");
        free(clean);
        free(clean_title);
        if (w < 0) break;
        n = (size_t)w + n > cap ? cap : n + (size_t)w;
        i++;
    }
    if (i == 0 && n < cap) n += (size_t)snprintf(out + n, cap - n, "\nNo results.");
    cJSON_Delete(root);
    return dup_printf_result(out, n > cap ? cap : n);
}

/* --- HTML to text -------------------------------------------------------------------------- */

/* strcasestr is not in C99 or newlib's default set. */
static const char *strcasestr_compat(const char *haystack, const char *needle)
{
    const size_t n = strlen(needle);
    for (const char *p = haystack; *p != '\0'; p++) {
        if (strncasecmp(p, needle, n) == 0) return p;
    }
    return NULL;
}

static size_t skip_block(const char *p, const char *tag)
{
    /* p at "<script" / "<style": skip to the matching close tag. */
    char close[16];
    snprintf(close, sizeof close, "</%s", tag);
    const char *end = strcasestr_compat(p, close);
    if (end == NULL) return strlen(p);
    const char *gt = strchr(end, '>');
    return gt ? (size_t)(gt + 1 - p) : strlen(p);
}

char *nc_html_text(const char *html, size_t max)
{
    if (html == NULL) return NULL;
    char *out = malloc(max + 1);
    if (out == NULL) return NULL;
    size_t o = 0;
    bool space = true; /* drop leading whitespace */
    const char *p = html;
    while (*p != '\0' && o < max) {
        if (*p == '<') {
            if (strncasecmp(p, "<script", 7) == 0) {
                p += skip_block(p, "script");
                continue;
            }
            if (strncasecmp(p, "<style", 6) == 0) {
                p += skip_block(p, "style");
                continue;
            }
            const bool block = strncasecmp(p, "<p", 2) == 0 || strncasecmp(p, "<br", 3) == 0 ||
                               strncasecmp(p, "<li", 3) == 0 || strncasecmp(p, "<h", 2) == 0 ||
                               strncasecmp(p, "<div", 4) == 0 || strncasecmp(p, "<tr", 3) == 0;
            const char *gt = strchr(p, '>');
            p = gt ? gt + 1 : p + strlen(p);
            if (block && o > 0 && out[o - 1] != '\n' && o < max) {
                if (o > 0 && out[o - 1] == ' ') o--;
                out[o++] = '\n';
                space = true;
            } else if (!space && o < max) {
                out[o++] = ' ';
                space = true;
            }
            continue;
        }
        char c = *p;
        if (c == '&') {
            static const struct {
                const char *name;
                char value;
            } ENTITIES[] = {{"&amp;", '&'},  {"&lt;", '<'},  {"&gt;", '>'},
                            {"&quot;", '"'}, {"&#39;", '\''}, {"&nbsp;", ' '}};
            bool done = false;
            for (size_t i = 0; i < sizeof ENTITIES / sizeof ENTITIES[0]; i++) {
                const size_t len = strlen(ENTITIES[i].name);
                if (strncmp(p, ENTITIES[i].name, len) == 0) {
                    c = ENTITIES[i].value;
                    p += len;
                    done = true;
                    break;
                }
            }
            if (!done) p++;
        } else {
            p++;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!space) {
                out[o++] = ' ';
                space = true;
            }
            continue;
        }
        out[o++] = c;
        space = false;
    }
    while (o > 0 && (out[o - 1] == ' ' || out[o - 1] == '\n')) o--;
    out[o] = '\0';
    nc_utf8_trim(out);
    return out;
}
