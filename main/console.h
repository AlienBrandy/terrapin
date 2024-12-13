/**
 * console.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 * 
 * console.c: manages the debug terminal
 *    initializes the uart and console
 *    launches the prompt thread
 *    launches the menu thread
 * 
 *  --------------------------
 * |> help                   | <--- prompt (interactive, single-line)
 * |-------------------------|
 * | menu:                   | <--- menu (write-only, scrolling)
 * |  wifi                   |
 * |  rtos                   |
 * |  nvs                    |
 * |                         |
 *  --------------------------
 */

#pragma once

typedef enum {
    CONSOLE_ERR_NONE,
    CONSOLE_ERR_PROMPT_INIT_FAIL,
    CONSOLE_ERR_MENU_INIT_FAIL,
    CONSOLE_ERR_TERMINAL_INIT_FAIL,
} CONSOLE_ERR_T;

CONSOLE_ERR_T console_init(void);
CONSOLE_ERR_T console_start(void);
