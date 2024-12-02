#pragma once

/**
 * @brief path to non-volatile filesystem for storing command history
 */
#define FILESYSTEM_MOUNT_PATH "/data"

typedef enum {
    FILESYSTEM_ERR_NONE,
    FILESYSTEM_ERR_INIT_NVS_FAILED,
    FILESYSTEM_ERR_INIT_FS_FAILED,
} FILESYSTEM_ERR_T;

FILESYSTEM_ERR_T filesystem_init(void);