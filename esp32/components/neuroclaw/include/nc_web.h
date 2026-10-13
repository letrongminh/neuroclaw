/*
 * NeuroClaw-ESP's web tools, as the Go NeuroClaw's web_search and web_fetch
 * (pkg/tools/integration/web.go): search through a configured provider
 * (SearXNG, keyless and self-hostable, or Brave with a key), and fetch a page as
 * text.
 *
 * web_fetch never reaches a private or local address — loopback, RFC 1918,
 * link-local, CGNAT, multicast, IPv6 ULA and link-local — unless the host is in
 * the configured allow list, as the Go version refuses them. On a home chip this
 * is what keeps the model from talking to the home's own devices around the
 * gate. The brain resolves the host and checks every address before it
 * connects, and follows at most three redirects itself, checking each.
 * Remaining risk, said plainly: the HTTP client resolves the name again when it
 * connects, so a DNS answer that changes in between (rebinding) is not caught.
 *
 * Plain C (+ cJSON, sockets headers): the parsing and the address rule are
 * tested on the host.
 */
#ifndef NC_WEB_H
#define NC_WEB_H

#include <stdbool.h>
#include <stddef.h>

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NC_FETCH_TEXT_MAX 6000u /* bytes of page text the model gets */

/* An address the model must not reach: IPv4 or IPv6 text form. Unparsable is
 * treated as private (refused). */
bool nc_ip_is_private(const char *address);

/* Split an http(s) URL. False for anything else, or userinfo in it. */
bool nc_url_parts(const char *url, bool *https, char *host, size_t host_cap, int *port);

/* `host` is in the comma-separated `allow` list (exact, case-insensitive). */
bool nc_host_allowed(const char *host, const char *allow);

/* Percent-encode for a query string. malloc'd. */
char *nc_url_encode(const char *text);

/* The search request URL for `provider` ("searxng" or "brave"), malloc'd; NULL
 * for an unknown provider. `range` is d, w, m, y or NULL. */
char *nc_search_url(const char *provider, const char *base_url, const char *query, int count,
                    const char *range);

/* A provider's JSON reply as the Go tool's text: "Results for: <q> (via <p>)"
 * then "N. title\n   url\n   snippet". malloc'd; an error text when it does not
 * parse. */
char *nc_search_text(const char *provider, const char *json, const char *query, int count);

/* HTML (or plain text) to readable text: scripts, styles and tags out, the
 * common entities decoded, whitespace collapsed, at most `max` bytes, UTF-8
 * whole. malloc'd. */
char *nc_html_text(const char *html, size_t max);

#ifdef __cplusplus
}
#endif

#endif /* NC_WEB_H */
