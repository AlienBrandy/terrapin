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
#include "mqtt.h"
#include "console_windows.h"
#include "mqtt_client.h"
#include "config.h"
#include "jsmn.h"

static bool mqtt_connected = false;

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

/**
 * @brief handler for updates to telemetry data
 */
static void telemetry_update_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    if (!mqtt_connected)
    {
        return;
    }

    datastream_t ds;
    if (datastream_get(id, &ds) == DATASTREAM_ERR_NONE)
    {
        char data[20];
        snprintf(data, 20, "%.*f", ds.precision, ds.value);
        mqtt_publish(ds.topic, ds.name, data);
    }
}

static void rpc_handler(esp_mqtt_event_handle_t event)
{
    // extract request ID from event topic
    int request_len = strlen("v1/devices/me/rpc/request/");
    int request_id = 0;
    if (event->topic_len < request_len)
    {
        ESP_LOGI(PROJECT_NAME, "rpc_handler(): could not extract request ID.");
        return;
    }
    request_id = atoi(event->topic + request_len);

    // build response topic
    static char response_topic[128];
    snprintf(response_topic, 128, "v1/devices/me/rpc/response/%d", request_id);

    // extract method and value from event data
    // data is received as {"method":"<datastream name>", "params":"<new value>"}
    // token[0] = (entire string)
    // token[1] = "method"
    // token[2] = <datastream name>
    // token[3] = "params"
    // token[4] = <new value>
    jsmn_parser parser;
    jsmntok_t token[6];
    jsmn_init(&parser);
    int nTokens = jsmn_parse(&parser, event->data, event->data_len, token, 6);
    if (nTokens != 5)
    {
        ESP_LOGI(PROJECT_NAME, "rpc_handler(): invalid request format, tokens = %d", nTokens);
        mqtt_publish(response_topic, "Result", "Error");
        return;
    }
    char key[20] = {0};
    char val[20] = {0};
    strncpy(key, event->data + token[2].start, token[2].end - token[2].start);
    strncpy(val, event->data + token[4].start, token[4].end - token[4].start);

    // update datastream
    double dval = atof(val);
    bool retc = datastream_update_by_name(key, dval) == DATASTREAM_ERR_NONE;

    // format reply
    static const char* keys[2];
    static const char* vals[2];
    keys[0] = key;
    vals[0] = val;
    keys[1] = "result";
    vals[1] = retc ? "Success" : "Error";

    mqtt_publish_list(response_topic, keys, vals, 2);
}

static void attributes_handler(esp_mqtt_event_handle_t event)
{
    ESP_LOGI(PROJECT_NAME, "attributes_handler()");
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
    if (datastream_register_update_handler(TERRAPIN_AMBIENT_TEMPERATURE, telemetry_update_handler) != DATASTREAM_ERR_NONE)
    {
        ESP_LOGE(PROJECT_NAME, "datastream_register_update_handler for AMBIENT_TEMPERATURE failed.\n");
        return false;
    }

    return true;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(PROJECT_NAME, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        mqtt_connected = true;
        mqtt_subscribe("v1/devices/me/rpc/request/+");
        mqtt_subscribe("v1/devices/me/attributes");
        ESP_LOGI(PROJECT_NAME, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqtt_connected = false;
        ESP_LOGI(PROJECT_NAME, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(PROJECT_NAME, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(PROJECT_NAME, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(PROJECT_NAME, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(PROJECT_NAME, "MQTT_EVENT_DATA");
        if (strstr(event->topic, "v1/devices/me/attributes") != NULL)
        {
            attributes_handler(event);
        }
        if (strstr(event->topic, "v1/devices/me/rpc/request") != NULL)
        {
            rpc_handler(event);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(PROJECT_NAME, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(PROJECT_NAME, "Other event id:%d", event->event_id);
        break;
    }
}
