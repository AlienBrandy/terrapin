#pragma once

#include <stdint.h>

typedef enum {
    RING_BUFFER_ERR_NONE,
    RING_BUFFER_ERR_NOT_INITIALIZED,
    RING_BUFFER_ERR_INIT_FAILED,
    RING_BUFFER_ERR_EMPTY,
    RING_BUFFER_ERR_FULL,
    RING_BUFFER_ERR_DATA_OVERSIZED,
} RING_BUFFER_ERR_T;

/**
 * @brief opaque pointer to ring buffer
 */
typedef struct ring_buffer_t* ring_buffer_handle_t;

RING_BUFFER_ERR_T ring_buffer_create(ring_buffer_handle_t* ring_buffer, int length);
void              ring_buffer_destroy(ring_buffer_handle_t ring_buffer);
RING_BUFFER_ERR_T ring_buffer_add(ring_buffer_handle_t ring_buffer, uint8_t* data, int data_len);
RING_BUFFER_ERR_T ring_buffer_remove(ring_buffer_handle_t ring_buffer, uint8_t** data, uint32_t* data_len);
RING_BUFFER_ERR_T ring_buffer_peek_head(ring_buffer_handle_t ring_buffer, uint8_t** data, int* data_len);
RING_BUFFER_ERR_T ring_buffer_peek_tail(ring_buffer_handle_t ring_buffer, uint8_t** data, int* data_len);
RING_BUFFER_ERR_T ring_buffer_peek_next(ring_buffer_handle_t ring_buffer, uint8_t** data, int* data_len);
RING_BUFFER_ERR_T ring_buffer_peek_prev(ring_buffer_handle_t ring_buffer, uint8_t** data, int* data_len);


