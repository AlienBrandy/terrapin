#pragma once

#include "errno.h"

typedef enum {
    DL_ERROR_FAIL               = -1,
    DL_ERROR_NONE               = 0,
    DL_ERROR_INIT_NVS_FAIL,
    DL_ERROR_INIT_FS_FAIL, 
    DL_ERROR_INIT_CONSOLE_FAIL,
    DL_ERROR_CLI_START_FAIL,  
} DL_ERROR_T;
