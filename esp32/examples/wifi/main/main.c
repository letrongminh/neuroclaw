/*
 * NeuroClaw-ESP on a Wi-Fi board: the brain alone, with an empty gate (no
 * devices), the workspace on LittleFS, the Wi-Fi signal in get_diagnostics, and
 * the serial console (help, memory_read, diag...). On a NeuroEdge chip the gate
 * is ne_gate and the firmware owns Wi-Fi; this is the smallest app that builds
 * the component for every target. Settings come from `idf.py menuconfig` and
 * end up in sdkconfig, which is not committed.
 */
#include <string.h>

#include "esp_console.h"
#include "esp_event.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "neuroclaw.h"
#include "nvs_flash.h"

static const char *TAG = "example";

/* --- an empty gate: no device tools ------------------------------------------------------- */

static size_t no_tools(void *ctx, const nc_tool **out)
{
    (void)ctx;
    *out = NULL;
    return 0;
}

static void reject(void *ctx, const char *name, const char *arguments, nc_tool_result *out)
{
    (void)ctx;
    (void)name;
    (void)arguments;
    memset(out, 0, sizeof *out);
    out->status = NC_REJECTED;
    strcpy(out->reason, "unknown_tool");
}

static void no_answer(void *ctx, const char *id, bool approve, nc_tool_result *out)
{
    (void)ctx;
    (void)id;
    (void)approve;
    memset(out, 0, sizeof *out);
    out->status = NC_BLOCK;
    strcpy(out->reason, "unknown_confirmation");
}

/* --- Wi-Fi --------------------------------------------------------------------------------- */

static EventGroupHandle_t s_ip;

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;
    if (base == WIFI_EVENT && (id == WIFI_EVENT_STA_START || id == WIFI_EVENT_STA_DISCONNECTED)) {
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_ip, BIT0);
    }
}

static void wifi_up(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    const wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    s_ip = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, NULL));
    wifi_config_t config = {0};
    strncpy((char *)config.sta.ssid, CONFIG_EXAMPLE_WIFI_SSID, sizeof config.sta.ssid);
    strncpy((char *)config.sta.password, CONFIG_EXAMPLE_WIFI_PASSWORD,
            sizeof config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
    xEventGroupWaitBits(s_ip, BIT0, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "network up");
}

static bool wifi_rssi(int *dbm)
{
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) return false;
    *dbm = ap.rssi;
    return true;
}

/* --- the console --------------------------------------------------------------------------- */

static void console_up(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    config.prompt = "neuroclaw>";
#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    const esp_console_dev_uart_config_t dev = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&dev, &config, &repl));
#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    const esp_console_dev_usb_cdc_config_t dev = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&dev, &config, &repl));
#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    const esp_console_dev_usb_serial_jtag_config_t dev =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&dev, &config, &repl));
#else
    return; /* no console on this board */
#endif
    ESP_ERROR_CHECK(esp_console_register_help_command());
    ESP_ERROR_CHECK(neuroclaw_console_register());
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    const esp_vfs_littlefs_conf_t fs = {
        .base_path = "/ws",
        .partition_label = "ws",
        .format_if_mount_failed = true,
    };
    ESP_ERROR_CHECK(esp_vfs_littlefs_register(&fs));
    wifi_up();
    const bool search = CONFIG_EXAMPLE_TAVILY_KEY[0] != '\0';
    const nc_config config = {
        .telegram_token = CONFIG_EXAMPLE_TELEGRAM_TOKEN,
        .owner_id = CONFIG_EXAMPLE_OWNER,
        .llm_url = CONFIG_EXAMPLE_LLM_URL,
        .llm_key = CONFIG_EXAMPLE_LLM_KEY,
        .llm_model = CONFIG_EXAMPLE_LLM_MODEL,
        .llm_api = CONFIG_EXAMPLE_LLM_API,
        .workspace = "/ws",
        .search_provider = search ? "tavily" : NULL,
        .search_key = CONFIG_EXAMPLE_TAVILY_KEY,
        .wifi_rssi = wifi_rssi,
    };
    const nc_gate gate = {NULL, no_tools, reject, no_answer};
    ESP_ERROR_CHECK(neuroclaw_start(&config, &gate));
    console_up();
}
