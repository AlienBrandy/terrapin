/**
 * main.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "esp_log.h"
#include "console.h"
#include "filesystem.h"
#include "network_manager.h"

void app_main(void)
{
    // initialize flash filesystem
    FILESYSTEM_ERR_T err = filesystem_init();
    if (err != FILESYSTEM_ERR_NONE)
    {
        ESP_LOGE(PROJECT_NAME, "filesystem_init() failed");        
        return;
    }

    // initialize debug console
    CONSOLE_ERR_T console_err = console_init();
    if (console_err != CONSOLE_ERR_NONE)
    {
        ESP_LOGE(PROJECT_NAME, "console_init() failed");        
        return;
    }

    // start debug console thread
    console_err = console_start();
    if (console_err != CONSOLE_ERR_NONE)
    {
        ESP_LOGE(PROJECT_NAME, "console_start() failed");        
        return;
    }

    // start network manager
    NETWORK_MANAGER_ERR_T network_manager_err = network_manager_init(WAIT);
    if (network_manager_err != NETWORK_MANAGER_ERR_NONE)
    {
        ESP_LOGE(PROJECT_NAME, "network_manager_init() failed");
        return;
    }

    while (1) {}
}
