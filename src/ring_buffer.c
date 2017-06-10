/*
 * ring_buffer.c
 *
 *  Created on: 11.1.2017
 *      Author: Mika
 */

#include "ring_buffer.h"
#include "atomic.h"

void rb_init(ring_buffer_t* rb, char* buffer, unsigned int size)
{
	rb->size = size;
	rb->buffer = buffer;
	rb->tail = 0;
	rb->count = 0;
}

void rb_pop(ring_buffer_t* rb, char* data)
{
	//__disable_irq();
	atomic() {
		*data = rb->buffer[rb->tail];
		rb->tail++;
		if(rb->tail >= rb->size)
			rb->tail -= rb->size;
		if(rb->count)
			rb->count--;
	}
	//__enable_irq();
}

void rb_push(ring_buffer_t* rb, char data)
{
	//__disable_irq();
	atomic() {
		unsigned int head = rb->tail + rb->count;
		if(head >= rb->size)
			head -= rb->size;
		rb->buffer[head] = data;
		rb->count++;
		if(rb->count >= rb->size)
			rb->count = rb->size - 1;
	}
	//__enable_irq();
}

void rb_peak(ring_buffer_t* rb, char* data)
{
	//__disable_irq();
	atomic() {
		unsigned int head = rb->tail + rb->count;
		if(head >= rb->size)
			rb->size -= rb->size;
		*data = rb->buffer[head];
	}
	//__enable_irq();
}
unsigned int rb_count(ring_buffer_t* rb)
{
	unsigned int ret;
	atomic() {
		ret = rb->count;
	}
	return ret;
}

unsigned int rb_is_full(ring_buffer_t* rb)
{
	unsigned int ret;
	atomic() {
		ret = (rb->count >= (rb->size - 1));
	}
	return ret;
}

unsigned int rb_is_empty(ring_buffer_t* rb)
{
	unsigned int ret;
	atomic() {
		ret = (rb->count == 0);
	}
	return ret;
}
