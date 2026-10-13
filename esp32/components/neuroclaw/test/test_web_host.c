/*
 * Host runner for NeuroClaw-ESP's web tools (nc_web.c): which addresses
 * web_fetch refuses, URL parsing, the allow list, search URLs and replies, and
 * HTML to text. Exit 0 and `NC_WEB OK checks=<n>` only when every check passed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nc_web.h"

static unsigned s_checks;

static int fail(const char *what, int line)
{
    fprintf(stderr, "FAIL line %d: %s\n", line, what);
    return 1;
}

#define CHECK(cond)                                \
    do {                                           \
        s_checks++;                                \
        if (!(cond)) return fail(#cond, __LINE__); \
    } while (0)

static int check_addresses(void)
{
    const char *private_ones[] = {"127.0.0.1", "10.0.2.2", "192.168.1.10", "172.16.0.1",
                                  "172.31.255.255", "169.254.169.254", "100.64.0.1", "0.0.0.0",
                                  "224.0.0.1", "255.255.255.255", "::1", "::", "fc00::1",
                                  "fd12:3456::1", "fe80::1", "ff02::1", "::ffff:192.168.1.1",
                                  "::ffff:127.0.0.1", "not-an-ip", "", "1.2.3", NULL};
    for (int i = 0; private_ones[i] != NULL; i++) {
        if (!nc_ip_is_private(private_ones[i])) {
            fprintf(stderr, "  not refused: %s\n", private_ones[i]);
            CHECK(0);
        }
    }
    CHECK(nc_ip_is_private(NULL));
    const char *public_ones[] = {"1.1.1.1", "8.8.8.8", "172.32.0.1", "172.15.255.255",
                                 "100.128.0.1", "192.169.0.1", "2606:4700:4700::1111",
                                 "::ffff:8.8.8.8", NULL};
    for (int i = 0; public_ones[i] != NULL; i++) {
        if (nc_ip_is_private(public_ones[i])) {
            fprintf(stderr, "  refused: %s\n", public_ones[i]);
            CHECK(0);
        }
    }
    return 0;
}

static int check_urls(void)
{
    bool https = false;
    char host[64];
    int port = 0;
    CHECK(nc_url_parts("https://vnexpress.net/thoi-tiet", &https, host, sizeof host, &port));
    CHECK(https && strcmp(host, "vnexpress.net") == 0 && port == 443);
    CHECK(nc_url_parts("http://10.0.2.2:8090/page?x=1", &https, host, sizeof host, &port));
    CHECK(!https && strcmp(host, "10.0.2.2") == 0 && port == 8090);
    CHECK(nc_url_parts("http://[fe80::1]:8080/", &https, host, sizeof host, &port));
    CHECK(strcmp(host, "fe80::1") == 0 && port == 8080);
    CHECK(nc_url_parts("HTTPS://Example.com", &https, host, sizeof host, &port));
    CHECK(!nc_url_parts("ftp://example.com/", &https, host, sizeof host, &port));
    CHECK(!nc_url_parts("file:///etc/passwd", &https, host, sizeof host, &port));
    CHECK(!nc_url_parts("http://user:pw@192.168.1.1/", &https, host, sizeof host, &port));
    CHECK(!nc_url_parts("http:///x", &https, host, sizeof host, &port));
    CHECK(!nc_url_parts("http://host:0/", &https, host, sizeof host, &port));
    CHECK(!nc_url_parts("http://host:99999/", &https, host, sizeof host, &port));
    CHECK(!nc_url_parts(NULL, &https, host, sizeof host, &port));

    CHECK(nc_host_allowed("10.0.2.2", "10.0.2.2"));
    CHECK(nc_host_allowed("NAS.local", "printer.local, nas.local"));
    CHECK(!nc_host_allowed("10.0.2.20", "10.0.2.2"));
    CHECK(!nc_host_allowed("10.0.2.2", ""));
    CHECK(!nc_host_allowed("10.0.2.2", NULL));
    return 0;
}

static int check_search(void)
{
    char *q = nc_url_encode("thời tiết Hà Nội & mưa");
    CHECK(q != NULL && strcmp(q, "th%E1%BB%9Di%20ti%E1%BA%BFt%20H%C3%A0%20N%E1%BB%99i%20%26%20m%C6%B0a") == 0);
    free(q);
    char *url = nc_search_url("searxng", "http://10.0.2.2:8090", "đèn", 3, "w");
    CHECK(url != NULL && strcmp(url, "http://10.0.2.2:8090/search?q=%C4%91%C3%A8n&format=json&time_range=week") == 0);
    free(url);
    url = nc_search_url("brave", NULL, "a b", 5, NULL);
    CHECK(url != NULL && strcmp(url, "https://api.search.brave.com/res/v1/web/search?q=a%20b&count=5") == 0);
    free(url);
    url = nc_search_url("brave", NULL, "a", 5, "d");
    CHECK(url != NULL && strstr(url, "&freshness=pd") != NULL);
    free(url);
    CHECK(nc_search_url("google", "x", "a", 5, NULL) == NULL);
    CHECK(nc_search_url("searxng", "", "a", 5, NULL) == NULL);

    char *text = nc_search_text("searxng",
                                "{\"results\":[{\"title\":\"Thời tiết <b>Hà Nội</b>\",\"url\":"
                                "\"https://a.example/1\",\"content\":\"Mưa rào &amp; dông\"},"
                                "{\"title\":\"B\",\"url\":\"https://b.example\"},"
                                "{\"title\":\"C\",\"url\":\"https://c.example\"}]}",
                                "thời tiết", 2);
    CHECK(text != NULL);
    CHECK(strncmp(text, "Results for: thời tiết (via SearXNG)\n1. Thời tiết Hà Nội\n   https://a.example/1\n   Mưa rào & dông", 107) == 0);
    CHECK(strstr(text, "2. B\n   https://b.example") != NULL);
    CHECK(strstr(text, "c.example") == NULL); /* count */
    free(text);
    text = nc_search_text("brave",
                          "{\"web\":{\"results\":[{\"title\":\"T\",\"url\":\"https://t.example\","
                          "\"description\":\"d\"}]}}",
                          "q", 5);
    CHECK(text != NULL && strstr(text, "(via Brave)\n1. T\n   https://t.example\n   d") != NULL);
    free(text);
    text = nc_search_text("searxng", "{\"results\":[]}", "q", 5);
    CHECK(text != NULL && strstr(text, "No results.") != NULL);
    free(text);
    text = nc_search_text("searxng", "<html>blocked</html>", "q", 5);
    CHECK(text != NULL && strncmp(text, "error:", 6) == 0);
    free(text);
    return 0;
}

