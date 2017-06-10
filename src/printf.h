/*
Copyright (c) 2017 Mika Terhokoski

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */


#ifndef PRINTF_H_
#define PRINTF_H_

#include <stdarg.h>
#include <stdint.h>

//#define PRINT_FLOAT

struct stream {
	char* buffer;
	unsigned int buffer_size;
	unsigned int pos;
	void (*printchar)(int c);
};

typedef unsigned int size_t;

int __vprintf(const char *format, va_list arg);
int __vfprintf(struct stream *strm, const char * format, va_list arg);
int __vsprintf(char *str, const char *format, va_list arg);
int __vsnprintf(char *str, unsigned int size, const char *format, va_list arg);

int __printf(const char *format, ...);
int __fprintf(struct stream *strm, const char *format, ...);
int __sprintf(char *str, const char *format, ...);
int __snprintf(char *str, unsigned int size, const char *format, ...);

extern void print_char(int c);

#define printf(...) (__printf(__VA_ARGS__))
#define fprintf(...) (__fprintf(__VA_ARGS__))
#define sprintf(...) (__sprintf(__VA_ARGS__))
#define snprintf(...) (__snprintf(__VA_ARGS__))

#define vprintf(...) (__vprintf(__VA_ARGS__))
#define vfprintf(...) (__vfprintf(__VA_ARGS__))
#define vsprintf(...) (__vsprintf(__VA_ARGS__))
#define vsnprintf(...) (__vsnprintf(__VA_ARGS__))

#endif /* PRINTF_H_ */
