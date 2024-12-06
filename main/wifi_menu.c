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
    // first item by convention is the name of the menu and gets unique formatting
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

static menu_item_t* connect(int argc, char* argv[])
{
    if (argc < 3)
    {
        console_windows_printf(CONSOLE_WINDOW_2, "connect: missing param(s)\n");
        return NULL;
    }

    char* ssid = argv[1];
    char* pwd  = argv[2];

    console_windows_printf(CONSOLE_WINDOW_2, "connecting... ");
    WIFI_ERR_T retc = wifi_connect(ssid, pwd, 10);
    console_windows_printf(CONSOLE_WINDOW_2, "%d.\n", retc);
    return NULL;
}

static menu_item_t* disconnect(int argc, char* argv[])
{
    console_windows_printf(CONSOLE_WINDOW_2, "disconnecting... ");
    WIFI_ERR_T retc = wifi_disconnect();
    console_windows_printf(CONSOLE_WINDOW_2, "%d.\n", retc);
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
    .cmd  = "x",
    .desc = "back to main menu"
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
    menu_register_item(&menu_item_connect, &item_list, &list_size);
    menu_register_item(&menu_item_disconnect, &item_list, &list_size);

    return &menu_item_wifi;
}

