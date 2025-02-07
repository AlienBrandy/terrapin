/**
 * config_menu.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include "config_menu.h"
#include "console_windows.h"
#include "config.h"

static menu_function_t parent_menu = NULL;

static menu_item_t* set(int argc, char* argv[])
{
    if (argc < 3)
    {
        console_windows_printf(MENU_WINDOW, "set: missing param(s)\n");
        return NULL;
    }

    char* key = argv[1];
    char* val = argv[2];
    console_windows_printf(MENU_WINDOW, "setting %s to %s...\n", key, val);
    bool retc = config_set(key, val);
    console_windows_printf(MENU_WINDOW, "set: %s\n", retc ? "No error" : "Failed.");
    return NULL;
}

static menu_item_t* show(int argc, char* argv[])
{
    console_windows_printf(MENU_WINDOW, "\nidx key                              value\n");
    console_windows_printf(MENU_WINDOW, "--- -------------------------------- ---------------------------------\n");
    int idx = 0;
    while (1)
    {
        const char* key = config_get_key(idx++);
        if (key)
        {
            const char* value;
            config_get_value(key, &value);
            console_windows_printf(MENU_WINDOW, "%03d %-32.32s %-32.32s\n", idx, key, value);
        }
        else
        {
            console_windows_printf(MENU_WINDOW, "\n");
            break;
        }
    }
    return NULL;
}

static menu_item_t* exit_menu(int argc, char* argv[])
{
    if (parent_menu == NULL)
    {
        return NULL;
    }
    return parent_menu(0, NULL);
}

static menu_item_t menu_item_config = {
    .func = config_menu,
    .cmd  = "",
    .desc = ""
};

static menu_item_t menu_item_exit = {
    .func = exit_menu,
    .cmd  = "prev",
    .desc = "previous menu"
};

static menu_item_t menu_item_set = {
    .func = set,
    .cmd  = "set",
    .desc = "set config <key> to <value>"
};

static menu_item_t menu_item_show = {
    .func = show,
    .cmd  = "show",
    .desc = "show all configs"
};

static menu_item_t* menu_item_list[] = 
{
    &menu_item_exit,
    &menu_item_show,
    &menu_item_set,
};

static void show_help(void)
{
    PRINT_MENU_TITLE("Configs");
    static const int list_length = sizeof(menu_item_list) / sizeof(menu_item_list[0]);

    for (int i = 0; i < list_length; i++)
    {
        console_windows_printf(MENU_WINDOW, "%-20s: %s\n", menu_item_list[i]->cmd, menu_item_list[i]->desc);
    }
}

menu_item_t* config_menu(int argc, char* argv[])
{
    // check for blank line which is an indication to display the help menu
    if (argc == 0 || argv == NULL)
    {
        show_help();
        return &menu_item_config;
    }

    // search for matching command in list of registered menu items
    static const int list_length = sizeof(menu_item_list) / sizeof(menu_item_list[0]);
    for (int i = 0; i < list_length; i++)
    {
        if (strcmp(argv[0], menu_item_list[i]->cmd) == 0)
        {
            // match found, call menu item function.
            return (*menu_item_list[i]->func)(argc, argv);
        }
    }
    console_windows_printf(MENU_WINDOW, "unknown command [%s]\n", argv[0]);
    return NULL;
}

void config_menu_set_parent(menu_function_t menu)
{
    parent_menu = menu;
}
