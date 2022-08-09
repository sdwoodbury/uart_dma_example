// file: fifo.h
#ifndef FIFO_H
#define FIFO_H

#ifdef __cplusplus 
extern "C" {
#endif 

#include <stdint.h>

typedef struct 
{
    uint8_t* buffer;
    uint16_t element_size;
    uint16_t capacity;
    uint16_t filled;
    uint16_t start_idx;
    uint16_t end_idx;

} fifo_t;

// implemented as a circular buffer
// when using this fifo to pass structs from interrupt context to process context, it is not helpful
// if only part of the struct is copied. ensure this doesn't happen by setting element_size to the
// size of the struct
void fifo_init(fifo_t* fifo, uint8_t* buffer,  uint16_t capacity, uint16_t element_size);

// add bytes to the end. returns the number of bytes added
uint16_t fifo_enqueue(fifo_t* fifo, uint8_t* src, uint16_t to_copy);

// copy bytes from the front of the fifo to the dest buffer. then frees those bytes from the fifo. returns the number of bytes read 
uint16_t fifo_dequeue(fifo_t* fifo, uint8_t* dest, uint16_t to_copy);

#ifdef __cplusplus
}
#endif 

#endif
