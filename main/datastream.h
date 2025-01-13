/**
 * datastream.h
 * 
 * Datastreams represent system inputs and outputs that are shared with the outside world.
 * The datastream object model stores the input/output value, the timestamp of the last 
 * update, and other const metadata to help identify and interpret the value. The datastream's 
 * value is a scalar quantity stored internally as a double precision floating point.
 * The value can be cast to any datatype that fits in a double.
 * 
 * Datastream objects should only be accessed using the read and write methods provided
 * in this module. The read method ensures all the fields of a datastream object are
 * retrieved as a coherent set in a threadsafe manner. The write function updates the 
 * fields atomically as well as signals execution of any callback functions that were 
 * registered with the particular datastream index.
 * 
 * The datastream module runs a thread to handle update events and execute registered
 * callback functions. The event loop registration helps decouple the code which updates
 * datastreams from the code that uses them.
 * 
 * SPDX-FileCopyrightText: Copyright © 2025 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 * 
 */
#pragma once
#include <stdint.h>
#include "esp_event.h"
#include "datastream.def"

/**
 * @brief datastream module return codes
 */
typedef enum {
    #define X(A, B) A,
    DATASTREAM_ERROR_LIST
    #undef X
    DATASTREAM_ERR_MAX
} DATASTREAM_ERR_T;

/**
 * @brief datastream definition
 */
typedef struct {
    double value;           // store all values as double; can cast to int or float as necessary.
    int64_t timestamp;      // time of last update, in milliseconds since epoch
    const char* topic;      // topic associated with data
    const char* name;       // name associated with data
    const char* units;      // unit of measure
    int precision;          // data precision, ie number of digits after the decimal
} datastream_t;

/**
 * @brief initialize the datastream module.
 * 
 * This should be called during system intialization before any of the other datastream
 * API functions are used. The datastream_array parameter accepts a list of datastreams
 * known to the system. The list of datastreams objects should be defined with static
 * visibility to discourage other modules from accessing the list other than through the
 * canonical read/write methods. 
 * 
 * Datastreams are identified by an index parameter passed to these read/write methods.
 * The index of each datastream is equivalent to it's position in the datastream_array
 * list. An enumeration of datastreams by index should be defined in a public header
 * for use by other modules. The size of the list is provided in another parameter and
 * used as a guard against overrunning the list.
 * 
 * @param datastream_array a list of datastreams known to the system
 * @param array_entries the number of entries in the list
 * @returns true if the datastream module was successfully initialized
 */
DATASTREAM_ERR_T datastream_init(datastream_t* datastream_array, uint32_t array_entries);

/**
 * @brief update a datastream with a new value
 * 
 * The datastream is updated with the given value and the timestamp is updated 
 * to the current time. An event is posted to inform any registered callbacks that
 * an update has occured.
 * 
 * @param datastream_id the index of the datastream to update
 * @param value the value to write
 * @returns true if the datatstream was successfully updated 
 */
DATASTREAM_ERR_T datastream_update(uint32_t datastream_id, double val);

/**
 * @brief retrieves a datastream.
 * 
 * @param datastream_id the index of the datastream to fetch
 * @param datastram receives the datastream
 * @returns true if the datastream was successfully returned
 */
DATASTREAM_ERR_T datastream_get(uint32_t datastream_id, datastream_t* datastream);

/**
 * @brief register a callback to execute when a datastream is updated.
 * 
 * The callback functions are executed sequentially in the context of a thread
 * private to the datastream module. As such, the callback functions should be 
 * designed to execute quickly and not block, otherwise subsequent update 
 * events will pile up and be executed late or dropped altogether.
 * 
 * @param datastream_id the index of the datastream to monitor
 * @param handler the callback function to run when the datastream is updated
 * @returns true if handler is successfully registered
 */
DATASTREAM_ERR_T datastream_register_update_handler(uint32_t datastream_id, esp_event_handler_t handler);

/**
 * @brief translate a datastream return code to a description.
 * 
 * @param code the code to translate
 * @returns a string description of the return code
 */
const char* datastream_get_error_string(DATASTREAM_ERR_T code);
