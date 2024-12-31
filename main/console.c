/**
 * console.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "console.h"
#include "console_windows.h"
#include "prompt.h"
#include "menu.h"
#include "main_menu.h"
#include "ansi_term.h"
#include "esp_log.h"

CONSOLE_ERR_T console_init(void)
{
    if (!ansi_term_init())
        return CONSOLE_ERR_TERMINAL_INIT_FAIL;

    if (prompt_init() != PROMPT_ERR_NONE)
        return CONSOLE_ERR_PROMPT_INIT_FAIL;

    if (menu_init() != MENU_ERR_NONE)
        return CONSOLE_ERR_MENU_INIT_FAIL;

    return CONSOLE_ERR_NONE;
}

CONSOLE_ERR_T console_start(void)
{
    console_windows_init();

    // redirect esp log messages to desired window
    esp_log_set_vprintf(console_windows_logf);

    prompt_start();
    menu_start(main_menu);

    return CONSOLE_ERR_NONE;
}

