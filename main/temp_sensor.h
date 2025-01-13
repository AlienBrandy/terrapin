/**
 * temp_sensor.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

void temp_sensor_init(uint32_t datastream_index);
float temp_sensor_get(void);
