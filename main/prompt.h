/**
 * prompt.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

typedef enum {
    PROMPT_ERR_NONE,
    PROMPT_ERR_INIT_FAIL,
    PROMPT_ERR_TASK_START_FAIL,
} PROMPT_ERR_T;

PROMPT_ERR_T prompt_init(void);
PROMPT_ERR_T prompt_start(void);