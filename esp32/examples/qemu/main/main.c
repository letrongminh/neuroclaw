/*
 * NeuroClaw-ESP on QEMU with a stub gate. The gate here is a stand-in with the
 * rules of NeuroEdge's chat-den-cua demo (light: allowed; door: at most 30 s,
 * and only after the owner approves, once); on a NeuroEdge chip the same
 * nc_gate is backed by ne_gate. esp32/scripts/qemu_e2e.py drives it.
 */
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "driver/uart.h"
#include "esp_console.h"
#include "esp_littlefs.h"
#include "esp_eth.h"
#include "esp_eth_mac_openeth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "neuroclaw.h"

static const char *TAG = "example";

/* --- the stub gate ------------------------------------------------------------------------- */

static const nc_tool TOOLS[] = {
    {"living_light_on", "Turn the living-room light on.", "{\"type\":\"object\",\"properties\":{}}"},
    {"front_door_unlock", "Unlock the front door for a few seconds.",
     "{\"type\":\"object\",\"properties\":{\"duration_s\":{\"type\":\"integer\",\"minimum\":1,"
     "\"maximum\":30}},\"required\":[\"duration_s\"]}"},
};

static char s_pending[32]; /* the one confirmation waiting, "" when none */
static unsigned s_next_id = 1;

static size_t stub_tools(void *ctx, const nc_tool **out)
{
    (void)ctx;
    *out = TOOLS;
    return sizeof TOOLS / sizeof TOOLS[0];
}

static void stub_call(void *ctx, const char *name, const char *arguments, nc_tool_result *out)
{
    (void)ctx;
    memset(out, 0, sizeof *out);
    if (strcmp(name, "living_light_on") == 0) {
        out->status = NC_ALLOW;
        return;
    }
    if (strcmp(name, "front_door_unlock") != 0) {
        out->status = NC_REJECTED;
        snprintf(out->reason, sizeof out->reason, "unknown_tool");
        return;
    }
    cJSON *args = cJSON_Parse(arguments);
    const cJSON *d = cJSON_GetObjectItemCaseSensitive(args, "duration_s");
    const int duration = cJSON_IsNumber(d) ? d->valueint : 0;
    cJSON_Delete(args);
    if (duration < 1 || duration > 30) {
        out->status = NC_BLOCK;
        snprintf(out->reason, sizeof out->reason, "criterion_failed");
        snprintf(out->failed_criterion, sizeof out->failed_criterion, "duration_s");
        snprintf(out->message, sizeof out->message, "Cửa mở tối đa 30 giây.");
        return;
    }
    snprintf(s_pending, sizeof s_pending, "c-%u", s_next_id++);
    out->status = NC_BLOCK;
    out->asks_owner = true;
    snprintf(out->reason, sizeof out->reason, "awaiting_confirmation");
    snprintf(out->confirmation_id, sizeof out->confirmation_id, "%s", s_pending);
    snprintf(out->confirmation_message, sizeof out->confirmation_message,
             "Bộ não xin mở cửa %d giây — chủ nhà có đồng ý không?", duration);
    out->expires_in_ms = 60000;
}

static void stub_answer(void *ctx, const char *id, bool approve, nc_tool_result *out)
{
    (void)ctx;
    memset(out, 0, sizeof *out);
    if (s_pending[0] == '\0' || strcmp(id, s_pending) != 0) {
        out->status = NC_BLOCK;
        snprintf(out->reason, sizeof out->reason, "unknown_confirmation");
        return;
    }
    s_pending[0] = '\0'; /* one answer per question */
    if (!approve) {
        out->status = NC_BLOCK;
        snprintf(out->reason, sizeof out->reason, "owner_refused");
        return;
    }
    out->status = NC_ALLOW;
    snprintf(out->reason, sizeof out->reason, "owner_confirmed");
}

/* --- QEMU's Ethernet ----------------------------------------------------------------------- */

static EventGroupHandle_t s_ip;

