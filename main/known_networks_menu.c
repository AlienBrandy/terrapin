/**
 * known_networks_menu.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include "known_networks_menu.h"
#include "main_menu.h"
#include "console_windows.h"
#include "known_networks.h"
#include "wifi_menu.h"
#include "esp_log.h"

static menu_item_t* item_list = NULL;
static int list_size = 0;

static void show_help(void)
{
    console_windows_printf(CONSOLE_WINDOW_2, "\nknown networks menu\n");

    for (int i = 0; i < list_size; i++)
    {
        console_windows_printf(CONSOLE_WINDOW_2, "%-20s: %s\n", item_list[i].cmd, item_list[i].desc);
    }
}

// forward declaration
static menu_item_t menu_item_known_networks;

menu_item_t* known_networks_menu(int argc, char* argv[])
{
    // check for blank line which indicates display help menu
    if (argc == 0 || argv == NULL)
    {
        show_help();
        return &menu_item_known_networks;
    }

    // search for matching command in list of registered menu items
    for (int i = 0; i < list_size; i++)
    {
        if (strcmp(argv[0], item_list[i].cmd) == 0)
        {
            // match found, call menu item function.
            return (*item_list[i].func)(argc, argv);
        }
    }
    console_windows_printf(CONSOLE_WINDOW_2, "unknown command [%s]\n", argv[0]);
    return NULL;
}

static menu_item_t* add(int argc, char* argv[])
{
    if (argc < 3)
    {
        console_windows_printf(CONSOLE_WINDOW_2, "add: missing arguments.\n");
        return NULL;
    }

    char* ssid = argv[1];
    char* pwd  = argv[2];

    KNOWN_NETWORKS_ERR_T code = known_networks_add(ssid, pwd);
    console_windows_printf(CONSOLE_WINDOW_2, "add: %s\n", known_networks_get_error_string(code));
    return NULL;
}

static menu_item_t* remove(int argc, char* argv[])
{
    if (argc < 2)
    {
        console_windows_printf(CONSOLE_WINDOW_2, "remove: missing arguments.\n");
        return NULL;
    }

    char* ssid = argv[1];

    KNOWN_NETWORKS_ERR_T code = known_networks_remove(ssid);
    console_windows_printf(CONSOLE_WINDOW_2, "remove: %s\n", known_networks_get_error_string(code));
    return NULL;
}

static menu_item_t* show(int argc, char* argv[])
{
    if (argc < 2)
    {
        console_windows_printf(CONSOLE_WINDOW_2, "get: missing arguments.\n");
        return NULL;
    }

    uint8_t index = atoi(argv[1]);
    known_network_entry_t entry;
    KNOWN_NETWORKS_ERR_T code = known_networks_get_entry(index, &entry);
    if (code == KNOWN_NETWORKS_ERR_NONE)
    {
        console_windows_printf(CONSOLE_WINDOW_2, "index %d: %s\n", index, entry.ssid);
    }
    console_windows_printf(CONSOLE_WINDOW_2, "show: %s\n", known_networks_get_error_string(code));

    return NULL;
}

static menu_item_t* show_all(int argc, char* argv[])
{
    uint8_t num_entries = known_networks_get_number_of_entries();
    if (num_entries > 0)
    {
        console_windows_printf(CONSOLE_WINDOW_2, "\nidx SSID\n");
        console_windows_printf(CONSOLE_WINDOW_2, "--- ---------------------------------\n");
        for (int idx = 0; idx < num_entries; idx++)
        {
            known_network_entry_t entry;
            known_networks_get_entry(idx, &entry);
            console_windows_printf(CONSOLE_WINDOW_2, "%03d %-32.32s\n", idx, entry.ssid);
        }
    }
    else
    {
        console_windows_printf(CONSOLE_WINDOW_2, "show: no known networks recorded.\n");
    }
    return NULL;
}

static menu_item_t* exit_known_networks_menu(int argc, char* argv[])
{
    // switch menus
    return wifi_menu(0, NULL);
}

static menu_item_t menu_item_known_networks = {
    .func = known_networks_menu,
    .cmd  = "",
    .desc = ""
};

static menu_item_t menu_item_exit = {
    .func = exit_known_networks_menu,
    .cmd  = "exit",
    .desc = "exit known networks menu"
};

static menu_item_t menu_item_add = {
    .func = add,
    .cmd  = "add",
    .desc = "add network <ssid> <pwd>"
};

static menu_item_t menu_item_remove = {
    .func = remove,
    .cmd  = "remove",
    .desc = "remove network <ssid>"
};

static menu_item_t menu_item_show = {
    .func = show,
    .cmd  = "show_idx",
    .desc = "show network <index> from list"
};

static menu_item_t menu_item_show_all = {
    .func = show_all,
    .cmd  = "show",
    .desc = "show all networks on list"
};

menu_item_t* known_networks_menu_init(void)
{
    menu_register_item(&menu_item_exit, &item_list, &list_size);
    menu_register_item(&menu_item_add, &item_list, &list_size);
    menu_register_item(&menu_item_remove, &item_list, &list_size);
    menu_register_item(&menu_item_show, &item_list, &list_size);
    menu_register_item(&menu_item_show_all, &item_list, &list_size);

    return &menu_item_known_networks;
}

