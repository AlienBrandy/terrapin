/**
 * datastream_menu.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include "datastream_menu.h"
#include "console_windows.h"
#include "datastream.h"

static menu_function_t parent_menu = NULL;

static menu_item_t* show(int argc, char* argv[])
{
    datastream_t ds;
    console_windows_printf(MENU_WINDOW, "\nIdx Name                                 Value\n");
    console_windows_printf(MENU_WINDOW, "--- -----------------------------------  --------------------\n");
    int idx = 0;
    while (datastream_get(idx, &ds) == DATASTREAM_ERR_NONE)
    {
        console_windows_printf(MENU_WINDOW, "%02d  %-32.32s %10.*f %-10.10s\n", idx, ds.name, ds.precision, ds.value, ds.units);
        idx++;
    }
    console_windows_printf(MENU_WINDOW, "\n");

    return NULL;
}

static menu_item_t* update(int argc, char* argv[])
{
    if (argc < 3)
    {
        console_windows_printf(MENU_WINDOW, "update: missing param(s)\n");
        return NULL;
    }

    int id = atoi(argv[1]);
    double value = atof(argv[2]);
    DATASTREAM_ERR_T retc = datastream_update(id, value);
    console_windows_printf(MENU_WINDOW, "update: %s\n", datastream_get_error_string(retc));
    return NULL;
}

static menu_item_t* update_by_name(int argc, char* argv[])
{
    if (argc < 3)
    {
        console_windows_printf(MENU_WINDOW, "update: missing param(s)\n");
        return NULL;
    }

    char* name = argv[1];
    double value = atof(argv[2]);
    DATASTREAM_ERR_T retc = datastream_update_by_name(name, value);
    console_windows_printf(MENU_WINDOW, "update: %s\n", datastream_get_error_string(retc));
    return NULL;
}

static menu_item_t* exit_datastream_menu(int argc, char* argv[])
{
    if (parent_menu == NULL)
    {
        return NULL;
    }
    return parent_menu(0, NULL);
}

static menu_item_t menu_item_datastream = {
    .func = datastream_menu,
    .cmd  = "",
    .desc = ""
};

static menu_item_t menu_item_exit = {
    .func = exit_datastream_menu,
    .cmd  = "exit",
    .desc = "exit datastream menu"
};

static menu_item_t menu_item_show = {
    .func = show,
    .cmd  = "show",
    .desc = "show all networks on list"
};

static menu_item_t menu_item_update = {
    .func = update,
    .cmd  = "update",
    .desc = "update datastream <idx> with <value>"
};

static menu_item_t menu_item_update_by_name = {
    .func = update_by_name,
    .cmd  = "update_name",
    .desc = "update datastream <name> with <value>"
};

static menu_item_t* menu_item_list[] = 
{
    &menu_item_exit,
    &menu_item_show,
    &menu_item_update,
    &menu_item_update_by_name,
};

static void show_help(void)
{
    console_windows_printf(MENU_WINDOW, "\ndatastream menu\n");

    static const int list_length = sizeof(menu_item_list) / sizeof(menu_item_list[0]);
    for (int i = 0; i < list_length; i++)
    {
        console_windows_printf(MENU_WINDOW, "%-20s: %s\n", menu_item_list[i]->cmd, menu_item_list[i]->desc);
    }
}

menu_item_t* datastream_menu(int argc, char* argv[])
{
    // check for blank line which is an indication to display the help menu
    if (argc == 0 || argv == NULL)
    {
        show_help();
        return &menu_item_datastream;
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

void datastream_menu_set_parent(menu_function_t menu)
{
    parent_menu = menu;
}

