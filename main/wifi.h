/**
 * wifi.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "wifi.def"

typedef enum {
    #define X(A, B) A,
    WIFI_ERROR_LIST
    #undef X
    WIFI_ERR_MAX
} WIFI_ERR_T;

WIFI_ERR_T wifi_init(void);
WIFI_ERR_T wifi_scan(void);
WIFI_ERR_T wifi_connect(const char* ssid, const char* password, uint32_t timeout_msec);
WIFI_ERR_T wifi_disconnect(void);
const char* wifi_get_error_string(WIFI_ERR_T code);

