/**
 * menu.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define MENU_COMMAND_MAX_BYTES 128

typedef enum {
    MENU_ERR_NONE,
    MENU_ERR_NOT_INITIALIZED,
    MENU_ERR_TASK_START_FAIL,
    MENU_ERR_QUEUE_CREATE_FAIL,
} MENU_ERR_T;

/**
 * 
 */
struct menu_item_tag;
typedef struct menu_item_tag* (*menu_function_t)(int argc, char* argv[]);

/**
 * 
 */
typedef struct menu_item_tag {
    menu_function_t func;
    const char*     cmd; 
    const char*     desc; 
} menu_item_t;

MENU_ERR_T menu_init(void);
MENU_ERR_T menu_start(menu_function_t top_level_menu);
MENU_ERR_T menu_send_command(const char* command);
