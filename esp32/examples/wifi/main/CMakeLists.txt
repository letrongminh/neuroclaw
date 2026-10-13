idf_component_register(SRCS "main.c"
                       REQUIRES neuroclaw esp_wifi esp_netif esp_event nvs_flash console
                                joltwallet__littlefs)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
