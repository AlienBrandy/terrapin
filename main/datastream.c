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

static SemaphoreHandle_t datastream_mutex = NULL;

static datastream_t datastreams[DATASTREAM_ID_MAX] =
{
    #define X(NAME, TOPIC, UNITS, PRECISION) {.name = #NAME, .topic = TOPIC, .units = UNITS, .precision = PRECISION},
    DATASTREAM_LIST
    #undef X
};

void datastream_init(void)
{
    datastream_mutex = xSemaphoreCreateRecursiveMutex();
}

void datastream_update(DATASTREAM_ID_T datastream_id, double value)
{
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    uint64_t millisecs = (spec.tv_sec * 1000) + (spec.tv_nsec / 1000);

    xSemaphoreTakeRecursive(datastream_mutex, portMAX_DELAY);
    datastreams[datastream_id].value = value;
    datastreams[datastream_id].timestamp = millisecs;
    xSemaphoreGiveRecursive(datastream_mutex);
}

datastream_t datastream_get(uint32_t datastream_id)
{
    xSemaphoreTakeRecursive(datastream_mutex, portMAX_DELAY);
    datastream_t ds = datastreams[datastream_id];
    xSemaphoreGiveRecursive(datastream_mutex);

    return ds;
}
