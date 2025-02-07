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
#include "esp_adc/adc_oneshot.h"
#include "terrapin.h"
#include <math.h>
#include "min_max.h"

static temperature_sensor_handle_t temp_sensor = NULL;
static adc_oneshot_unit_handle_t adc1_handle = NULL;

static float calc_temperature(uint32_t adc_raw)
{
    static const float BETA = 3375.0;
    static const float NOMINAL_R_OHMS = 10000.0;
    static const float NOMINAL_TEMP_KELVIN = 25.0 + 273.15;
    static const float ADC_RAW_MAX = 4095.0;

    // avoid divide-by-zero
    adc_raw = min(4094, adc_raw);

    float deg_K = 1.0 / ( (log(((NOMINAL_R_OHMS * adc_raw) / (ADC_RAW_MAX - adc_raw)) / NOMINAL_R_OHMS) / BETA) + (1 / (NOMINAL_TEMP_KELVIN)) );
    float deg_C = deg_K - 273.15;

    return deg_C;
}

static void temp_sensor_task(void* args)
{
    while (1)
    {
        float cpu_temp = 0;
        if (temperature_sensor_get_celsius(temp_sensor, &cpu_temp) == ESP_OK)
        {
            datastream_update(DATASTREAM_CPU_TEMPERATURE, cpu_temp);
        }
        int adc_raw = 0;
        if (adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &adc_raw) == ESP_OK)
        {
            float adc_temp = calc_temperature(adc_raw);
            datastream_update(DATASTREAM_CH1_TEMPERATURE, adc_temp);
        }
        if (adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &adc_raw) == ESP_OK)
        {
            float adc_temp = calc_temperature(adc_raw);
            datastream_update(DATASTREAM_CH2_TEMPERATURE, adc_temp);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void temp_sensor_init(void)
{
    // initialize internal cpu temp sensor
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    temperature_sensor_install(&temp_sensor_config, &temp_sensor);
    temperature_sensor_enable(temp_sensor);

    // initialize ADC1
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };    
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    // initialize ADC1 channels 3 & 4
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config);
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &config);

    // create thread
    static const uint32_t TEMP_SENSOR_TASK_STACK_DEPTH_BYTES = 4096;
    static const uint32_t TEMP_SENSOR_TASK_PRIORITY = 2;
    static const char*    TEMP_SENSOR_TASK_NAME = "temp sensor";
    TaskHandle_t h_task = NULL;
    xTaskCreatePinnedToCore(temp_sensor_task, TEMP_SENSOR_TASK_NAME, TEMP_SENSOR_TASK_STACK_DEPTH_BYTES, NULL, TEMP_SENSOR_TASK_PRIORITY, &h_task, 1);
}
