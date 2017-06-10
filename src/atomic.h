/*
 * atomic.h
 *
 *  Created on: 12.1.2017
 *      Author: Mika
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

#include "chip.h"


#define atomic() \
	int __done;	\
	unsigned int __pri; \
	for(__pri = __get_PRIMASK(), __disable_irq(), __done = 1; __done; __done = 0, __set_PRIMASK(__pri)) \



#endif /* ATOMIC_H_ */
