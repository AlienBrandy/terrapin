idf_component_register(SRCS "config.c" "config_menu.c"
                       INCLUDE_DIRS "."
                       REQUIRES debug_console
                       PRIV_REQUIRES utilities filesystem)

message("CMAKE_PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
message("COMPONENT_TARGET = ${COMPONENT_TARGET}")
target_compile_definitions(${COMPONENT_TARGET} PRIVATE PROJECT_NAME="${CMAKE_PROJECT_NAME}")
