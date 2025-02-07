/**
 * state_machine.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * 
 */
typedef enum {
    STATE_MACHINE_ERR_NONE,
    STATE_MACHINE_ERR_NOT_INITIALIZED,
    STATE_MACHINE_ERR_INVALID_PARAMETER,
    STATE_MACHINE_OBJECT_CREATE_FAIL,
    STATE_MACHINE_ERR_QUEUE_CREATE_FAIL,
    STATE_MACHINE_ERR_QUEUE_FULL,
    STATE_MACHINE_ERR_TASK_START_FAIL,
} STATE_MACHINE_ERR_T;

/**
 * 
 */
typedef struct state_machine_t* state_machine_handle_t;

/**
 * 
 */
enum {
    SIGNAL_USER  =  0,
    SIGNAL_ENTRY = -1,
    SIGNAL_EXIT  = -2,    
};

/**
 * 
 */
#define STATE_MACHINE_MESSAGE_DATA_SIZE 128
typedef struct {
    int32_t signal;
    TaskHandle_t caller;
    uint8_t data[STATE_MACHINE_MESSAGE_DATA_SIZE];
} state_machine_message_t;

/**
 * 
 */
typedef void (*state_machine_function_t)(state_machine_message_t* message);

STATE_MACHINE_ERR_T state_machine_init(state_machine_handle_t* state_machine, const char* name, uint32_t thread_priority, state_machine_function_t initial_state);

STATE_MACHINE_ERR_T state_machine_post(state_machine_handle_t state_machine, state_machine_message_t* message);

void state_machine_set_state(state_machine_handle_t state_machine, state_machine_function_t new_state);

