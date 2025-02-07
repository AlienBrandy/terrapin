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
#include <stdbool.h>

/**
 * @brief a configuration entry
 */
typedef struct {
    char* name;
    char* val;
} CONFIG_ENTRY_T;

/**
 * @brief initialize the config module.
 * 
 * Call this before using any other API method. Config values are
 * populated from non-volatile storage.
 */
bool config_init(CONFIG_ENTRY_T* configs, int nConfigs);

/**
 * @brief change a configuration setting.
 * 
 * @param key the setting to change.
 * @param value the new value, restricted to the max size of a value string.
 * @returns true of the new setting was recorded, false otherwise.
 */
bool config_set(const char* key, const char* value);

/**
 * @brief retrieve a configuration setting.
 * 
 * @param key the setting to retrieve.
 * @param value receives a pointer to the string associated with the key.
 * @returns true if setting was retrieved, false otherwise.
 */
bool config_get_value(const char* key, const char** value);

/**
 * @brief retrieve a boolean configuration setting.
 * 
 * @param key the setting to retrieve.
 * @returns the value interpreted as a boolean datatype. Return code is true
 * if the value begins with '1', 't', or 'T'. Method returns false otherwise.
 */
bool config_get_boolean(const char* key);

/**
 * @brief retrieve the key of the config entry with the given index.
 * 
 * @param index the config index
 * @returns the name associated with the given index, or NULL if index is invalid.
 */
const char* config_get_key(int index);