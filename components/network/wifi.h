/**
 * wifi.h
 * 
 * The wifi module establishes network connectivity through the wireless interface.
 * The module provides functions to scan for available networks, connect to a network,
 * and disconnect from a network. Once running, the underlying network library signals
 * the application through event handlers when the connection status changes. The event
 * handlers set event bits in a FreeRTOS event group, which the application then waits
 * on to synchronize with the interface.
 * 
 * Other modules can also register event handlers to get signaled when the connection
 * status changes. The event base and the signals are defined in the esp wifi library.
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "wifi.def"

#define WIFI_SSID_FIELD_SIZE 33

/**
 * @brief network data record
 */
typedef struct {
    char ssid[WIFI_SSID_FIELD_SIZE];
    int8_t  rssi;
} wifi_network_record_t;

/**
 * @brief wifi function return codes
 */
typedef enum {
    #define X(A, B) A,
    WIFI_ERROR_LIST
    #undef X
    WIFI_ERR_MAX
} WIFI_ERR_T;

/**
 * @brief Initialize the wifi component and the underlying TCP/IP stack.
 * 
 * Call this once at startup before calling any other wifi functions.
 * 
 * @return WIFI_ERR_NONE on success, or an error code on failure. 
 */
WIFI_ERR_T wifi_init(void);

/**
 * @brief Scan for available networks.
 * 
 * The list of detected access points is saved in a file-scope array of wifi_ap_record_t structures. 
 * 
 * @return WIFI_ERR_NONE on success, or an error code on failure. 
 */
WIFI_ERR_T wifi_scan(void);

/**
 * @brief Connect to a wifi network.
 * 
 * @param ssid The network name.
 * @param password The network password.
 * @param timeout_msec The maximum time to wait for a connection to be established.
 * @returns WIFI_ERR_NONE on success, or an error code on failure.
 */
WIFI_ERR_T wifi_connect(const char* ssid, const char* password, uint32_t timeout_msec);

/**
 * @brief Disconnect from the current wifi network.
 */
void wifi_disconnect(void);

/**
 * @brief Get the number of networks detected by the last wifi_scan() call.
 * @returns The number of networks detected.
 */
uint16_t wifi_get_number_of_networks(void);

/**
 * @brief Get the network record of the specified index.
 * 
 * This indexes into the list of networks detected by the last wifi_scan() call.
 * 
 * @param index The index of the network record to retrieve.
 * @param record Receives the network record.
 * @returns WIFI_ERR_NONE on success, or an error code on failure.
 */
WIFI_ERR_T wifi_get_network_record(uint16_t index, wifi_network_record_t* record);

/**
 * @brief Get a string representation of a wifi error code.
 * 
 * @param code The error code.
 * @returns A string representation of the error code.
 */
const char* wifi_get_error_string(WIFI_ERR_T code);

