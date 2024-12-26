/**
 * filesystem.h
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

FILESYSTEM_ERR_T filesystem_init(void);