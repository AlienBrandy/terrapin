/**
 * network_manager_menu.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include "network_manager_menu.h"
#include "wifi_menu.h"
#include "mqtt_menu.h"
#include "console_windows.h"
#include "network_manager.h"
#include "mqtt.h"

static menu_function_t parent_menu = NULL;

static menu_item_t* show_wifi_menu(int argc, char* argv[])
{
    // switch menus
    wifi_menu_set_parent(network_manager_menu);
    return wifi_menu(0, NULL);
}

static menu_item_t* show_mqtt_menu(int argc, char* argv[])
{
    // switch menus
    mqtt_menu_set_parent(network_manager_menu);
    return mqtt_menu(0, NULL);
}

static menu_item_t* initialize(int argc, char* argv[])
{
    console_windows_printf(MENU_WINDOW, "initializing...\n");
    NETWORK_MANAGER_ERR_T code = network_manager_init();
    console_windows_printf(MENU_WINDOW, "initialize: %s\n", network_manager_get_error_string(code));
    return NULL;
}

static menu_item_t* connect(int argc, char* argv[])
{
    console_windows_printf(MENU_WINDOW, "starting scan for known networks...\n");
    NETWORK_MANAGER_ERR_T code = network_manager_connect(WAIT);
    console_windows_printf(MENU_WINDOW, "connect: %s\n", network_manager_get_error_string(code));
    return NULL;
}

static menu_item_t* connect_to(int argc, char* argv[])
{
    if (argc < 3)
    {
        console_windows_printf(MENU_WINDOW, "connect_to: missing param(s)\n");
        return NULL;
    }
    char* ssid = argv[1];
    char* pwd  = argv[2];

    console_windows_printf(MENU_WINDOW, "connecting to %s...\n", ssid);
    NETWORK_MANAGER_ERR_T code = network_manager_connect_to(ssid, pwd, WAIT);
    console_windows_printf(MENU_WINDOW, "connect_to: %s\n", network_manager_get_error_string(code));
    return NULL;
}

static menu_item_t* disconnect(int argc, char* argv[])
{
    console_windows_printf(MENU_WINDOW, "disconnecting...\n");
    NETWORK_MANAGER_ERR_T code = network_manager_disconnect(WAIT);
    console_windows_printf(MENU_WINDOW, "disconnect: %s\n", network_manager_get_error_string(code));
    return NULL;
}

static menu_item_t* show_current_state(int argc, char* argv[])
{
    console_windows_printf(MENU_WINDOW, "state: %s\n", network_manager_get_current_state());
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

static menu_item_t menu_item_network_manager = {
    .func = network_manager_menu,
    .cmd  = "",
    .desc = ""
};

static menu_item_t menu_item_exit = {
    .func = exit_menu,
    .cmd  = "prev",
    .desc = "previous menu"
};

static menu_item_t menu_item_wifi = {
    .func = show_wifi_menu,
    .cmd  = "wifi",
    .desc = "wifi menu"
};

static menu_item_t menu_item_mqtt = {
    .func = show_mqtt_menu,
    .cmd  = "mqtt",
    .desc = "mqtt menu"
};

static menu_item_t menu_item_initialize = {
    .func = initialize,
    .cmd  = "init",
    .desc = "initialize network manager"
};

static menu_item_t menu_item_connect = {
    .func = connect,
    .cmd  = "connect",
    .desc = "connect to known networks"
};

static menu_item_t menu_item_connect_to = {
    .func = connect_to,
    .cmd  = "connect_to",
    .desc = "connect to network <ssid> <pwd>"
};

static menu_item_t menu_item_disconnect = {
    .func = disconnect,
    .cmd  = "disconnect",
    .desc = "disconnect from network"
};

static menu_item_t menu_item_show_current_state = {
    .func = show_current_state,
    .cmd  = "state",
    .desc = "show current state"
};

static menu_item_t* menu_item_list[] = 
{
    &menu_item_exit,
    &menu_item_initialize,
    &menu_item_connect,
    &menu_item_connect_to,
    &menu_item_disconnect,
    &menu_item_show_current_state,
    &menu_item_wifi,
    &menu_item_mqtt,
};

static void show_help(void)
{
    PRINT_MENU_TITLE("Network Manager");
    static const int list_length = sizeof(menu_item_list) / sizeof(menu_item_list[0]);

    for (int i = 0; i < list_length; i++)
    {
        console_windows_printf(MENU_WINDOW, "%-20s: %s\n", menu_item_list[i]->cmd, menu_item_list[i]->desc);
    }
}

menu_item_t* network_manager_menu(int argc, char* argv[])
{
    // check for blank line which is an indication to display the help menu
    if (argc == 0 || argv == NULL)
    {
        show_help();
        return &menu_item_network_manager;
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

void network_manager_menu_set_parent(menu_function_t menu)
{
    parent_menu = menu;
}
