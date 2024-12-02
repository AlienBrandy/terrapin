#pragma once

#define MENU_COMMAND_MAX_BYTES 128

typedef enum {
    MENU_ERR_NONE,
    MENU_ERR_NOT_INITIALIZED,
    MENU_ERR_TASK_START_FAIL,
    MENU_ERR_QUEUE_CREATE_FAIL,
} MENU_ERR_T;

typedef struct {
    char command_string[MENU_COMMAND_MAX_BYTES];
} menu_command_t;

MENU_ERR_T menu_init(void);
MENU_ERR_T menu_start(void);
MENU_ERR_T menu_send_command(menu_command_t* command);