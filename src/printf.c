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


#include "ctype.h"
#include "printf.h"

#define FLAG_LEFT_JUST		(1 << 0)
#define FLAG_FORCE_SIGN		(1 << 1)
#define FLAG_BLANK_SIGN		(1 << 2)
#define FLAG_PRECEED		(1 << 3)
#define FLAG_PAD_ZERO		(1 << 4)

#define iszero(f) (((*(uint32_t*)&f) & 0x7FFFFFFF) == 0)
#define abs(v) (v < 0 ? -v : v)

static struct stream std_strm = {
	(void*)0,
	0,
	0,
	&print_char
};

struct sub_specifiers {
	int f;
	int w;
	int p;
};


void* memset(void* ptr, int value, int size)
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

char* strapp(char *dest, const char *src)
{
	while(*src)
		*(dest++) = *(src++);
	return dest;
}


#define min(a,b) \
	({ __typeof__ (a) _a = (a); \
	  __typeof__ (b) _b = (b); \
	  _a < _b ? _a : _b;})

static int num_to_int(const char* str, int *value)
{
	int i = 0;
	*value = 0;

	while(str[i] >= '0' && str[i] <= '9') {
		*value *= 10;
		*value += str[i++] - '0';
	}

	return i;
}

static char* int_to_num(char* buf, unsigned int value, int base, int ucase)
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

static int _strlen(char* str)
{
	int i = 0;
	while(*(str++))
		i++;
	return i;
}

static int get_flags(const char* stream, int* chars_got)
{
	int flags = 0;
	*chars_got = 0;

	while(1) {
		switch(*(stream++)) {
			case '-':
				flags |= FLAG_LEFT_JUST;
				break;
			case '+':
				flags |= FLAG_FORCE_SIGN;
				break;
			case ' ':
				flags |= FLAG_BLANK_SIGN;
				break;
			case '#':
				flags |= FLAG_PRECEED;
				break;
			case '0':
				flags |= FLAG_PAD_ZERO;
				break;
			default:
				return flags;
		}

		(*chars_got)++;
	}
}

static int get_width(const char* stream, int* chars_got, va_list* arg)
{
	*chars_got = 0;

	if(*stream == '*') {
		*chars_got = 1;
		return va_arg(*arg, int);
	}

	if(isdigit(*stream)) {
		int length;
		*chars_got = num_to_int(stream, &length);
		return length;
	}

	return -1;
}

static int get_precision(const char* stream, int* chars_got, va_list* arg)
{
	*chars_got = 0;

	if(*stream != '.')
		return -1;

	(*chars_got)++;
	stream++;

	if(*stream == '*') {
		(*chars_got)++;
		return va_arg(*arg, int);
	}

	if(isdigit(*stream)) {
		int precision;
		*chars_got += num_to_int(stream, &precision);
		return precision;
	}

	return 0;
}

static int print_to_stream(int c, struct stream *strm)
{
	if(strm->buffer){
		if(strm->buffer_size > strm->pos)
			strm->buffer[strm->pos++] = c;
	}

	if(strm->printchar)
		strm->printchar(c);

	return 1;
}


static int printstr(char* str, struct stream *strm)
{
	int i;
	for(i = 0; *str; i++)
		print_to_stream(*str++, strm);
	return i;
}

static int printchars(char c, int len, struct stream *strm)
{
	if(len <= 0)
		return 0;
	int i;
	for(i = 0; i < len; i++)
		print_to_stream(c, strm);
	return len;
}


int printchar(int c, struct stream *strm)
{
	print_to_stream(c, strm);
	return 1;
}

static int print_with_padding(char* value, char* sign, struct sub_specifiers *ss, struct stream *strm)
{
	int chars_printed = 0;
	int left_pad = 0;
	if(!(ss->f & FLAG_LEFT_JUST)) {
		left_pad = ss->w - (ss->p > _strlen(value) ? ss->p : _strlen(value));
		if(sign[0])
			left_pad -= _strlen(sign);
		if(left_pad < 0)
			left_pad = 0;

		if(!(ss->f & FLAG_PAD_ZERO))
			chars_printed += printchars(' ', left_pad, strm);
	}

	chars_printed += printstr(sign, strm);

	if(ss->f & FLAG_PAD_ZERO)
		chars_printed += printchars('0', left_pad, strm);

	chars_printed += printchars('0', ss->p - _strlen(value), strm);
	chars_printed += printstr(value, strm);

	if(ss->f & FLAG_LEFT_JUST)
		chars_printed += printchars(' ', ss->w - chars_printed, strm);

	return chars_printed;
}


