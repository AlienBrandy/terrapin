/**
 * terrapin.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2025 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include "terrapin.h"
#include "esp_log.h"
#include "datastream.h"
#include "temp_sensor.h"
#include "rgb_led.h"
#include "driver/gpio.h"

/**
 * @brief list of terrapin datastreams
 */
static datastream_t datastreams[TERRAPIN_DATASTREAM_IDX_MAX] =
{
    #define X(NAME, TOPIC, UNITS, PRECISION) {.name = #NAME, .topic = TOPIC, .units = UNITS, .precision = PRECISION},
    DATASTREAM_LIST
    #undef X
};

/**
 * @brief handler for updates to RGB led datastream value
 */
static void rgb_led_update_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    datastream_t ds;
    datastream_get(TERRAPIN_RGB_LED, &ds);
    rgb_led_write(ds.value);
}

/**
 * @brief handler for updates to RGB led datastream value
 */
static void gpio38_update_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    datastream_t ds;
    datastream_get(TERRAPIN_GPIO_38, &ds);
    gpio_set_level(GPIO_NUM_38, ds.value ? 1 : 0);
}

bool terrapin_init(void)
{
    // initialize datastream module
    if (datastream_init(datastreams, TERRAPIN_DATASTREAM_IDX_MAX) != DATASTREAM_ERR_NONE)
    {
        ESP_LOGE(PROJECT_NAME, "datastream_init() failed");
        return false;
    }

    // start the temp sensor task
    temp_sensor_init(TERRAPIN_AMBIENT_TEMPERATURE);

    // initialize the LED module
    if (!rgb_led_init())
    {
        ESP_LOGE(PROJECT_NAME, "rgb_led_init() failed");
        return false;
    }

    // initialize a sample GPIO
    gpio_config_t config = 
    {
        .pin_bit_mask = GPIO_NUM_38,
        .mode = GPIO_MODE_DEF_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    if (gpio_config(&config) != ESP_OK)
    {
        ESP_LOGE(PROJECT_NAME, "gpio_config failed");
        return false;
    }

    // register datastream update handlers
    if (datastream_register_update_handler(TERRAPIN_RGB_LED, rgb_led_update_handler) != DATASTREAM_ERR_NONE)
    {
        ESP_LOGE(PROJECT_NAME, "datastream_register_update_handler for RGB_LED failed.\n");
        return false;
    }
    if (datastream_register_update_handler(TERRAPIN_GPIO_38, gpio38_update_handler) != DATASTREAM_ERR_NONE)
    {
        ESP_LOGE(PROJECT_NAME, "datastream_register_update_handler for GPIO_38 failed.\n");
        return false;
    }

    return true;
}
