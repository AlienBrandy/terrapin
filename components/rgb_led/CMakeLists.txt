idf_component_register(SRCS "rgb_led_menu.c" "rgb_led.c"
                       INCLUDE_DIRS "public_includes"
                       REQUIRES debug_console
                       PRIV_REQUIRES driver)

message("CMAKE_PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
message("COMPONENT_TARGET = ${COMPONENT_TARGET}")                 
target_compile_definitions(${COMPONENT_TARGET} PRIVATE PROJECT_NAME="${CMAKE_PROJECT_NAME}")
