/*
 * editor.c
 *
 *  Created on: 18.1.2017
 *      Author: Mika
 */

#include "nvm.h"
#include "editor.h"
#include "serial.h"
#include "printf.h"

#include <string.h>
#include <stdbool.h>
#include "utils.h"

char buffer[1024] __attribute__((aligned(256)));
int cursor_x = 1;
int cursor_y = 1;
bool file_modified = false;


char get_char()
{
	while(!serial_data_available());
	char c = serial_get_char();
	return c;
}

void set_cursor()
{
	printf("\033[100;60H\033[K %i,%i  %i/1023", cursor_x, cursor_y, strlen(buffer));
	printf("\033[%i;%iH", cursor_y, cursor_x);
}

char* find_line(char* buf, int line)
{
	line--;

	while(*buf) {
		if(line <= 0)
			return buf;

		if(*buf++ == '\n')
			line--;
	}

	return buf;
}

int line_length(char* buf, int line)
{
	buf = find_line(buf, line);
	int i = 1;
	while(*buf != '\0' && *buf != '\n' ) {
		buf++;
		i++;
	}
	return i;
}

int number_of_lines(char* buf)
{
	int i = 1;
	while(*buf != '\0') {
		if(*buf++ == '\n')
			i++;
	}

	return i;
}

void handle_escape()
{
	char c;
	while((c = get_char()) == '\e');

	if(c == '[') {
		c = get_char();

		switch(c) {
		case 'A':
			cursor_y--;
			break;
		case 'B':
			cursor_y++;
			break;
		case 'C':
			cursor_x++;
			break;
		case 'D':
			cursor_x--;
			break;
		case '3':
			break;

		default:
			printf("!%c!", c);
			break;
		}

		cursor_y = clamp(cursor_y, 1, number_of_lines(buffer));
		cursor_x = clamp(cursor_x, 1, line_length(buffer, cursor_y));
		set_cursor();
	} else {
		//Shouldn't happen
		printf("##: 0x%x", c);
	}
}

void full_refresh()
{
	printf("\033[2J\033[H%s", buffer);
	printf("\033[100;1Hctrl+x: exit, ctrl+o: save, ctrl+r: open, ctrl+h: help");
	set_cursor();
}

void refresh_pos(char* pos)
{
	printf("%s", pos);
	set_cursor();
}

void refresh_line_pos(char* pos)
{
	printf("\033[K", cursor_y);

	while(*pos && *pos != '\n')
		print_char(*pos++);

	set_cursor();
}

void refresh_line(int line)
{
	printf("\033[2K\033[%i;1H", line);

	char* pos = find_line(buffer, cursor_y);

	while(*pos && *pos != '\n')
		print_char(*pos++);

	set_cursor();
}

void add_character(char* pos, char c)
{
	if(strlen(buffer) >= 1023)
		return;

	file_modified = true;

	if(c == '\n' || c == '\r') {
		cursor_y++;
		cursor_x = 1;
		memmove(pos + 1, pos, strlen(pos));
		*pos = '\n';
		full_refresh();
	} else {
		cursor_x++;
		memmove(pos + 1, pos, strlen(pos));
		*pos = c;
		refresh_line_pos(pos);
	}
}

void remove_character(char* pos)
{
	if(pos == buffer)
		return;

	if(cursor_x == 1 && cursor_y > 1)
		cursor_x = line_length(buffer, --cursor_y);
	else
		cursor_x = max(cursor_x - 1, 1);


	file_modified = true;

	char removed_char = *(pos - 1);

	int len = strlen(pos);
	memmove(pos - 1, pos, len);
	*(pos + len - 1) = '\0';


	if(removed_char == '\n') {
		full_refresh();
	} else {
		printf("\033[D");
		refresh_line_pos(pos - 1);
	}
}

void delete_character(char* pos)
{
	file_modified = true;

	char removed_char = *(pos);

	int len = strlen(pos + 1);
	memmove(pos, pos + 1, len);
	*(pos + len) = '\0';

	if(removed_char == '\n')
		full_refresh();
	else
		refresh_line_pos(pos);
}

int get_sector_input(int cur)
{
	printf("\033[%i;1H\033[KSelect sector (1-8), current = %i: ", 100, cur + 1);

	while(1){
		char c = get_char();
		if(c >= '1' && c <= '8') {
			return c - '1';
		} else if(c == '\r' || c == '\n'){
			return cur;
		}
	}
}

int editor(int sector)
{
	int current = sector;
	load_file(sector, buffer);

	cursor_x = 1;
	cursor_y = 1;
	file_modified = false;
	
	full_refresh();	

	while(1)
	{
		char c = get_char();

		char* line = find_line(buffer, cursor_y);
		char* pos  = line + cursor_x - 1;

		switch (c)
		{
		case '\e':	//VT100 commands
			handle_escape();
			break;

		case 127:	//Backspace
			remove_character(pos);
			break;

		case 126:	//Delete
			delete_character(pos);
			break;

		case  24:	//CTRL+X
			if(file_modified){
				printf("\033[%i;1H\033[KFile %i modified: [s]ave/[c]ansel/[i]gnore", 100, current + 1);

				while(1){
					char c = get_char();
					if(c == 's'){
						int s = get_sector_input(current);
						save_file(s, buffer);
						return s;
					} else if(c == 'c'){
						break;
					} else if(c == 'i'){
						return current;
					}
				}
			} else {
				return current;
			}
			break;

		case 15: {	//CTRL+O
			file_modified = false;
			int s = get_sector_input(current);
			save_file(s, buffer);
			printf("\033[%i;1H\033[2Ksaved", 100);
			break;
		}

		case 18: {	//CTRL+R
			file_modified = false;
			int s = get_sector_input(current);
			current = s;
			load_file(s, buffer);
			cursor_x = 1;
			cursor_y = 1;
			printf("\033[%i;1H\033[2Kopened", 100);
			full_refresh();
			break;
		}

		case 8:	//CTRL+H help
			printf("\033[2J\033[H");
			printf("-Variable names are one letter long.\n");
			printf("-Use peek(get) and poke(set) modify led buffer.\n");
			printf(" Addresses are from 0 to 900 in following order:\n");
			printf(" 0 = 0g, 1 = 0r, 2 = 0b, 3 = 1g, 4 = 1r, ... 898 = 299r, 899 = 299b\n");
			printf("-Use peek address >1000 to get configs. Edit configs in page 8\n");
			printf(" Address 1000: strip length\n");
			printf("-Use clr to sets led buffer to 0.\n");
			printf("-Use update [leds] to write led buffer to the strip.\n");
			printf(" [led] is number of leds to update (usually length of the strip).\n");
			printf("-Use rnd as variable to get random 32-bit number.\n");
			printf("-Use sin([angle]) to get sine. Angle is between 0 and 255.\n");
			printf(" Return value is between -128 and 127.\n");

			break;

		default:
			add_character(pos, c);
			break;
		}
	}
}
