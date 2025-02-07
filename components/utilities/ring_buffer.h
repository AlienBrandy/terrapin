/**
 * ring_buffer.h
 * 
 * Ciruclar buffer supporting variable length records.
 * This implementation of a ring buffer is designed for situations where records may
 * be different length, and the storage overhead of tracking record length and offset
 * is less than the memory potentially wasted by using a buffer with a fixed entry size
 * to hold the largest possible record.
 * 
 * This implementation utilizes a linked-list, in which a header preceeding every 
 * record contains the pointers to the previous and next records as well as the size
 * of the record. This amounts to 12 bytes overhead for each record. The oldest
 * records will be overwritten as necessary to make room for new ones. Depending on 
 * the record lengths, this overwrite strategy may orphan some memory at the end of
 * the buffer until the buffer has wrapped around and approaches the end again.
 * The benefit is that records are always contained in contiguous memory which
 * simplifies access.
 * 
 * Usage:
 * The methods are fairly straightforward. Create a new ring buffer by calling 
 * ring_buffer_create() which will do a one-time memory allocation. ring_buffer_add()
 * and ring_buffer_remove() are for adding and removing records respectively. The 
 * peek methods will fetch a record without removing it from the buffer. A read
 * pointer maintians the current peek position. peek_head() and peek_tail() set the 
 * read pointer, whereas peek_prev() and peek_next() move the read pointer by one
 * record. The read pointer will wrap at the end of the list.
 * 
 * SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

/**
 * @brief Error codes associated with the ring buffer module
 * 
 */
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


