#include <stdint.h>
#include <string.h>
#include "main_menu.h"
#include "wifi_menu.h"
#include "console_windows.h"

static menu_item_t* item_list = NULL;
static int list_size = 0;

static void show_help(void)
{
    // first item by convention is the name of the menu and gets unique formatting
    console_windows_printf(CONSOLE_WINDOW_2, "\nmain menu\n");

    for (int i = 0; i < list_size; i++)
    {
        console_windows_printf(CONSOLE_WINDOW_2, "%-20s: %s\n", item_list[i].cmd, item_list[i].desc);
    }
}

// forward declaration
static menu_item_t menu_item_main;

menu_item_t* main_menu(int argc, char* argv[])
{
    // display help menu if command parameter is empty
    if (argc == 0 || argv == NULL)
    {
        show_help();
        return &menu_item_main;
    }

    // search for matching command in list of registered menu items
    for (int i = 0; i < list_size; i++)
    {
        if (strcmp(argv[0], item_list[i].cmd) == 0)
        {
            // match found, execute menu item function.    
            return (*item_list[i].func)(argc, argv);
        }
    }
    console_windows_printf(CONSOLE_WINDOW_2, "unknown command [%s]\n", argv[0]);
    return NULL;
}

static menu_item_t* say_hello(int argc, char* argv[])
{
    console_windows_printf(CONSOLE_WINDOW_2, "hello back!\n");
    return NULL;
}

static menu_item_t* say_goodbye(int argc, char* argv[])
{
    console_windows_printf(CONSOLE_WINDOW_2, "see ya!\n");
    return NULL;
}

static menu_item_t* show_wifi_menu(int argc, char* argv[])
{
    // switch menus
    return wifi_menu(0, NULL);
}

static menu_item_t menu_item_main = {
    .func = main_menu,
    .cmd  = "",
    .desc = ""
};

static menu_item_t menu_item_hello = {
    .func = say_hello,
    .cmd  = "hello",
    .desc = "say hello"
};

static menu_item_t menu_item_goodbye = {
    .func = say_goodbye,
    .cmd  = "goodbye",
    .desc = "say goodbye"
};

static menu_item_t menu_item_wifi = {
    .func = show_wifi_menu,
    .cmd  = "wifi",
    .desc = "wifi submenu"
};

menu_item_t* main_menu_init(void)
{
    menu_register_item(&menu_item_hello, &item_list, &list_size);
    menu_register_item(&menu_item_goodbye, &item_list, &list_size);
    menu_register_item(&menu_item_wifi, &item_list, &list_size);

    wifi_menu_init();

    return &menu_item_main;
}

