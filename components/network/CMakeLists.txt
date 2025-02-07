idf_component_register(SRCS "wifi.c" "wifi_menu.c" "known_networks.c" "known_networks_menu.c" "mqtt.c" "mqtt_menu.c" "network_manager.c" "network_manager_menu.c"
                       INCLUDE_DIRS "public_includes"
                       REQUIRES debug_console
                       PRIV_REQUIRES state_machine configs esp_wifi mqtt filesystem)

message("CMAKE_PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
message("COMPONENT_TARGET = ${COMPONENT_TARGET}")                 
target_compile_definitions(${COMPONENT_TARGET} PRIVATE PROJECT_NAME="${CMAKE_PROJECT_NAME}")
