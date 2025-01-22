/**
 * mqtt.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>

bool mqtt_start(void);
void mqtt_stop(void);
void mqtt_publish(const char* topic, const char* key, const char* val);
void mqtt_subscribe(char* topic);
