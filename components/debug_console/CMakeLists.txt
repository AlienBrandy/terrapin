idf_component_register(SRCS "prompt.c" "menu.c" "console.c" "console_windows.c" "ansi_term.c" "linenoise_lite.c"
                       INCLUDE_DIRS "public_includes"
                       PRIV_REQUIRES utilities vfs driver)

message("CMAKE_PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
message("COMPONENT_TARGET = ${COMPONENT_TARGET}")
target_compile_definitions(${COMPONENT_TARGET} PRIVATE PROJECT_NAME="${CMAKE_PROJECT_NAME}")
