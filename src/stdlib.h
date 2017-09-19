

#define NULL ((void*)0)

typedef unsigned int size_t;

void* memcpy(void* destination, const void* source, size_t num);
void* memmove(void * dest, const void* src, size_t num);
size_t strlen(const char* str);
void* memset(void* ptr, int value, size_t size);
char* strcpy(char *dest, const char *src);
int strncmp (const char* str1, const char* str2, size_t num);
int num_to_int(const char* str, int *value);
char* int_to_num(char* buf, unsigned int value, int base, int ucase);
int atoi(const char* str);
int isdigit(int c);