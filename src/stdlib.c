#include "stdlib.h"

void* memcpy(void* destination, const void* source, size_t num)
{
	while(num--)
        *((unsigned char*)destination + num) = *((unsigned char*)source + num);
    return destination;
}

void* memmove(void * dest, const void* src, size_t num)
{
	signed char operation;
	size_t end;
	size_t current;
  
	if(dest != src) {
	  if(dest < src) {
		operation = 1;
		current = 0;
		end = num;
	  } else {
		operation = -1;
		current = num - 1;
		end = -1;
	  }
  
	  for( ; current != end; current += operation) {
		*(((unsigned char*)dest) + current) = *(((unsigned char*)src) + current);
	  }
	}
	return dest;
}

size_t strlen(const char* str)
{
	int i = 0;
	while(*(str++))
		i++;
	return i;
}

void* memset(void* ptr, int value, size_t size)
{
	while(size--)
		*((unsigned char*)ptr + size) = value;
	return ptr;
}

char* strcpy(char *dest, const char *src)
{
	char* org = dest;
	while(*src)
		*(dest++) = *(src++);
	*(dest) = *(src);
	return org;
}

int strncmp (const char* str1, const char* str2, size_t num)
{
	while(num-- && *str1 && *str2 && *(str1) == *(str2)) {
		str1++;
		str2++;
	}
	if(!*str1 || !*str2)
		return 0;

	return *str1 - *str2;
}

int num_to_int(const char* str, int *value)
{
	int i = 0;
	*value = 0;

	while(str[i] >= '0' && str[i] <= '9') {
		*value *= 10;
		*value += str[i++] - '0';
	}

	return i;
}

char* int_to_num(char* buf, unsigned int value, int base, int ucase)
{
	int i = 0;
	buf[i++] = '\0';

	if(value == 0) {
		buf[i++] = '0';
	} else {
		while(value) {
			int digit = (value % base);
			buf[i++] = digit < 10 ? ('0' + digit) : (ucase ? 'A' : 'a') + digit - 10;
			value /= base;
		}
	}
	i--;

	//reverse string
	int j = 0;
	while(j < i) {
		char c = buf[j];
		buf[j] = buf[i];
		buf[i] = c;
		i--;
		j++;
	}

	return buf;
}

int atoi(const char* str)
{
	int value;
	num_to_int(str, &value);
	return value;
}

int isdigit(int c)
{
	if(c >= '0' && c <= '9')
		return 1;
	else
		return 0;
}