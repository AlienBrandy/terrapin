/**
 * known_networks.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "known_networks.def"

#define KNOWN_NETWORKS_MAX_SSID 33 // ssid of network, max 32 bytes plus null character
#define KNOWN_NETWORKS_MAX_PWD  64 // password associated with network, max 63 bytes plus null character

/**
 * @brief path to non-volatile storage for storing list of networks
 */
#define KNOWN_NETWORKS_MOUNT_PATH "/networks"

typedef struct {
    char ssid[KNOWN_NETWORKS_MAX_SSID];
    char pwd[KNOWN_NETWORKS_MAX_PWD];
} known_network_entry_t;

/**
 * @brief known network function return codes
 */
typedef enum {
    #define X(A, B) A,
    KNOWN_NETWORKS_ERROR_LIST
    #undef X
    KNOWN_NETWORKS_ERR_MAX
} KNOWN_NETWORKS_ERR_T;

/**
 * @brief initialize the known_networks module.
 * 
 * Restores list of networks from non-volatile storage
 */
KNOWN_NETWORKS_ERR_T known_networks_init(void);

/**
 * @brief add a network to the list.
 * 
 * if ssid is already on the list, then the old entry is replaced.
 */
KNOWN_NETWORKS_ERR_T known_networks_add(char* ssid, char* password);

/**
 * @brief removes a network from the list.
 */
KNOWN_NETWORKS_ERR_T known_networks_remove(char* ssid);

/**
 * @brief retrieve an entry from the list.
 */
KNOWN_NETWORKS_ERR_T known_networks_get_entry(uint8_t index, known_network_entry_t* entry);

/**
 * @brief return the number of known networks.
 */
uint8_t known_networks_get_number_of_entries(void);

/**
 * @brief get string for return code.
 */
const char* known_networks_get_error_string(KNOWN_NETWORKS_ERR_T code);
