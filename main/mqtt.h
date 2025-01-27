/**
 * mqtt.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "mqtt_client.h"

bool mqtt_init(void);
bool mqtt_start(void);
void mqtt_stop(void);
void mqtt_publish(const char* topic, const char* key, const char* val);
void mqtt_publish_list(const char* topic, const char* keys[], const char* vals[], int nPairs);
void mqtt_subscribe(char* topic);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
