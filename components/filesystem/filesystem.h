/**
 * filesystem.h
 * 
 * This component is a thin wrapper around the ESP-IDF filesystem API with the intent of simplifying
 * filesystem initialization to one function. Once the filesystem is initialized, standard filesystem
 * operations can be performed such as fopen, fread, fwrite, etc. The filesystem has a single 
 * mount point at /data, so all file operations should use that mount path, eg:
 *  FILE* fp = fopen("/data/myfile.txt", "r");
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdio.h>

/**
 * @brief path to non-volatile filesystem for storing user data
 */
#define FILESYSTEM_MOUNT_PATH "/data"

typedef enum {
    FILESYSTEM_ERR_NONE,
    FILESYSTEM_ERR_INIT_NVS_FAILED,
    FILESYSTEM_ERR_INIT_FS_FAILED,
} FILESYSTEM_ERR_T;

/**
 * @brief initialize the filesystem.
 * 
 * This function initializes the filesystem, which includes initializing the NVS flash storage.
 * Execute this function once at startup before performing any filesystem operations.
 * 
 * @returns FILESYSTEM_ERR_NONE if successful, otherwise an error code
 */
FILESYSTEM_ERR_T filesystem_init(void);