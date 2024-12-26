/**
 * wifi_menu.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include "wifi_menu.h"
#include "main_menu.h"
#include "console_windows.h"
#include "wifi.h"

static menu_item_t* item_list = NULL;
static int list_size = 0;

static void show_help(void)
{
    console_windows_printf(CONSOLE_WINDOW_2, "\nwifi menu\n");

    for (int i = 0; i < list_size; i++)
    {
        console_windows_printf(CONSOLE_WINDOW_2, "%-20s: %s\n", item_list[i].cmd, item_list[i].desc);
    }
}

// forward declaration
static menu_item_t menu_item_wifi;

menu_item_t* wifi_menu(int argc, char* argv[])
{
    // check for blank line which indicates display help menu
    if (argc == 0 || argv == NULL)
    {
        show_help();
        return &menu_item_wifi;
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

static menu_item_t* scan(int argc, char* argv[])
{
    console_windows_printf(CONSOLE_WINDOW_2, "scanning...\n");
    WIFI_ERR_T code = wifi_scan();
    if (code == WIFI_ERR_NONE)
    {
        uint16_t num_networks = wifi_get_number_of_networks();
        console_windows_printf(CONSOLE_WINDOW_2, "%d networks found.\n", num_networks);
        if (num_networks > 0)
        {
            console_windows_printf(CONSOLE_WINDOW_2, "idx SSID                              dBm \n");
            console_windows_printf(CONSOLE_WINDOW_2, "--- --------------------------------- ----\n");
            for (uint16_t i = 0; i < num_networks; i++)
            {
                wifi_network_record_t network_record;
                wifi_get_network_record(i, &network_record);
                console_windows_printf(CONSOLE_WINDOW_2, "%03d %-32.32s %4d\n", i, network_record.ssid, network_record.rssi);
            }
        }
    }

    console_windows_printf(CONSOLE_WINDOW_2, "scan: %s\n", wifi_get_error_string(code));
    return NULL;
}

static menu_item_t* connect(int argc, char* argv[])
{
    if (argc < 3)
    {
        console_windows_printf(CONSOLE_WINDOW_2, "connect: missing param(s)\n");
        return NULL;
    }

    char* ssid = argv[1];
    char* pwd  = argv[2];
    uint32_t timeout_ms = 10000;

    console_windows_printf(CONSOLE_WINDOW_2, "connecting...\n");
    WIFI_ERR_T code = wifi_connect(ssid, pwd, timeout_ms);
    console_windows_printf(CONSOLE_WINDOW_2, "connect: %s\n", wifi_get_error_string(code));
    return NULL;
}

static menu_item_t* disconnect(int argc, char* argv[])
{
    console_windows_printf(CONSOLE_WINDOW_2, "disconnecting...\n");
    WIFI_ERR_T code = wifi_disconnect();
    console_windows_printf(CONSOLE_WINDOW_2, "disconnect: %s\n", wifi_get_error_string(code));
    return NULL;
}

static menu_item_t* exit_wifi_menu(int argc, char* argv[])
{
    // switch menus
    return main_menu(0, NULL);
}

static menu_item_t menu_item_wifi = {
    .func = wifi_menu,
    .cmd  = "",
    .desc = ""
};

static menu_item_t menu_item_exit = {
    .func = exit_wifi_menu,
    .cmd  = "exit",
    .desc = "exit wifi menu"
};

static menu_item_t menu_item_scan_for_networks = {
    .func = scan,
    .cmd  = "scan",
    .desc = "scan for networks"
};

static menu_item_t menu_item_connect = {
    .func = connect,
    .cmd  = "connect",
    .desc = "connect to wifi <ssid> <pwd>"
};

static menu_item_t menu_item_disconnect = {
    .func = disconnect,
    .cmd  = "disconnect",
    .desc = "disconnect from wifi"
};

menu_item_t* wifi_menu_init(void)
{
    menu_register_item(&menu_item_exit, &item_list, &list_size);
    menu_register_item(&menu_item_scan_for_networks, &item_list, &list_size);
    menu_register_item(&menu_item_connect, &item_list, &list_size);
    menu_register_item(&menu_item_disconnect, &item_list, &list_size);

    return &menu_item_wifi;
}

