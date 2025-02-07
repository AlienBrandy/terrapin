/**
 * config.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config.h"
#include "filesystem.h"
#include "string.h"
#include "esp_log.h"
#include "min_max.h"

#define CONFIG_PATH FILESYSTEM_MOUNT_PATH "/configs.csv"
#define CONFIG_VALUE_MAX_BYTES 64

/**
 * @brief list of default configs, provided to init function
 */ 
static CONFIG_ENTRY_T* _default_configs = NULL;

/**
 * @brief list of configs, allocated and populated during initialization
 */
static CONFIG_ENTRY_T* _configs = NULL;

/**
 * @brief number of config key/value pairs, provided to init function
 */
static int _nConfigs = 0;

/**
 * @brief overwrite config values with defaults
 */
static void populate_values_from_defaults(void)
{
    for (int idx = 0; idx < _nConfigs; idx++)
    {
        strncpy(_configs[idx].val, _default_configs[idx].val, CONFIG_VALUE_MAX_BYTES - 1);
    }
}

/**
 * @brief get the index of the config entry associated with the given key
 */
static int get_index_for_key(const char* key)
{
    // search for matching key name in configs
    for (int idx = 0; idx < _nConfigs; idx++)
    {
        if (strcmp(_configs[idx].name, key) == 0)
        {
            return idx;
        }
    }

    // not found
    return -1;
}

/**
 * @brief overwrite config values with values from non-volatile memory
 */
static void restore_values_from_file(void)
{
    FILE* fp = fopen(CONFIG_PATH, "r");
    if (fp == NULL)
    {
        // no saved configs
        return;
    }

    // read one line at a time from file into a buffer
    #define BUF_SIZE 1024
    static char buf[BUF_SIZE];
    while (fgets(buf, BUF_SIZE, fp))
    {
        // first part of string is key name
        static const char *delims = ",\n";
        char* key = strtok(buf, delims);
        if (key)
        {
            // find index for given key
            int idx = get_index_for_key(key);
            if (idx != -1)
            {
                // found match. Second part of string contains value
                char* val = strtok(0, delims);
                if (val)
                {
                    // copy value and ensure we leave a null terminator
                    strncpy(_configs[idx].val, val, CONFIG_VALUE_MAX_BYTES - 1);
                }
            }
        }
    }
    fclose(fp);
}

/**
 * @brief save config entries to non-volatile storage
 */
static void save_values_to_file(void)
{
    FILE* fp = fopen(CONFIG_PATH, "w");
    if (fp == NULL)
    {
        ESP_LOGW(PROJECT_NAME, "config::save_values_to_file(): could not create file.");
        return;
    }
    for (int idx = 0; idx < _nConfigs; idx++)
    {
        if (fprintf(fp, "%s,%s\n", _configs[idx].name, _configs[idx].val) < 0)
        {
            ESP_LOGW(PROJECT_NAME, "config::save_values_to_file(): write error.");
            break;
        }
    }
    fclose(fp);
}

bool config_init(CONFIG_ENTRY_T* default_configs, int nConfigs)
{
    if (nConfigs < 1)
    {
        return false;
    }
    if (default_configs == NULL)
    {
        return false;
    }
    _default_configs = default_configs;
    _nConfigs = nConfigs;

    // allocate memory for config entries
    _configs = calloc(nConfigs, sizeof(CONFIG_ENTRY_T));
    if (_configs == NULL)
    {
        return false;
    }

    // a config entry contains pointers to the config name and value.
    // we need to allocate memory to hold the config values
    char* config_value_store = calloc(nConfigs, CONFIG_VALUE_MAX_BYTES);
    if (config_value_store == NULL)
    {
        free(_configs);
        return false;
    }

    // assign config entry pointers to the names and value storage
    for (int idx = 0; idx < nConfigs; idx++)
    {
        _configs[idx].name = _default_configs[idx].name;
        _configs[idx].val  = config_value_store + (idx * CONFIG_VALUE_MAX_BYTES);
    }

    populate_values_from_defaults();
    restore_values_from_file();
    return true;
}

bool config_set(const char* key, const char* value)
{
    int idx = get_index_for_key(key);
    if (idx == -1)
    {
        return false;
    }
    if (strlen(value) > CONFIG_VALUE_MAX_BYTES - 1)
    {
        return false;
    }

    strncpy(_configs[idx].val, value, CONFIG_VALUE_MAX_BYTES - 1);
    save_values_to_file();
    return true;
}

bool config_get_value(const char* key, const char** value)
{
    int idx = get_index_for_key(key);
    if (idx == -1)
    {
        return false;
    }

    *value = _configs[idx].val;
    return true;
}

bool config_get_boolean(const char* key)
{
    const char* value;
    if (!config_get_value(key, &value))
    {
        return false;
    }
    return (value[0] == '1') || (value[0] == 't') || (value[0] == 'T');
}

const char* config_get_key(int index)
{
    if (index < _nConfigs)
    {
        return _configs[index].name;
    }

    return NULL;
}
