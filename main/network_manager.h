/**
 * network_manager.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <stdbool.h>
#include "network_manager.def"

/**
 * @brief network manager return codes
 */
typedef enum {
    #define X(A, B) A,
    NETWORK_MANAGER_ERROR_LIST
    #undef X
    NETWORK_MANAGER_ERR_MAX
} NETWORK_MANAGER_ERR_T;

#define WAIT true
#define NOWAIT false

NETWORK_MANAGER_ERR_T network_manager_init(bool wait);
NETWORK_MANAGER_ERR_T network_manager_connect(bool wait);
NETWORK_MANAGER_ERR_T network_manager_connect_to(char* ssid, char* pwd, bool wait);
NETWORK_MANAGER_ERR_T network_manager_disconnect(bool wait);
const char* network_manager_get_error_string(NETWORK_MANAGER_ERR_T code);
const char* network_manager_get_current_state(void);
