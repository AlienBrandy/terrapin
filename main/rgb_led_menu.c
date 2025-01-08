/**
 * rgb_led_menu.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "rgb_led_menu.h"
#include "console_windows.h"
#include "rgb_led.h"

static menu_function_t parent_menu = NULL;

static menu_item_t* set_color(int argc, char* argv[])
{
    if (argc < 4)
    {
        console_windows_printf(MENU_WINDOW, "set color: missing param(s)\n");
        return NULL;
    }

    uint32_t R = atoi(argv[1]) & 0xFF;
    uint32_t G = atoi(argv[2]) & 0xFF;
    uint32_t B = atoi(argv[3]) & 0xFF;

    uint32_t RGB = R << 16 | G << 8 | B;
    console_windows_printf(MENU_WINDOW, "setting LED to 0x%x...\n", RGB);
    bool retc = rgb_led_write(RGB);
    console_windows_printf(MENU_WINDOW, "set_color: %s\n", retc ? "OK" : "Failed");
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

static menu_item_t menu_item_rgb_led = {
    .func = rgb_led_menu,
    .cmd  = "",
    .desc = ""
};

static menu_item_t menu_item_exit = {
    .func = exit_menu,
    .cmd  = "exit",
    .desc = "exit menu"
};

static menu_item_t menu_item_set_color = {
    .func = set_color,
    .cmd  = "color",
    .desc = "set color to <R> <G> <B> (0-255)"
};

static menu_item_t* menu_item_list[] = 
{
    &menu_item_exit,
    &menu_item_set_color,
};

static void show_help(void)
{
    console_windows_printf(MENU_WINDOW, "\nrgb_led menu\n");
    static const int list_length = sizeof(menu_item_list) / sizeof(menu_item_list[0]);

    for (int i = 0; i < list_length; i++)
    {
        console_windows_printf(MENU_WINDOW, "%-20s: %s\n", menu_item_list[i]->cmd, menu_item_list[i]->desc);
    }
}

menu_item_t* rgb_led_menu(int argc, char* argv[])
{
    // check for blank line which is an indication to display the help menu
    if (argc == 0 || argv == NULL)
    {
        show_help();
        return &menu_item_rgb_led;
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

void rgb_led_menu_set_parent(menu_function_t menu)
{
    parent_menu = menu;
}