static int print_int(int value, struct sub_specifiers *ss, struct stream *strm)
{
	if(ss->p == 0 && value == 0)
		return 0;

	char buf[11];
	int_to_num(buf, abs(value), 10, 0);

	char* sign = "";
	if(value < 0) {
		sign = "-";
	} else if(ss->f & FLAG_FORCE_SIGN){
		sign = "+";
	} else if(ss->f & FLAG_BLANK_SIGN){
		sign = " ";
	}

	return print_with_padding(buf, sign, ss, strm);
}

static int print_uint(unsigned int value, struct sub_specifiers *ss, struct stream *strm)
{
	if(ss->p == 0 && value == 0)
		return 0;

	char buf[11];
	int_to_num(buf, value, 10, 0);
	char* sign = "";

	return print_with_padding(buf, sign, ss, strm);
}

static int print_octal(unsigned int value, struct sub_specifiers *ss, struct stream *strm)
{
	if(ss->p == 0 && value == 0)
		return 0;

	char buf[12];
	int_to_num(buf, value, 8, 0);
	char* sign = "";
	if((ss->f & FLAG_PRECEED) && value)
		sign = "0";

	return print_with_padding(buf, sign, ss, strm);
}

static int print_hex(unsigned int value, struct sub_specifiers *ss, struct stream *strm)
{
	if(ss->p == 0 && value == 0)
		return 0;

	char buf[9];
	int_to_num(buf, value, 16, 0);
	char* sign = "";
	if((ss->f & FLAG_PRECEED) && value)
		sign = "0x";

	return print_with_padding(buf, sign, ss, strm);
}

static int print_hex_upper(unsigned int value, struct sub_specifiers *ss, struct stream *strm)
{
	if(ss->p == 0 && value == 0)
		return 0;

	char buf[9];
	int_to_num(buf, value, 16, 1);
	char* sign = "";
	if(ss->f & FLAG_PRECEED)
		sign = "0X";

	return print_with_padding(buf, sign, ss, strm);
}

static int print_pointer(void* ptr, struct sub_specifiers *ss, struct stream *strm)
{
	char buf[9];
	int_to_num(buf, (unsigned int) ptr, 16, 0);
	char* sign = "";
	if(ss->f & FLAG_PRECEED)
		sign = "0X";

	ss->f &= ~FLAG_PAD_ZERO;
	ss->p = 8;
	return print_with_padding(buf, sign, ss, strm);
}

static int print_string(char* string, struct sub_specifiers *ss, struct stream *strm)
{
	if(string == (void*)0) {
		return printstr("(null)", strm);
	}

	int chars_printed = 0;
	int left_pad = 0;
	int string_len = _strlen(string);
	if(ss->p > 0)
		string_len = (string_len < ss->p) ? string_len : ss->p;

	if(!(ss->f & FLAG_LEFT_JUST)) {
		left_pad = ss->w - string_len;
		if(left_pad < 0)
			left_pad = 0;

		if(ss->f & FLAG_PAD_ZERO)
			chars_printed += printchars('0', left_pad, strm);
		else
			chars_printed += printchars(' ', left_pad, strm);
	}

	while(string_len--){
		print_to_stream(*(string++), strm);
		chars_printed++;
	}

	if(ss->f & FLAG_LEFT_JUST)
		chars_printed += printchars(' ', ss->w - chars_printed, strm);

	return chars_printed;
}

static int print_character(int c, struct sub_specifiers *ss, struct stream *strm)
{
	char buf[2];
	buf[0] = c;
	buf[1] = '\0';

	return print_with_padding(buf, "", ss, strm);
}

#ifdef PRINT_FLOAT

static int float_to_str_integer(char* str, int str_size, uint32_t man, uint32_t exp)
{
	memset(str, '0', str_size);

	int digits = 0;
	int i;

	for (i = 0; i < exp + 1; i++) {
		int32_t j;
		uint32_t carry;

		carry = man & (1 << (23 - i)) ? 1 : 0;

		for (j = digits; j >= 0; j--) {
			str[j] += str[j] - '0' + carry;
			carry = (str[j] > '9') ? 1 : 0;

			if (carry) {
				str[j] -= 10;
				if(j == 0) {
					int k;
					for (k = str_size - 1; k >= 0; k--)
						str[k + 1] = str[k];

					str[0] = '1';
					digits++;
				}
			}
		}
	}

	str[str_size - 1] = '\0';
	digits++;
	return digits;
}

