#pragma once
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"

typedef enum {
    WIFI_ERR_NONE,
    WIFI_ERR_TIMEOUT,
} WIFI_ERR_T;

WIFI_ERR_T wifi_init(void);
WIFI_ERR_T wifi_connect(const char* ssid, const char* password, int timeout_secs);
WIFI_ERR_T wifi_disconnect(void);