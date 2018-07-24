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

int strcmp (const char* str1, const char* str2)
{
	while(*str1 && *str2 && *(str1) == *(str2)) {
		str1++;
		str2++;
	}

	return *str1 - *str2;
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
	while(*str && isspace(*str))
		str++;

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

int isspace(int c)
{
	if(c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r')
		return c;
	else
		return 0;
}

int toupper(int c)
{
	if(c >= 'a' && c <= 'z')
		return c - 32;
	else
		return c; 
}

const char* strchr(const char* str, int character)
{
	while(*str++){
		if(*str == character)
			return str;
	}

	return NULL;
}

char* strpbrk(char* str1, const char* str2)
{
	int pos;
	while(*str1++){
		pos = 0;
		while(*(str2 + pos))
			if(*(str2 + pos++) == *str1)
				return str1;
	}

	return NULL;
}

char* strtok_r(char* str, const char* delimiters, char **saveptr)
{
	if(str)
		*saveptr = str;

	char* start_pos = *saveptr;
	char* new_pos = strpbrk(start_pos, delimiters);

	if(new_pos == NULL || *new_pos == '\0')
		return NULL;

	*new_pos = '\0';
	*saveptr = new_pos + 1;
	return start_pos;	
}

char* strtok(char* str, const char* delimiters)
{
	static char* saveptr;
	return strtok_r(str, delimiters, &saveptr);
}
