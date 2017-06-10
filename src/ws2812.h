/*
 * ws2812.h
 *
 *  Created on: 14.1.2017
 *      Author: Mika
 */

#ifndef WS2812_H_
#define WS2812_H_

#include <stdint.h>

typedef struct{
	uint8_t g;
	uint8_t r;
	uint8_t b;
} __attribute__((__packed__)) led_t;

extern led_t strip[300];

void send_strip_data(uint32_t size);
void clear_data();


#endif /* WS2812_H_ */
