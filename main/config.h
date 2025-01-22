/**
 * config.h
 * 
 * The config module provides non-volatile storage for system settings. Configs are
 * prepopulated with hard-coded default values. The defaults can be overwritten using
 * the set method which commits the new values to non-volatile memory. On startup,
 * the non-volatile values will be restored over the defaults. 
 * 
 * All config values are stored and returned as strings. Convenience functions translate
 * the string to common datatypes, however the datatype is not inherent to the config, 
 * so it's up to the user to utilize the proper translation.
 * 
 * The config name is stored along with the value. If configs are added, deleted, or
 * rearranged with new firmware updates, then the stored values will still be associated
 * with the proper keys.
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "config.def"
#include <stdbool.h>

/**
 * @brief config keys
 */
typedef enum {
    #define X(A,B) A,
    CONFIG_KEYS
    #undef X
    CONFIG_KEY_MAX
} CONFIG_KEY_T;

/**
 * @brief initialize the config module.
 * 
 * Call this before using any other API method. Config values are
 * populated from non-volatile storage.
 */
void config_init(void);

/**
 * @brief change a configuration setting.
 * 
 * @param key the setting to change.
 * @param value the new value, restricted to the max size of a value string.
 * @returns true of the new setting was recorded, false otherwise.
 */
bool config_set(CONFIG_KEY_T key, const char* value);

/**
 * @brief retrieve a configuration setting.
 * 
 * @param key the setting to retrieve.
 * @param value receives the value associated with the key.
 * @returns true if setting was retrieved, false otherwise.
 */
bool config_get(CONFIG_KEY_T key, const char** value);

/**
 * @brief retrieve a boolean configuration setting.
 * 
 * @param key the setting to retrieve.
 * @returns the value interpreted as a boolean datatype. Return code is true
 * if the value begins with '1', 't', or 'T'. Method returns false otherwise.
 */
bool config_get_boolean(CONFIG_KEY_T key);

/**
 * @brief retrieve the name associated with the configuration setting.
 * 
 * @param key the config setting to retrieve the name of
 */
const char* config_get_name(CONFIG_KEY_T key);