/**
 * mqtt.c
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mqtt.h"
#include "mqtt_client.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "config.h"

/**
 * MQTT connection configuration
 * 
 * sample mosquitto command:
 * -q 1: QoS level 1
 * -h  : hostname
 * -d  : enable debug messages
 * -p  : port to connect with
 * -u  : username for authentication
 * -t  : topic on which to publish
 * -m  : message to send
 * mosquitto_pub -d -q 1 -h "mqtt.thingsboard.cloud" -p "1883" -t "v1/devices/me/telemetry" -u "$ACCESS_TOKEN" -m {"temperature":25}
 */
static const char* MQTT_PORT_TCP = "1883";
static const char* MQTT_PORT_TLS = "8883";

static esp_mqtt_client_handle_t client = NULL;

bool mqtt_init(void)
{
    if (client != NULL)
    {
        return true;
    }

    // configure logging
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    // retrieve connection parameters from configs
    const char* broker = '\0';
    const char* access_token = '\0';
    config_get_value("CONFIG_MQTT_BROKER", &broker);
    config_get_value("CONFIG_MQTT_ACCESS_TOKEN", &access_token);

    if (strlen(broker) == 0)
    {
        ESP_LOGW(PROJECT_NAME, "mqtt_start(): broker not defined.");
        return false;
    }
    if (strlen(access_token) == 0)
    {
        ESP_LOGW(PROJECT_NAME, "mqtt_start(): access token not defined.");
        return false;
    }

    // create the mqtt client
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker,
        .credentials.username = access_token,
        .credentials.set_null_client_id = true,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    return (client != NULL);
}

bool mqtt_start(void)
{
    if (client == NULL)
    {
        return false;
    }
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    return true;
}

void mqtt_stop(void)
{
    if (client == NULL)
    {
        return;
    }
    esp_mqtt_client_stop(client);
}

void mqtt_publish(const char* topic, const char* key, const char* val)
{
    if (client == NULL)
    {
        return;
    }
    static const int JSON_STRING_MAX = 128;
    char data[JSON_STRING_MAX];
    snprintf(data, JSON_STRING_MAX, "{\"%s\":\"%s\"}", key, val);
    ESP_LOGI(PROJECT_NAME, "publishing %s to %s", data, topic);
    esp_mqtt_client_publish(client, topic, data, 0, 1, 0);
}

void mqtt_publish_list(const char* topic, const char* keys[], const char* vals[], int nPairs)
{
    if (client == NULL)
    {
        return;
    }
    static const int JSON_STRING_MAX = 128;
    char data[JSON_STRING_MAX];
    int nWritten = snprintf(data, JSON_STRING_MAX, "{");

    for (int i = 0; i < nPairs; i++)
    {
        if (i)
        {
            nWritten += snprintf(data + nWritten, JSON_STRING_MAX - nWritten, ",");
        }
        nWritten += snprintf(data + nWritten, JSON_STRING_MAX - nWritten, "\"%s\":", keys[i]);
        if (vals[i] != NULL)
        {
            nWritten += snprintf(data + nWritten, JSON_STRING_MAX - nWritten, "\"%s\"", vals[i]);
        }
        else
        {
            nWritten += snprintf(data + nWritten, JSON_STRING_MAX - nWritten, "null");
        }
    }
    snprintf(data + nWritten, JSON_STRING_MAX - nWritten, "}");
    ESP_LOGI(PROJECT_NAME, "publishing %s to %s", data, topic);
    esp_mqtt_client_publish(client, topic, data, 0, 1, 0);
}

void mqtt_subscribe(char* topic)
{
    if (client == NULL)
    {
        return;
    }
    ESP_LOGI(PROJECT_NAME, "subscribing to %s", topic);
    esp_mqtt_client_subscribe(client, topic, 0);
}

__attribute__((weak))
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(PROJECT_NAME, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(PROJECT_NAME, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
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
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(PROJECT_NAME, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(PROJECT_NAME, "Other event id:%d", event->event_id);
        break;
    }
}