static int float_to_str_fraction_div2(char* str, int str_size, uint32_t digits)
{
	int32_t j;
	for (j = digits; j >= 0; j--) {
		if((str[j] & 0x1) && (j < str_size - 2)) { //is odd
			str[j +1 ] += 5;
			if(j + 1 > digits)
				digits++;
		}

		str[j] /= 2;
	}

	return digits;
}

static int float_to_str_fraction(char* str, int str_size, uint32_t man, int32_t exp)
{
	memset(str, 0, str_size);

	int i;
	int digits = 0;
	int mantisa_len = 24;
	if(exp > -2)
		mantisa_len -= (exp + 2);

	if(mantisa_len < 0) {
		memset(str, '0', str_size);
		return 8;
	}

	if(man & 0x01)
		str[0] += 5;

	for (i = 0; i < mantisa_len ; i++) {
		digits = float_to_str_fraction_div2(str, str_size, digits);

		man >>= 1;
		if(man & 0x01)
			str[0] += 5;
	}

	if(exp < -2) {
		i = abs(exp + 2);
		while(i--) {
			digits = float_to_str_fraction_div2(str, str_size, digits);
		}
	}

	for(i = 0; i < str_size; i++)
		str[i] += '0';

	digits++;
	return digits;
}

static char* float_string(float value, char* buffer, struct sub_specifiers *ss, int ucase)
{
	char* buf = buffer;

	if(isnan(value))
		return strcpy(buf, ucase ? "NAN" : "nan");
	else if(isinf(value))
		return strcpy(buf, ucase ? "INF" : "inf");
	else if(iszero(value))
		return strcpy(buf, "0");

	uint32_t v = *(uint32_t*)&value;
	uint32_t sign = v >> 31;
	int32_t exp = ((v >> 23) & 0xff) - 127;
	uint32_t man = v & ((1 << 23) - 1);
	man |= (1 << 23);

	if(exp < 0) {
		if(ss->p == 0 && value >= 0.5f)
			buf = strapp(buf, "1");
		else
			buf = strapp(buf, "0.");
	} else {
		char str[17];
		int a = float_to_str_integer(str, sizeof(str), man, exp);

		int b10_man = min(a, sizeof(str) - 1);
		int b10_exp = a - b10_man;

		str[b10_man] = '\0';

		buf = strapp(buf, str);
		if(b10_exp > 0)
			while(b10_exp--)
				buf = strapp(buf, "0");
		buf = strapp(buf, ".");
	}

	char str[17];
	int precision = float_to_str_fraction(str, sizeof(str), man, exp);

	precision = ss->p;
	str[precision] = '\0';
	buf = strapp(buf, str);
	*buf = '\0';
	return buf;
}

static char* float_string_e(float value, char* buffer, struct sub_specifiers *ss, int ucase)
{
	char* buf = buffer;

	if(isnan(value))
		return strcpy(buf, ucase ? "NAN" : "nan");
	else if(isinf(value))
		return strcpy(buf, ucase ? "INF" : "inf");
	else if(iszero(value))
		return strcpy(buf, "0");

	uint32_t v = *(uint32_t*)&value;
	uint32_t sign = v >> 31;
	int32_t exp = ((v >> 23) & 0xff) - 127;
	uint32_t man = v & ((1 << 23) - 1);
	man |= (1 << 23);

	int precision = ss->p;

	if(exp > 0) {
		char str[17];
		int a = float_to_str_integer(str, sizeof(str), man, exp);

		*(buf++) = *str;
		buf = strapp(buf, ".");

		int b10_man = min(a, sizeof(str) - 2);
		int b10_exp = a - b10_man;
		int p = min(b10_man, precision + 1);
		precision -= p - 1 < 0 ? 0 : p - 1;
		str[p] = '\0';

		buf = strapp(buf, str + 1);

	}

	//buf = strapp(buf, "!");

	if(precision) {
		char str[17];
		int precision = float_to_str_fraction(str, sizeof(str), man, exp);

		precision = ss->p;
		str[precision] = '\0';
		buf = strapp(buf, str);
	}

	*buf = '\0';
	return buf;
}

