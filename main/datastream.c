/**
 * datastream.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2025 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include "datastream.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/semphr.h"
#include "esp_event.h"

/**
 * @brief  provides for threadsafe access to datastream list
 */
static SemaphoreHandle_t datastream_mutex = NULL;

/**
 * @brief handle to event loop
 */
static esp_event_loop_handle_t loop_handle;

/**
 * @brief number of datastreams
 */
static uint32_t number_of_datastreams = 0;

/**
 * @brief array of datastreams
 */
static datastream_t* datastreams = NULL;

/**
 * @brief event base for datastream events
 */
ESP_EVENT_DEFINE_BASE(DATASTREAM_EVENTS);

DATASTREAM_ERR_T datastream_init(datastream_t* datastream_array, uint32_t array_entries)
{
    datastreams = datastream_array;
    number_of_datastreams = array_entries;

    datastream_mutex = xSemaphoreCreateRecursiveMutex();

    // create event loop
    esp_event_loop_args_t args =
    {
        .queue_size = 25,
        .task_name = "Datastream evt loop",
        .task_priority = 2,
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY,
    };

    esp_err_t retc = esp_event_loop_create(&args, &loop_handle);
    return (retc == ESP_OK) ? DATASTREAM_ERR_NONE : DATASTREAM_ERR_CREATE_EVENT_LOOP_FAILED;
}

DATASTREAM_ERR_T datastream_update(uint32_t datastream_id, double value)
{
    if (datastream_id >= number_of_datastreams)
    {
        return DATASTREAM_ERR_INVALID_INDEX;
    }
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    uint64_t millisecs = (spec.tv_sec * 1000) + (spec.tv_nsec / 1000);

    xSemaphoreTakeRecursive(datastream_mutex, portMAX_DELAY);
    datastreams[datastream_id].value = value;
    datastreams[datastream_id].timestamp = millisecs;
    xSemaphoreGiveRecursive(datastream_mutex);

    // publish update to event loop
    esp_err_t retc = esp_event_post_to(loop_handle, DATASTREAM_EVENTS, datastream_id, NULL, 0, portMAX_DELAY);
    return (retc == ESP_OK) ? DATASTREAM_ERR_NONE : DATASTREAM_ERR_POST_EVENT_FAILED;
}

DATASTREAM_ERR_T datastream_get(uint32_t datastream_id, datastream_t* datastream)
{
    if (datastream_id >= number_of_datastreams)
    {
        return DATASTREAM_ERR_INVALID_INDEX;
    }
    xSemaphoreTakeRecursive(datastream_mutex, portMAX_DELAY);
    *datastream = datastreams[datastream_id];
    xSemaphoreGiveRecursive(datastream_mutex);

    return DATASTREAM_ERR_NONE;
}

DATASTREAM_ERR_T datastream_register_update_handler(uint32_t datastream_id, esp_event_handler_t handler)
{
    if (datastream_id >= number_of_datastreams)
    {
        return DATASTREAM_ERR_INVALID_INDEX;
    }
    esp_err_t retc = esp_event_handler_register_with(loop_handle, DATASTREAM_EVENTS, datastream_id, handler, NULL);
    return (retc == ESP_OK) ? DATASTREAM_ERR_NONE : DATASTREAM_ERR_REGISTER_EVENT_FAILED;
}

const char* datastream_get_error_string(DATASTREAM_ERR_T code)
{
    static const char * error_string[DATASTREAM_ERR_MAX] = 
    {
        #define X(A, B) B,
        DATASTREAM_ERROR_LIST
        #undef X
    };
    if (code < DATASTREAM_ERR_MAX)
    {
        return error_string[code];        
    }
    return "unknown error";
}
