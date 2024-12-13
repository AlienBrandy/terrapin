/**
 * wifi.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: MIT
 */

#include "wifi.h"
#include "esp_log.h"

WIFI_ERR_T wifi_init(void)
{
    return WIFI_ERR_NONE;
}

WIFI_ERR_T wifi_connect(const char* ssid, const char* password, int timeout_secs)
{
    ESP_LOGI(PROJECT_NAME, "wifi_connect: %s, %s, %d", ssid, password, timeout_secs);        
    return WIFI_ERR_NONE;
}

WIFI_ERR_T wifi_disconnect(void)
{
    return WIFI_ERR_NONE;
}