#include <stdint.h>
#include <stdbool.h>
#include "filesystem.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"

/**
 * 
 */
static bool initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        err = nvs_flash_erase();
        if (err != ESP_OK)
        {
            ESP_LOGE(PROJECT_NAME, "nvs_flash_erase() failed: %s", esp_err_to_name(err));
            return false;
        }
        err = nvs_flash_init();
        if (err != ESP_OK)
        {
            ESP_LOGE(PROJECT_NAME, "nvs_flash_init() failed: %s", esp_err_to_name(err));
            return false;            
        }
    }
    else if (err != ESP_OK)
    {
        ESP_LOGE(PROJECT_NAME, "nvs_flash_init() failed: %s", esp_err_to_name(err));
        return false;            
    }

    ESP_LOGI(PROJECT_NAME, "Flash storage initialized");
    return true;
}

/**
 * 
 */
static bool initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };

    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(FILESYSTEM_MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(PROJECT_NAME, "esp_vfs_fat_spiflash_mount_rw_wl() failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(PROJECT_NAME, "Filesystem initialized");
    return true;
}

FILESYSTEM_ERR_T filesystem_init(void)
{
    if (!initialize_nvs())
        return FILESYSTEM_ERR_INIT_NVS_FAILED;

    if (!initialize_filesystem())
        return FILESYSTEM_ERR_INIT_FS_FAILED;        

    return FILESYSTEM_ERR_NONE;
}