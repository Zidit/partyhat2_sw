/*
 * serial.h
 *
 *  Created on: 12.1.2017
 *      Author: Mika
 */

#ifndef SERIAL_H_
#define SERIAL_H_

void serial_init();
void serial_send_char(char c);
void serial_send_string(char* str);
char serial_data_available();
char serial_get_char();
void serial_set_baud(unsigned int baud);

#endif /* SERIAL_H_ */
