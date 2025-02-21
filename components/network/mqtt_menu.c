/**
 * mqtt_menu.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include "mqtt_menu.h"
#include "console_windows.h"
#include "mqtt.h"

static menu_function_t parent_menu = NULL;

static menu_item_t* init(int argc, char* argv[])
{
    console_windows_printf(MENU_WINDOW, "initializing mqtt module...\n");
    bool retc = mqtt_init();
    console_windows_printf(MENU_WINDOW, "mqtt_init: %s\n", retc ? "No error" : "Failed");
    return NULL;
}

static menu_item_t* start_client(int argc, char* argv[])
{
    console_windows_printf(MENU_WINDOW, "starting mqtt client...\n");
    bool retc = mqtt_start();
    console_windows_printf(MENU_WINDOW, "mqtt_start: %s\n", retc ? "No error" : "Failed");
    return NULL;
}

static menu_item_t* stop_client(int argc, char* argv[])
{
    console_windows_printf(MENU_WINDOW, "stopping mqtt client...\n");
    mqtt_stop();
    console_windows_printf(MENU_WINDOW, "mqtt_stop() called.\n");
    return NULL;
}

static menu_item_t* publish(int argc, char* argv[])
{
    if (argc < 4)
    {
        console_windows_printf(MENU_WINDOW, "publish: missing param(s)\n");
        return NULL;
    }
    char* topic = argv[1];
    char* key = argv[2];
    char* val = argv[3];

    console_windows_printf(MENU_WINDOW, "publishing %s to %s/%s...\n", val, topic, key);
    mqtt_publish(topic, key, val);
    console_windows_printf(MENU_WINDOW, "mqtt_publish() called.\n");
    return NULL;
}

static menu_item_t* subscribe(int argc, char* argv[])
{
    if (argc < 2)
    {
        console_windows_printf(MENU_WINDOW, "subscribe: missing param(s)\n");
        return NULL;
    }
    char* topic = argv[1];

    console_windows_printf(MENU_WINDOW, "subscribing to %s...\n", topic);
    mqtt_subscribe(topic);
    console_windows_printf(MENU_WINDOW, "mqtt_subscribe() called.\n");
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

static menu_item_t menu_item_mqtt = {
    .func = mqtt_menu,
    .cmd  = "",
    .desc = ""
};

static menu_item_t menu_item_exit = {
    .func = exit_menu,
    .cmd  = "prev",
    .desc = "previous menu"
};

static menu_item_t menu_item_init = {
    .func = init,
    .cmd  = "init",
    .desc = "init mqtt module"
};

static menu_item_t menu_item_start = {
    .func = start_client,
    .cmd  = "start",
    .desc = "start mqtt client"
};

static menu_item_t menu_item_stop = {
    .func = stop_client,
    .cmd  = "stop",
    .desc = "stop mqtt client"
};

static menu_item_t menu_item_publish = {
    .func = publish,
    .cmd  = "publish",
    .desc = "publish <topic> <key> <value>"
};

static menu_item_t menu_item_subscribe = {
    .func = subscribe,
    .cmd  = "subscribe",
    .desc = "subscribe to <topic>"
};

static menu_item_t* menu_item_list[] = 
{
    &menu_item_exit,
    &menu_item_init,
    &menu_item_start,
    &menu_item_stop,
    &menu_item_publish,
    &menu_item_subscribe,
};

static void show_help(void)
{
    PRINT_MENU_TITLE("MQTT");
    static const int list_length = sizeof(menu_item_list) / sizeof(menu_item_list[0]);

    for (int i = 0; i < list_length; i++)
    {
        console_windows_printf(MENU_WINDOW, "%-20s: %s\n", menu_item_list[i]->cmd, menu_item_list[i]->desc);
    }
}

menu_item_t* mqtt_menu(int argc, char* argv[])
{
    // check for blank line which is an indication to display the help menu
    if (argc == 0 || argv == NULL)
    {
        show_help();
        return &menu_item_mqtt;
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

void mqtt_menu_set_parent(menu_function_t menu)
{
    parent_menu = menu;
}
