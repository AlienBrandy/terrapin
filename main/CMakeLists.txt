idf_component_register(SRCS "main.c" "prompt.c" "menu.c" "console.c" "filesystem.c" "console_windows.c" "ansi_term.c"
                    INCLUDE_DIRS ".")

message("CMAKE_PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
message("COMPONENT_TARGET = ${COMPONENT_TARGET}")                 
target_compile_definitions(${COMPONENT_TARGET} PRIVATE PROJECT_NAME="${CMAKE_PROJECT_NAME}")