static void on_ip(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    (void)id;
    (void)data;
    xEventGroupSetBits(s_ip, BIT0);
}

static void eth_up(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *netif = esp_netif_new(&netif_config);
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    esp_eth_mac_t *mac = esp_eth_mac_new_openeth(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_dp83848(&phy_config);
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &handle));
    ESP_ERROR_CHECK(esp_netif_attach(netif, esp_eth_new_netif_glue(handle)));
    s_ip = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, on_ip, NULL));
    ESP_ERROR_CHECK(esp_eth_start(handle));
    xEventGroupWaitBits(s_ip, BIT0, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "network up");
}

/* --- the serial console ------------------------------------------------------------------- */

/* NeuroClaw-ESP's commands on UART0, one per line, without a prompt (the test
 * reads the same UART). A board would rather run esp_console_new_repl_uart. */
static void console_task(void *arg)
{
    (void)arg;
    const uart_port_t port = CONFIG_ESP_CONSOLE_UART_NUM;
    ESP_ERROR_CHECK(uart_driver_install(port, 512, 0, 0, NULL, 0));
    char line[256];
    size_t n = 0;
    while (true) {
        uint8_t ch;
        if (uart_read_bytes(port, &ch, 1, portMAX_DELAY) != 1) continue;
        if (ch != '\n' && ch != '\r') {
            if (n < sizeof line - 1) line[n++] = (char)ch;
            continue;
        }
        if (n == 0) continue;
        line[n] = '\0';
        n = 0;
        int ret = 0;
        if (esp_console_run(line, &ret) == ESP_ERR_NOT_FOUND) printf("NC_CONSOLE unknown\n");
        fflush(stdout);
    }
}

static void console_up(void)
{
    const esp_console_config_t config = ESP_CONSOLE_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_init(&config));
    ESP_ERROR_CHECK(neuroclaw_console_register());
    xTaskCreate(console_task, "console", 6144, NULL, 3, NULL);
}

/* The workspace: LittleFS on the `ws` partition, kept across reboots. */
static void workspace_up(void)
{
    const esp_vfs_littlefs_conf_t conf = {
        .base_path = "/ws",
        .partition_label = "ws",
        .format_if_mount_failed = true,
    };
    ESP_ERROR_CHECK(esp_vfs_littlefs_register(&conf));
}

void app_main(void)
{
    workspace_up();
    eth_up();
    char llm_url[128];
    snprintf(llm_url, sizeof llm_url, "%s/v1/chat/completions", CONFIG_EXAMPLE_CLOUD);
    /* The fallback speaks the Anthropic Messages API: each URL has its own format. */
    char fallback_url[128];
    snprintf(fallback_url, sizeof fallback_url, "%s/v2/messages", CONFIG_EXAMPLE_CLOUD);
    char search_url[128];
    snprintf(search_url, sizeof search_url, "%s/tavily/search", CONFIG_EXAMPLE_CLOUD);
    const nc_config config = {
        .telegram_token = "7000000001:AAQemuOnlyNotARealTokenxxxxxxxxxxxx",
        .owner_id = CONFIG_EXAMPLE_OWNER,
        .telegram_api = CONFIG_EXAMPLE_CLOUD,
        .llm_url = llm_url,
        .llm_key = "sk-qemu-only-not-a-real-key",
        .llm_model = "fake",
        .workspace = "/ws",
        .tz = "<+07>-7",
        .search_provider = "tavily",
        .search_url = search_url,
        .search_key = "tvly-qemu-not-a-real-key",
        /* Test only: the fake cloud lives on QEMU's host address, which is private. */
        .fetch_allow = "10.0.2.2",
        .llm_fallback_url = fallback_url,
        .llm_fallback_key = "sk-qemu-fallback-not-a-real-key",
        .llm_fallback_model = "fake-backup",
        .llm_fallback_api = "anthropic",
    };
    const nc_gate gate = {NULL, stub_tools, stub_call, stub_answer};
    ESP_ERROR_CHECK(neuroclaw_start(&config, &gate));
    console_up();
}
