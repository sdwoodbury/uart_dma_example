// file: fifo.c
#include "fifo.h"
#include <string.h>

// implemented as a circular buffer
void fifo_init(fifo_t* fifo, uint8_t* buffer,  uint16_t capacity, uint16_t element_size)
{
    memset(buffer, 0, capacity);
    memset(fifo, 0, sizeof(fifo_t));
    fifo->buffer = buffer;
    fifo->capacity = capacity;
    fifo->element_size = element_size;
}

// add bytes to the end. returns the number of bytes added
uint16_t fifo_enqueue(fifo_t* fifo, uint8_t* src, uint16_t to_copy)
{
    // guarantee that fifo_enqueue will not overflow the buffer
    if (fifo->filled + to_copy > fifo->capacity)
        to_copy = (fifo->capacity - fifo->filled);

    // do not enqueue anything less than the element_size
    // if something is greater than the element_size, trim it
    uint16_t to_trim = to_copy % fifo->element_size;
    to_copy -= to_trim;

    // return if the buffer is empty or to_copy is smaller than the element_size
    if (to_copy == 0)
        return 0;

    uint16_t add_to_end = fifo->capacity - fifo->end_idx;
    if (add_to_end > to_copy)
        add_to_end = to_copy;

    uint16_t add_to_start = to_copy - add_to_end;
    memcpy(fifo->buffer + fifo->end_idx, src, add_to_end);
    memcpy(fifo->buffer, src + add_to_end, add_to_start);

    fifo->end_idx = (fifo->end_idx + to_copy) % fifo->capacity;
    fifo->filled += to_copy;
    return to_copy;
}

// copy bytes from the front of the fifo to the dest buffer. then frees those bytes from the fifo. returns the number of bytes read 
uint16_t fifo_dequeue(fifo_t* fifo, uint8_t* dest, uint16_t to_copy)
{
    // guarantee that fifo_dequeue will not underflow the buffer
    if (to_copy > fifo->filled)
        to_copy = fifo->filled;

    // must dequeue at least element_size
	uint16_t to_trim = to_copy % fifo->element_size;
	to_copy -= to_trim;

	// return if the fifo is empty or the destination buffer is not large enough
	if (fifo->filled == 0 || to_copy < fifo->element_size)
		return 0;

    uint16_t take_from_end = fifo->capacity - fifo->start_idx;
    if (take_from_end > to_copy)
        take_from_end = to_copy;

    uint16_t take_from_start = to_copy - take_from_end;

    memcpy(dest, fifo->buffer + fifo->start_idx, take_from_end);
    memcpy(dest + take_from_end, fifo->buffer, take_from_start);

    fifo->start_idx = (fifo->start_idx + to_copy) % fifo->capacity;
    fifo->filled -= to_copy;
    return to_copy;
}
