#include "known_networks.h"
#include "filesystem.h"
#include <string.h>
#include <stdbool.h>

#define KNOWN_NETWORKS_MAX_ENTRIES 10
#define KNOWN_NETWORKS_PATH FILESYSTEM_MOUNT_PATH "/nets.csv"

static known_network_entry_t networks[KNOWN_NETWORKS_MAX_ENTRIES];
static uint8_t num_networks = 0;

static void clear_network_list(void)
{
    memset(networks, 0, sizeof(networks));
    num_networks = 0;
}

static void fill_network_list_from_file(void)
{
    FILE* fp = fopen(KNOWN_NETWORKS_PATH, "r");
    if (fp == NULL)
    {
        // no file; no networks.
        return;
    }

    // read one line at a time from file into a buffer
    #define BUF_SIZE sizeof(known_network_entry_t)
    static char buf[BUF_SIZE];
    while (fgets(buf, BUF_SIZE, fp) && (num_networks < KNOWN_NETWORKS_MAX_ENTRIES))
    {
        static const char *delims = ",\n";
        char* token = strtok(buf, delims);
        if (token)
        {
            // copy network name from string
            strncpy(networks[num_networks].ssid, token, KNOWN_NETWORKS_MAX_SSID);
            networks[num_networks].ssid[KNOWN_NETWORKS_MAX_SSID - 1] = '\0';
            token = strtok(0, delims);
            if (token)
            {
                // copy password from string
                strncpy(networks[num_networks].pwd, token, KNOWN_NETWORKS_MAX_PWD);
                networks[num_networks].pwd[KNOWN_NETWORKS_MAX_PWD - 1] = '\0';
            }
            num_networks++;
        }
    }
    fclose(fp);
}

static bool save_network_list_to_file(void)
{
    FILE* fp = fopen(KNOWN_NETWORKS_PATH, "w");
    if (fp == NULL)
    {
        // filesystem error
        return false;
    }

    // write one entry at a time from list to file
    for (int i = 0; i < num_networks; i++)
    {
        int num_written = fprintf(fp, "%s,%s\n", networks[i].ssid, networks[i].pwd);
        if (num_written < 0)
        {
            // error writing to stream
            fclose(fp);
            return false;
        }
    }
    fclose(fp);
    return true;
}

static void add_entry(char* ssid, char* pwd)
{
    if (num_networks == KNOWN_NETWORKS_MAX_ENTRIES)
    {
        // if the list is full, no need to shift the last entry,
        // it will just get overwritten.
        num_networks--;
    }

    // shift all entries to make room for the new one
    for (int i = num_networks - 1; i >= 0; i--)
    {
        if (i == KNOWN_NETWORKS_MAX_ENTRIES - 1)
        {
            // when the list is full the oldest entry just gets overwritten; there's nowhere to copy it.
            continue;
        }
        memcpy(networks[i + 1].ssid, networks[i].ssid, KNOWN_NETWORKS_MAX_SSID);
        memcpy(networks[i + 1].pwd, networks[i].pwd, KNOWN_NETWORKS_MAX_PWD);
    }

    // insert the new entry
    strncpy(networks[0].ssid, ssid, KNOWN_NETWORKS_MAX_SSID);
    networks[0].ssid[KNOWN_NETWORKS_MAX_SSID - 1] = '\0';
    strncpy(networks[0].pwd, pwd, KNOWN_NETWORKS_MAX_PWD);
    networks[0].pwd[KNOWN_NETWORKS_MAX_PWD - 1] = '\0';

    // update the list size
    num_networks++;
}

static bool remove_entry(char* ssid)
{
    // search for entry in table
    uint8_t index_to_remove = 0;
    for (index_to_remove = 0; index_to_remove < KNOWN_NETWORKS_MAX_ENTRIES; index_to_remove++)
    {
        if (strncmp(networks[index_to_remove].ssid, ssid, KNOWN_NETWORKS_MAX_SSID) == 0)
        {
            // found entry; shift older entries up the list to overwrite it.
            for (uint8_t index_to_shift = index_to_remove + 1; index_to_shift < KNOWN_NETWORKS_MAX_ENTRIES; index_to_shift++)
            {
                uint8_t index_to_overwrite = index_to_shift - 1;
                memcpy(networks[index_to_overwrite].ssid, networks[index_to_shift].ssid, KNOWN_NETWORKS_MAX_SSID);
                memcpy(networks[index_to_overwrite].pwd, networks[index_to_shift].pwd, KNOWN_NETWORKS_MAX_PWD);
            }

            // update list size
            num_networks--;
            return true;
        }
    }
    return false;
}

uint8_t known_networks_get_number_of_entries(void)
{
    return num_networks;
}

KNOWN_NETWORKS_ERR_T known_networks_init(void)
{
    clear_network_list();
    fill_network_list_from_file();
    return KNOWN_NETWORKS_ERR_NONE;
}

KNOWN_NETWORKS_ERR_T known_networks_remove(char* ssid)
{
    if (ssid == NULL)
    {
        return KNOWN_NETWORKS_ERR_BAD_ARGUMENT;        
    }

    if (!remove_entry(ssid))
    {
        return KNOWN_NETWORKS_ERR_NOT_FOUND;
    }

    if (!save_network_list_to_file())
    {
        return KNOWN_NETWORKS_ERR_SAVE_FAILED;
    }

    return KNOWN_NETWORKS_ERR_NONE;
}

KNOWN_NETWORKS_ERR_T known_networks_add(char* ssid, char* password)
{
    if ((ssid == NULL) || (password == NULL))
    {
        return KNOWN_NETWORKS_ERR_BAD_ARGUMENT;
    }

    // remove entry if it already exists to prevent duplicates
    // and then add to the front as the freshest entry.
    remove_entry(ssid);
    add_entry(ssid, password);
    if (!save_network_list_to_file())
    {
        return KNOWN_NETWORKS_ERR_SAVE_FAILED;
    }

    return KNOWN_NETWORKS_ERR_NONE;
}

KNOWN_NETWORKS_ERR_T known_networks_get_entry(uint8_t index, known_network_entry_t* entry)
{
    if (entry == NULL)
    {
        return KNOWN_NETWORKS_ERR_BAD_ARGUMENT;
    }
    if (index >= num_networks)
    {
        entry->ssid[0] = 0;
        entry->pwd[0] = 0;
        return KNOWN_NETWORKS_ERR_INVALID_INDEX;
    }

    *entry = networks[index];
    return KNOWN_NETWORKS_ERR_NONE;
}

const char* known_networks_get_error_string(KNOWN_NETWORKS_ERR_T code)
{
    static const char * error_string[KNOWN_NETWORKS_ERR_MAX] = 
    {
        #define X(A, B) B,
        KNOWN_NETWORKS_ERROR_LIST
        #undef X
    };
    if (code < KNOWN_NETWORKS_ERR_MAX)
    {
        return error_string[code];        
    }
    return "unknown error";
}