static int check_html(void)
{
    char *t = nc_html_text("<html><head><title>X</title><style>p{color:red}</style>"
                           "<script>var a = '<p>';</script></head><body><h1>Tiêu đề</h1>"
                           "<p>Đoạn   một &amp; hai.</p><ul><li>Một</li><li>Hai</li></ul>"
                           "<SCRIPT>evil()</SCRIPT>Cuối&nbsp;trang</body></html>",
                           1000);
    CHECK(t != NULL);
    CHECK(strstr(t, "color") == NULL && strstr(t, "var a") == NULL && strstr(t, "evil") == NULL);
    CHECK(strstr(t, "Tiêu đề") != NULL);
    CHECK(strstr(t, "Đoạn một & hai.") != NULL);
    CHECK(strstr(t, "Một\nHai") != NULL);
    CHECK(strstr(t, "Cuối trang") != NULL);
    CHECK(strchr(t, '<') == NULL);
    free(t);
    t = nc_html_text("plain text, not html", 1000);
    CHECK(t != NULL && strcmp(t, "plain text, not html") == 0);
    free(t);
    /* Cut at max, never inside a character. */
    t = nc_html_text("ăăăăă", 5); /* 2 bytes each: 5 bytes is two and a half */
    CHECK(t != NULL && strcmp(t, "ăă") == 0);
    free(t);
    t = nc_html_text("<script>never closed", 100);
    CHECK(t != NULL && t[0] == '\0');
    free(t);
    CHECK(nc_html_text(NULL, 10) == NULL);
    return 0;
}

int main(void)
{
    if (check_addresses() != 0) return 1;
    if (check_urls() != 0) return 1;
    if (check_search() != 0) return 1;
    if (check_html() != 0) return 1;
    printf("NC_WEB OK checks=%u\n", s_checks);
    return 0;
}
