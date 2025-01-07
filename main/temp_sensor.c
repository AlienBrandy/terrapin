/**
 * temp_sensor.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */


#include "driver/temperature_sensor.h"
#include "datastream.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "temp_sensor.h"

static temperature_sensor_handle_t temp_sensor = NULL;

static void temp_sensor_task(void* args)
{
    while (1)
    {
        float val;
        temperature_sensor_get_celsius(temp_sensor, &val);
        datastream_update(AMBIENT_TEMPERATURE, val);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void temp_sensor_init(void)
{
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    temperature_sensor_install(&temp_sensor_config, &temp_sensor);
    temperature_sensor_enable(temp_sensor);

    // create thread
    static const uint32_t TEMP_SENSOR_TASK_STACK_DEPTH_BYTES = 4096;
    static const uint32_t TEMP_SENSOR_TASK_PRIORITY = 2;
    static const char*    TEMP_SENSOR_TASK_NAME = "temp sensor";
    TaskHandle_t h_task = NULL;
    xTaskCreatePinnedToCore(temp_sensor_task, TEMP_SENSOR_TASK_NAME, TEMP_SENSOR_TASK_STACK_DEPTH_BYTES, NULL, TEMP_SENSOR_TASK_PRIORITY, &h_task, tskNO_AFFINITY);
}

float temp_sensor_get(void)
{
    float val = (float)datastream_get(AMBIENT_TEMPERATURE).value;
    return val;
}
