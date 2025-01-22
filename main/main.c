/**
 * main.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "esp_log.h"
#include "esp_event.h"
#include "console.h"
#include "filesystem.h"
#include "network_manager.h"
#include "config.h"
#include "terrapin.h"

void app_main(void)
{
    // initialize flash filesystem
    FILESYSTEM_ERR_T err = filesystem_init();
    if (err != FILESYSTEM_ERR_NONE)
    {
        ESP_LOGE(PROJECT_NAME, "filesystem_init() failed");        
        return;
    }

    // initialize config module
    config_init();

    // create the default event loop for system events
    esp_err_t esp_err = esp_event_loop_create_default();
    if (esp_err != ESP_OK)
    {
        ESP_LOGW(PROJECT_NAME, "esp_event_loop_create_default failed: %d\n", esp_err);
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

    // set log level to show warnings and errors
    esp_log_level_set(PROJECT_NAME, ESP_LOG_WARN);
    
    // project-specific initialization
    if (!terrapin_init())
    {
        ESP_LOGE(PROJECT_NAME, "terrapin_init() failed");
    }

    while (1) {}
}
