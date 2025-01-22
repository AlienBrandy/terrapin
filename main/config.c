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
#include "sys/param.h"

#define CONFIG_PATH FILESYSTEM_MOUNT_PATH "/configs.csv"
#define CONFIG_VALUE_MAX_BYTES 64

typedef struct {
    char* name;
    char* val;
} CONFIG_ENTRY_T;

static CONFIG_ENTRY_T default_configs[CONFIG_KEY_MAX] = 
{
    #define X(A,B) [A].name = #A, [A].val = B,
    CONFIG_KEYS
    #undef X
};

static char value_store[CONFIG_KEY_MAX][CONFIG_VALUE_MAX_BYTES];

static CONFIG_ENTRY_T configs[CONFIG_KEY_MAX] = 
{
    #define X(A,B) [A].name = #A, [A].val = value_store[A],
    CONFIG_KEYS
    #undef X
};

static void populate_from_defaults(void)
{
    for (int idx = 0; idx < CONFIG_KEY_MAX; idx++)
    {
        int length = strlen(default_configs[idx].val);
        length = MIN(length, CONFIG_VALUE_MAX_BYTES);
        strncpy(configs[idx].val, default_configs[idx].val, length);
    }
}

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
        char* token = strtok(buf, delims);
        if (token)
        {
            // search for matching key name in configs
            for (int idx = 0; idx < CONFIG_KEY_MAX; idx++)
            {
                if (strcmp(configs[idx].name, token) == 0)
                {
                    // found match. Second part of string contains value
                    token = strtok(0, delims);
                    if (token)
                    {
                        // copy value and ensure we leave a null terminator
                        strncpy(configs[idx].val, token, CONFIG_VALUE_MAX_BYTES - 1);
                    }
                    break;
                }
            }
        }
    }
    fclose(fp);
}

static void save_values_to_file(void)
{
    FILE* fp = fopen(CONFIG_PATH, "w");
    if (fp == NULL)
    {
        ESP_LOGW(PROJECT_NAME, "config::save_values_to_file(): could not create file.");
        return;
    }
    for (int idx = 0; idx < CONFIG_KEY_MAX; idx++)
    {
        if (fprintf(fp, "%s,%s\n", configs[idx].name, configs[idx].val) < 0)
        {
            ESP_LOGW(PROJECT_NAME, "config::save_values_to_file(): write error.");
            break;
        }
    }
    fclose(fp);
}

void config_init(void)
{
    populate_from_defaults();
    restore_values_from_file();
}

bool config_set(CONFIG_KEY_T key, const char* value)
{
    if (key >= CONFIG_KEY_MAX)
    {
        return false;
    }
    if (strlen(value) > CONFIG_VALUE_MAX_BYTES)
    {
        return false;
    }

    strncpy(configs[key].val, value, CONFIG_VALUE_MAX_BYTES);
    save_values_to_file();

    return true;
}

bool config_get(CONFIG_KEY_T key, const char** value)
{
    if (key >= CONFIG_KEY_MAX)
    {
        return false;
    }

    *value = configs[key].val;
    return true;
}

bool config_get_boolean(CONFIG_KEY_T key)
{
    const char* value;
    if (!config_get(key, &value))
    {
        return false;
    }
    return (value[0] == '1') || (value[0] == 't') || (value[0] == 'T');
}

const char* config_get_name(CONFIG_KEY_T key)
{
    if (key < CONFIG_KEY_MAX)
    {
        return configs[key].name;
    }
    return "invalid key";
}
