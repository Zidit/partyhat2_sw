/*
 * serial.c
 *
 *  Created on: 12.1.2017
 *      Author: Mika
 */

#include "chip.h"
#include "serial.h"
#include "ring_buffer.h"

ring_buffer_t tx;
char tx_buf[64];
ring_buffer_t rx;
char rx_buf[64];


void UART0_IRQHandler(void)
{
	LPC_USART_T *pUART = LPC_USART0;

	if ((Chip_UART_GetStatus(pUART) & UART_STAT_TXRDY) != 0) {
		while (((Chip_UART_GetStatus(pUART) & UART_STAT_TXRDY) != 0) &&
			   !rb_is_empty(&tx)) {

			char c;
			rb_pop(&tx, &c);
			Chip_UART_SendByte(pUART, c);
		}

		if (rb_is_empty(&tx)) {
			Chip_UART_IntDisable(pUART, UART_INTEN_TXRDY);
		}
	}

	while ((Chip_UART_GetStatus(pUART) & UART_STAT_RXRDY) != 0) {
		char c = Chip_UART_ReadByte(pUART);
		rb_push(&rx, c);
	}
}


void serial_init()
{
	rb_init(&tx, tx_buf, sizeof(tx_buf));
	rb_init(&rx, rx_buf, sizeof(rx_buf));

	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_UART0);
	Chip_SYSCTL_PeriphReset(RESET_USART0);

	Chip_UART_ConfigData(LPC_USART0, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1);
	Chip_Clock_SetUSARTNBaseClockRate((9600 * 16), true);

	Chip_UART_Enable(LPC_USART0);
	Chip_UART_TXEnable(LPC_USART0);
	Chip_UART_SetBaud(LPC_USART0, 9600);

	Chip_UART_IntEnable(LPC_USART0, UART_INTEN_RXRDY);
	Chip_UART_IntDisable(LPC_USART0, UART_INTEN_TXRDY);
	NVIC_EnableIRQ(UART0_IRQn);
}

void serial_set_baud(unsigned int baud)
{
	Chip_Clock_SetUSARTNBaseClockRate((baud * 16), true);
	Chip_UART_SetBaud(LPC_USART0, baud);
}

void serial_send_char(char c)
{
	while(rb_is_full(&tx));
	rb_push(&tx, c);
	Chip_UART_IntEnable(LPC_USART0, UART_INTEN_TXRDY);
}

void serial_send_string(char* str)
{
	while(*str)
		serial_send_char(*str++);
}

char serial_data_available()
{
	return !rb_is_empty(&rx);
}

char serial_get_char()
{
	char c;
	rb_pop(&rx, &c);
	return c;
}

void print_char(int c)
{
	serial_send_char(c);
}