#endif //PRINT_FLOAT

static int print_float(float value, struct sub_specifiers *ss, struct stream *strm, char specifier)
{
#ifdef PRINT_FLOAT
	char buffer[64] = {'\0'};

	if(ss->p < 0)
		ss->p = 6;

	if(specifier == 'e')
		float_string_e(value, buffer, ss, 0);
	else if(specifier == 'E')
		float_string_e(value, buffer, ss, 1);
	else if (specifier == 'f')
		float_string(value, buffer, ss, 0);
	else if (specifier == 'F')
		float_string(value, buffer, ss, 1);
	else
		return 0;

	char* sign = "";
	if(value < 0) {
		sign = "-";
	} else if(ss->f & FLAG_FORCE_SIGN){
		sign = "+";
	} else if(ss->f & FLAG_BLANK_SIGN){
		sign = " ";
	}

	return print_with_padding(buffer, sign, ss, strm);
#else //PRINT_FLOAT
	return print_with_padding("NO FLOAT", "", ss, strm);
#endif //PRINT_FLOAT
}

static int print_variable(char specifier, struct sub_specifiers *ss, va_list *arg, struct stream *strm)
{
	if(specifier == 'd' || specifier == 'i')
		return print_int(va_arg(*arg, int), ss, strm);
	else if(specifier == 'u')
		return print_uint(va_arg(*arg, unsigned int), ss, strm);
	else if(specifier == 'o')
		return print_octal(va_arg(*arg, unsigned int), ss, strm);
	else if(specifier == 'x')
		return print_hex(va_arg(*arg, unsigned int), ss, strm);
	else if(specifier == 'X')
		return print_hex_upper(va_arg(*arg, unsigned int), ss, strm);
	else if(specifier == 'c')
		return print_character(va_arg(*arg, int), ss, strm);
	else if(specifier == 's')
		return print_string(va_arg(*arg, char*), ss, strm);
	else if(specifier == 'p')
		return print_pointer(va_arg(*arg, void*), ss, strm);
	else if(specifier == '%')
		return print_to_stream('%', strm);

	else if(specifier == 'f' || specifier == 'F')
		return print_float(va_arg(*arg, double), ss, strm, specifier);
	else
		return 0;
}

int __vprintf(const char * format, va_list arg)
{
	struct stream strm = std_strm;
	return __vfprintf(&strm, format, arg);
}

int __vsprintf(char *str, const char *format, va_list arg)
{
	struct stream strm = {
		str,
		0xFFFFFFFF,
		0,
		(void*)0
	};

	int ret = __vfprintf(&strm, format, arg);
	strm.buffer[strm.pos] = '\0';
	return ret;
}

int __vsnprintf(char *str, unsigned int size, const char *format, va_list arg)
{
	struct stream strm = {
		str,
		size - 1,
		0,
		(void*)0
	};

	__vfprintf(&strm, format, arg);
	strm.buffer[strm.pos] = '\0';
	return strm.pos;
}

int __vfprintf(struct stream *strm, const char * format, va_list arg)
{
	char c;
	int tot = 0;

	for(;(c = *format); format++) {
		if(c == '%') {
			format++;

			struct sub_specifiers ss;

			int chars_taken = 0;
			ss.f = get_flags(format, &chars_taken);
			format += chars_taken;

			ss.w = get_width(format, &chars_taken, &arg);
			format += chars_taken;

			if(ss.w < 0) {
				ss.w = -ss.w;
				ss.f |= FLAG_LEFT_JUST;
			}

			ss.p = get_precision(format, &chars_taken, &arg);
			format += chars_taken;

			char specifier = *format;
			if(specifier == '\0')
				return tot;
			if(specifier == 'n'){
				*((int*)va_arg(arg, void*)) = tot;
			} else {
				tot += print_variable(specifier, &ss, &arg, strm);
			}
		} else {
			tot ++;
			print_to_stream(c,strm);
		}
	}

	return tot;
}

int __printf(const char * format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = __vprintf(format, args);
	va_end(args);

	return ret;
}

int __fprintf(struct stream *strm, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = __vfprintf(strm, format, args);
	va_end(args);

	return ret;
}

int __sprintf(char *str, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = __vsprintf(str, format, args);
	va_end(args);

	return ret;
}

int __snprintf(char *str, unsigned int size, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int ret = __vsnprintf(str, size, format, args);
	va_end(args);

	return ret;
}
