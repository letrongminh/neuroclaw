# NeuroClaw-ESP — the NeuroClaw brain as an ESP-IDF component (NeuroEdge Q-79).
# src/nc_core.c is plain C99 + cJSON and is tested on the host
# (esp32/scripts/host_test.sh); src/nc_brain.c owns HTTP and the task.
idf_component_register(
    SRCS
        "src/nc_core.c"
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
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
