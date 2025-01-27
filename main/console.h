/**
 * console.h
 * 
 * initializes the terminal interface.
 * The console module consolidates the initialization and startup of the components that together
 * provide a user interface for access via a terminal program over a serial connection. The terminal
 * interface presents a screen that is divided into two regions:
 * 
 * |-------------------------|
 * |                         | <-- output window
 * |                         |
 * |                         | 
 * | main menu               |
 * | net: show network       |
 * | status: show status     |
 * |-------------------------|
 * |                         | <-- user input window
 * | prompt> net             |
 * |-------------------------|
 * 
 * The lower region is small and displays a prompt for user input. The upper region is for menus, 
 * status screens, and other large blocks of text output. The regions can resize when the user
 * resizes the terminal window, though the screen will only repaint when <enter> is pressed.
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 * 
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
