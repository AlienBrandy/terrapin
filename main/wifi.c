/**
 * wifi.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: MIT
 */

#include "wifi.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

static EventGroupHandle_t wifi_event_group;

typedef enum {
    WIFI_EVENT_CONNECTED     = BIT0,
    WIFI_EVENT_SCAN_COMPLETE = BIT1,
} WIFI_EVENT_BITS;

#define MAX_AP_RECORDS 10
static uint16_t num_ap_records = 0;
static wifi_ap_record_t ap_records[MAX_AP_RECORDS];

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(wifi_event_group, WIFI_EVENT_CONNECTED);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_EVENT_CONNECTED);
    }
    else if (event_base == IP_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_EVENT_SCAN_COMPLETE);
    }
}

WIFI_ERR_T wifi_init(void)
{
    esp_err_t esp_err;
    static bool initialized = false;
    if (initialized)
    {
        return WIFI_ERR_NONE;
    }

    // set log level to suppress messages below warning level
    esp_log_level_set("wifi", ESP_LOG_WARN);

    // initialize TCP/IP stack
    esp_err = esp_netif_init();
    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_netif_init failed: %d\n", esp_err);
        return WIFI_ERR_INIT_FAILED;
    }

    // create an event group for signaling wifi events from the event handler
    wifi_event_group = xEventGroupCreate();

    // create wifi station
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL)
    {
        ESP_LOGW("wifi", "esp_netif_create_default_wifi_sta failed: %d\n", esp_err);
        return WIFI_ERR_INIT_FAILED;
    }

    // initialize wifi driver and start wifi task
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err = esp_wifi_init(&cfg);
    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_wifi_init failed: %d\n", esp_err);
        return WIFI_ERR_INIT_FAILED;
    }

    // set event handlers for connect and disconnect
    esp_err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_handler, NULL);
    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_event_handler_register failed: %d\n", esp_err);
        return WIFI_ERR_INIT_FAILED;
    }
    esp_err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_event_handler_register failed: %d\n", esp_err);
        return WIFI_ERR_INIT_FAILED;
    }

    // use RAM to store wifi configuration 
    esp_err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_wifi_set_storage failed: %d\n", esp_err);
        return WIFI_ERR_INIT_FAILED;
    }

    // set wifi mode to station
    esp_err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_wifi_set_mode failed: %d\n", esp_err);
        return WIFI_ERR_INIT_FAILED;
    }

    // start wifi according to current configuration
    esp_err = esp_wifi_start();
    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_wifi_start failed: %d\n", esp_err);
        return WIFI_ERR_INIT_FAILED;
    }

    initialized = true;
    return WIFI_ERR_NONE;
}

WIFI_ERR_T wifi_scan(void)
{
    esp_err_t esp_err;

    // esp_wifi_set_country();

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 200,
        .scan_time.active.max = 300,
        .show_hidden = 1
    };
    esp_err = esp_wifi_scan_start(&scan_config, true);

    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_wifi_scan_start failed: %d\n", esp_err);
        return WIFI_ERR_SCAN_FAILED;
    }

    num_ap_records = MAX_AP_RECORDS;
    esp_err = esp_wifi_scan_get_ap_records(&num_ap_records, ap_records);
    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_wifi_scan_get_ap_records failed: %d\n", esp_err);
        return WIFI_ERR_SCAN_FAILED;
    }

    return WIFI_ERR_NONE;
}

WIFI_ERR_T wifi_connect(const char* ssid, const char* password, uint32_t timeout_msec)
{
    esp_err_t esp_err;
    wifi_config_t wifi_config = { 0 };
    strlcpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *) wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_wifi_start failed: %d\n", esp_err);
        return WIFI_ERR_CONNECT_FAILED;
    }

    esp_err = esp_wifi_connect();
    if (esp_err != ESP_OK)
    {
        ESP_LOGW("wifi", "esp_wifi_connect failed: %d\n", esp_err);
        return WIFI_ERR_CONNECT_FAILED;
    }

    int bits = xEventGroupWaitBits(wifi_event_group, WIFI_EVENT_CONNECTED,
                                   pdFALSE, pdTRUE, timeout_msec / portTICK_PERIOD_MS);

    if (bits & WIFI_EVENT_CONNECTED)
    {
        return WIFI_ERR_NONE;
    }

    return WIFI_ERR_CONNECTION_TIMEOUT;
}

WIFI_ERR_T wifi_disconnect(void)
{
    return esp_wifi_disconnect();
}

const char* wifi_get_error_string(WIFI_ERR_T code)
{
    static const char * error_string[WIFI_ERR_MAX] = 
    {
        #define X(A, B) B,
        WIFI_ERROR_LIST
        #undef X
    };
    if (code < WIFI_ERR_MAX)
    {
        return error_string[code];        
    }
    return "unknown error";
}

uint16_t wifi_get_number_of_networks(void)
{
    return num_ap_records;
}

WIFI_ERR_T wifi_get_network_record(uint16_t index, wifi_network_record_t* record)
{
    assert(record != NULL);

    if (index > num_ap_records)
    {   
        record->ssid[0] = 0;
        record->rssi = -127;
        return WIFI_ERR_INVALID_RECORD_INDEX;
    }
    memcpy(record->ssid, ap_records[index].ssid, sizeof(record->ssid));
    record->rssi = ap_records[index].rssi;
    return WIFI_ERR_NONE;
}
