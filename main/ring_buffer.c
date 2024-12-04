#include "ring_buffer.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct node_tag {
    struct node_tag*  prev;
    struct node_tag*  next;
    uint32_t          size;
    uint8_t           data;
} node_t;

struct ring_buffer_t {
    uint8_t* buffer;
    int      length;
    node_t*  head;
    node_t*  tail;
    node_t*  read;
};

RING_BUFFER_ERR_T ring_buffer_create(ring_buffer_handle_t* ring_buffer, int length)
{
    ring_buffer_handle_t rb;

    // create fifo structure
    rb = calloc(1, sizeof(struct ring_buffer_t));
    if (rb == NULL)
    {
        return RING_BUFFER_ERR_INIT_FAILED;
    }

    // allocate fifo buffer
    rb->buffer = calloc(length, sizeof(uint8_t));
    if (rb->buffer == NULL)
    {
        ring_buffer_destroy(rb);
        return RING_BUFFER_ERR_INIT_FAILED;
    }
    rb->length = length;

    // initialize structure pointers
    rb->head = (node_t*)rb->buffer;
    rb->tail = (node_t*)rb->buffer;
    rb->read = (node_t*)rb->buffer;
    
    rb->head->prev = (node_t*)rb->buffer;
    rb->head->next = (node_t*)rb->buffer;
    rb->head->size = 0;

    *ring_buffer = rb;
    return RING_BUFFER_ERR_NONE;
}

void ring_buffer_destroy(ring_buffer_handle_t rb)
{
    if (rb != NULL)
    {
        if (rb->buffer != NULL)
        {
            free(rb->buffer);
        }
        free(rb);
    }
}

static bool is_empty(ring_buffer_handle_t rb)
{
    return rb->head->size == 0;
}

static node_t* allocate_node(ring_buffer_handle_t rb, uint32_t length)
{
    // check relative position of head and tail
    if (rb->head <= rb->tail)
    {
        // calculate space from end of tail to end of buffer
        uint8_t* start_of_free_space = (uint8_t*)rb->tail + rb->tail->size;
        uint8_t* end_of_free_space = rb->buffer + rb->length;
        uint32_t free_space = end_of_free_space - start_of_free_space;
        if (free_space >= length)
        {
            return (node_t*)start_of_free_space;
        }

        // calculate space from start of buffer to head
        if ((uint8_t*)rb->head - rb->buffer >= length)
        {
            return (node_t*)rb->buffer;
        }
    }
    else
    {
        // calculate space between tail and head
        uint8_t* start_of_free_space = (uint8_t*)rb->tail + rb->tail->size;
        uint8_t* end_of_free_space = (uint8_t*)rb->head;
        uint32_t free_space = end_of_free_space - start_of_free_space;
        if (free_space >= length)
        {
            return (node_t*)start_of_free_space;
        }
    }
    return NULL;
}

RING_BUFFER_ERR_T ring_buffer_add(ring_buffer_handle_t rb, uint8_t* data, int data_len)
{
    if (rb == NULL) return RING_BUFFER_ERR_NOT_INITIALIZED;

    // check if new item can fit in buffer
    uint32_t new_element_size = data_len + sizeof(node_t);
    if (new_element_size > rb->length)
    {
        return RING_BUFFER_ERR_DATA_OVERSIZED;
    }

    // find location for new entry
    node_t* new_node = allocate_node(rb, new_element_size);
    while (new_node == NULL)
    {
        // delete old entry to free up space
        if (ring_buffer_remove(rb, NULL, NULL) != RING_BUFFER_ERR_NONE)
        {
            // this could only happen if the entire buffer is too small
            // to hold the element
            return RING_BUFFER_ERR_DATA_OVERSIZED;
        }

        // check again for avialable space
        new_node = allocate_node(rb, new_element_size);
    }

    // append new element after tail
    memcpy(&(new_node->data), data, data_len);
    new_node->size = new_element_size;

    // update list pointers
    new_node->prev = rb->tail;
    new_node->next = rb->head;
    rb->tail->next = new_node;
    rb->head->prev = new_node;

    // update tail and read pointers
    rb->tail = new_node;

    return RING_BUFFER_ERR_NONE;
}

RING_BUFFER_ERR_T ring_buffer_remove(ring_buffer_handle_t rb, uint8_t** data, uint32_t* data_len)
{
    if (rb == NULL) return RING_BUFFER_ERR_NOT_INITIALIZED;

    if (is_empty(rb))
    {
        return RING_BUFFER_ERR_EMPTY;
    }

    // retrieve data from first element
    if (data) *data = &rb->head->data;
    if (data_len) *data_len = rb->head->size - sizeof(node_t);

    // update node pointers
    rb->head->prev->next = rb->head->next;
    rb->head->next->prev = rb->head->prev;
    rb->head->size = 0;

    // update head and read pointers
    if (rb->read == rb->head) rb->read = rb->head->next;
    rb->head = rb->head->next;

    return RING_BUFFER_ERR_NONE;
}

static RING_BUFFER_ERR_T ring_buffer_read(ring_buffer_handle_t rb, uint8_t** data, int* data_len)
{
    if (rb == NULL) return RING_BUFFER_ERR_NOT_INITIALIZED;

    if (is_empty(rb))
    {
        return RING_BUFFER_ERR_EMPTY;
    }

    // retrieve data from read pointer
    if (data) *data = &rb->read->data;
    if (data_len) *data_len = rb->read->size - sizeof(node_t);

    return RING_BUFFER_ERR_NONE;
}

RING_BUFFER_ERR_T ring_buffer_peek_head(ring_buffer_handle_t rb, uint8_t** data, int* data_len)
{
    if (rb == NULL) return RING_BUFFER_ERR_NOT_INITIALIZED;

    // set read pointer to head
    rb->read = rb->head;

    return ring_buffer_read(rb, data, data_len);
}

RING_BUFFER_ERR_T ring_buffer_peek_tail(ring_buffer_handle_t rb, uint8_t** data, int* data_len)
{
    if (rb == NULL) return RING_BUFFER_ERR_NOT_INITIALIZED;

    // set read pointer to tail
    rb->read = rb->tail;

    return ring_buffer_read(rb, data, data_len);
}

RING_BUFFER_ERR_T ring_buffer_peek_next(ring_buffer_handle_t rb, uint8_t** data, int* data_len)
{
    if (rb == NULL) return RING_BUFFER_ERR_NOT_INITIALIZED;

    // increment read pointer
    rb->read = rb->read->next;

    return ring_buffer_read(rb, data, data_len);
}

RING_BUFFER_ERR_T ring_buffer_peek_prev(ring_buffer_handle_t rb, uint8_t** data, int* data_len)
{
    if (rb == NULL) return RING_BUFFER_ERR_NOT_INITIALIZED;

    // decrement read pointer
    rb->read = rb->read->prev;

    return ring_buffer_read(rb, data, data_len);
}
