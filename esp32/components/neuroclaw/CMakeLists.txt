# NeuroClaw-ESP — the NeuroClaw brain as an ESP-IDF component (NeuroEdge Q-79).
# src/nc_core.c, nc_workspace.c and nc_cron.c are plain C (+ cJSON, POSIX files)
# and are tested on the host (esp32/scripts/host_test.sh); src/nc_brain.c owns
# HTTP, the clock and the task.
idf_component_register(
    SRCS
        "src/nc_core.c"
        "src/nc_workspace.c"
        "src/nc_cron.c"
        "src/nc_web.c"
        "src/nc_brain.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        json
    PRIV_REQUIRES
        esp_http_client
        esp-tls
        mbedtls
        freertos
        log
        esp_timer
        lwip
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
