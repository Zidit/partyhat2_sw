/*
 * ring_buffer.h
 *
 *  Created on: 11.1.2017
 *      Author: Mika
 */

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

typedef struct {
	char* buffer;
	unsigned int size;
	unsigned int count;
	unsigned int tail;
} ring_buffer_t;

void rb_init(ring_buffer_t* rb, char* buffer, unsigned int size);
void rb_pop(ring_buffer_t* rb, char* data);
void rb_push(ring_buffer_t* rb, char data);
void rb_peak(ring_buffer_t* rb, char* data);

unsigned int rb_count(ring_buffer_t* rb);
unsigned int rb_is_full(ring_buffer_t* rb);
unsigned int rb_is_empty(ring_buffer_t* rb);

#endif /* RING_BUFFER_H_ */
