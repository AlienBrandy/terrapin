idf_component_register(SRCS  "jsmn.c" "main.c" "main_menu.c" "temp_sensor.c" "terrapin.c"
                    INCLUDE_DIRS ".")

message("CMAKE_PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
message("COMPONENT_TARGET = ${COMPONENT_TARGET}")                 
target_compile_definitions(${COMPONENT_TARGET} PRIVATE PROJECT_NAME="${CMAKE_PROJECT_NAME}")
