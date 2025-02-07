/**
 * state_machine.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "state_machine.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define STATE_MACHINE_QUEUE_LENGTH 10

struct state_machine_t {
    QueueHandle_t h_queue;
    TaskHandle_t h_task;
    state_machine_function_t current_state;
};

static void task(void *args)
{
    state_machine_handle_t state_machine = *(state_machine_handle_t*)args;
    state_machine_message_t message;

    while (1)
    {
        // wait for messages in queue
        xQueueReceive(state_machine->h_queue, &message, portMAX_DELAY);

        // ensure initial transition has been executed
        assert(state_machine->current_state != NULL);

        // call current state machine function with message
        state_machine->current_state(&message);
    }
}

STATE_MACHINE_ERR_T state_machine_init(state_machine_handle_t* state_machine, const char* name, uint32_t thread_priority, state_machine_function_t initial_state)
{
    // validate parameters
    if ((state_machine == NULL) || (name == NULL))
    {
        return STATE_MACHINE_ERR_INVALID_PARAMETER;
    }

    // create state machine object
    *state_machine = malloc(sizeof(struct state_machine_t));
    if (*state_machine == NULL)
    {
        return STATE_MACHINE_OBJECT_CREATE_FAIL;
    }

    // create queue
    (*state_machine)->h_queue = xQueueCreate(STATE_MACHINE_QUEUE_LENGTH, sizeof(state_machine_message_t));
    if ((*state_machine)->h_queue == NULL)
    {
        return STATE_MACHINE_ERR_QUEUE_CREATE_FAIL;
    }

    // create thread
    static const uint32_t STATE_MACHINE_TASK_STACK_DEPTH_BYTES = 4096;
    (*state_machine)->h_task = NULL;
    xTaskCreatePinnedToCore(task, name, STATE_MACHINE_TASK_STACK_DEPTH_BYTES, state_machine, thread_priority, &((*state_machine)->h_task), tskNO_AFFINITY);
    if ((*state_machine)->h_task == NULL)
    {
        return STATE_MACHINE_ERR_TASK_START_FAIL;
    }

    // transition to initial state
    static state_machine_message_t entry_message = {.signal = SIGNAL_ENTRY};
    (*state_machine)->current_state = initial_state;
    (*state_machine)->current_state(&entry_message);

    return STATE_MACHINE_ERR_NONE;
}

void state_machine_set_state(state_machine_handle_t state_machine, state_machine_function_t new_state)
{
    assert(state_machine != NULL);
    assert(state_machine->current_state != NULL);
    assert(new_state != NULL);

    static state_machine_message_t entry_message = {.signal = SIGNAL_ENTRY};
    static state_machine_message_t exit_message = {.signal = SIGNAL_EXIT};

    if (state_machine->current_state != new_state)
    {
        state_machine->current_state(&exit_message);
        state_machine->current_state = new_state;
        state_machine->current_state(&entry_message);
    }
}

STATE_MACHINE_ERR_T state_machine_post(state_machine_handle_t state_machine, state_machine_message_t* message)
{
    assert(message != NULL);
    assert(message->signal >= SIGNAL_USER);

    if (state_machine == NULL)
    {
        return STATE_MACHINE_ERR_NOT_INITIALIZED;
    }

    int retc = xQueueSend(state_machine->h_queue, message, 0);
    if (retc != pdTRUE)
    {
        return STATE_MACHINE_ERR_QUEUE_FULL;
    }

    return STATE_MACHINE_ERR_NONE;
}
