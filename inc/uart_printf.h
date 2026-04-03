#ifndef UART_PRINTF_H
#define UART_PRINTF_H

#include <stdarg.h>

void uart_printf(const char *format, ...);
void uart_puts(const char *str);
void uart_putchar(char c);

void uart_tx_start(void);

void uart_write_byte(char c);
void uart_tx_start(void);
void uart_tx_irq_handler(void);

#endif
