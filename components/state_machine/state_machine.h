/**
 * state_machine.h
 * 
 * This is a basic framework for running state machines. Inspired by the QP state machine
 * design, it borrows the concept of a message-driven active object executing in a thread.
 * The state machine remains idle until a message is posted to it. When a message is posted,
 * the framework calls the current state function. Each state is represented by a function
 * that takes a message as input and returns void. The messages might originate externally 
 * or from the framework itself as a way of signaling entry and exit conditions.
 * 
 * The message structure includes a signal to identify the message type. It also includes a
 * fixed-size data field that can be interpreted by the state function as needed. The state
 * functions will handle the message by executing the appropriate action, possibly changing
 * states and/or posting new messages. If a caller wants to block until a message has been
 * processed, it can pass its task handle to the message and then call xTaskNotifyWait().
 * The state machine function should then notify the caller when the message execution is
 * complete by calling xTaskNotify().
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Error codes returned by the state machine functions.
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
 * @brief an opaque handle to the state machine object.
 */
typedef struct state_machine_t* state_machine_handle_t;

/**
 * @brief Signal values that can be posted to the state machine.
 * 
 * The SIGNAL_ENTRY and SIGNAL_EXIT signal values are reserved
 * for the state machine framework. Any value above or including
 * SIGNAL_USER can be used by the application.
 */
enum {
    SIGNAL_USER  =  0,  // first avaliable signal value
    SIGNAL_ENTRY = -1,  // reserved; indicates entry to state
    SIGNAL_EXIT  = -2,  // reserved; indicates exit from state
};

/**
 * @brief The message structure that is posted to the state machine.
 */
#define STATE_MACHINE_MESSAGE_DATA_SIZE 128
typedef struct {
    int32_t      signal;    // signal value
    TaskHandle_t caller;    // task handle of caller; NULL if not waiting.
    uint8_t      data[STATE_MACHINE_MESSAGE_DATA_SIZE]; // generic data field
} state_machine_message_t;

/**
 * @brief The state machine function type.
 */
typedef void (*state_machine_function_t)(state_machine_message_t* message);

/**
 * @brief Initialize the state machine object.
 * 
 * This function creates the state machine object and starts the thread
 * that will execute the state machine functions. The initial state function
 * is called with a SIGNAL_ENTRY message to allow the state to initialize.
 * 
 * @param state_machine The state machine object to initialize.
 * @param name The name of the state machine thread; for debugging.alignas
 * @param thread_priority The priority assigned to the state machine thread.
 * @param initial_state The initial state function of the state machine.
 * @return STATE_MACHINE_ERR_T The error code.
 */
STATE_MACHINE_ERR_T state_machine_init(state_machine_handle_t* state_machine, const char* name, uint32_t thread_priority, state_machine_function_t initial_state);

/**
 * @brief Post a message to the state machine.
 * 
 * @param state_machine The state machine object.
 * @param message The message to post.
 * @return STATE_MACHINE_ERR_T The error code.
 */
STATE_MACHINE_ERR_T state_machine_post(state_machine_handle_t state_machine, state_machine_message_t* message);

/**
 * @brief Set the state of the state machine.
 * 
 * This function will call the exit function of the current state and the
 * entry function of the new state, unless the new and current states are
 * the same in which case no action is taken.
 * 
 * @param state_machine The state machine object.
 * @param new_state The new state function.
 */
void state_machine_set_state(state_machine_handle_t state_machine, state_machine_function_t new_state);

