/**
 * datastream.h
 * 
 * SPDX-FileCopyrightText: Copyright © 2025 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 * 
 */
#pragma once
#include <stdint.h>
#include "datastream.def"

/**
 * @brief datastream definition
 */
typedef struct {
    double value;           // store all values as double; can cast to int or float as necessary. int64 may lose precision.
    int64_t timestamp;      // time of last update, in milliseconds since epoch
    const char* topic;      // topic associated with data
    const char* name;       // name associated with data
    const char* units;      // unit of measure
    int precision;          // data precision, ie number of digits after the decimal
} datastream_t;

/**
 * @brief datastream identifiers
 */
typedef enum {
    #define X(NAME, TOPIC, UNITS, PRECISION) NAME,
    DATASTREAM_LIST
    #undef X
    DATASTREAM_ID_MAX
} DATASTREAM_ID_T;

/**
 * 
 */
void datastream_init(void);

/**
 * 
 */
void datastream_update(DATASTREAM_ID_T datastream_id, double val);

/**
 * 
 */
datastream_t datastream_get(uint32_t datastream_id);
