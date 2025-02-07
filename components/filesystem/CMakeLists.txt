idf_component_register(SRCS "filesystem.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES vfs nvs_flash fatfs)

message("CMAKE_PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
message("COMPONENT_TARGET = ${COMPONENT_TARGET}")
target_compile_definitions(${COMPONENT_TARGET} PRIVATE PROJECT_NAME="${CMAKE_PROJECT_NAME}")
