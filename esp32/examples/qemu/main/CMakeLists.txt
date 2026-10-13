idf_component_register(SRCS "main.c"
                       REQUIRES neuroclaw esp_eth esp_netif esp_event json)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
