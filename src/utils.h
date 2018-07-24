/*
 * utils.h
 *
 *  Created on: 20.1.2017
 *      Author: Mika
 */

#ifndef UTILS_H_
#define UTILS_H_

#ifndef max
#define max(a,b) \
		({ __typeof__ (a) _a = (a); \
		   __typeof__ (b) _b = (b); \
		   _a > _b ? _a : _b; })
#endif

#ifndef min
#define min(a,b) \
		({ __typeof__ (a) _a = (a); \
		   __typeof__ (b) _b = (b); \
		   _a < _b ? _a : _b; })
#endif

#ifndef clamp
#define clamp(value, minv, maxv) \
		({ __typeof__ (value) _value = (value); \
		   __typeof__ (minv) _min = (minv); \
		   __typeof__ (maxv) _max = (maxv); \
		   _value < _min ? _min : (_value > _max ? _max : _value) ; })
#endif

#ifndef elements
#define elements(x) (sizeof(x) / sizeof(x[0]))
#endif

#endif /* UTILS_H_ */
