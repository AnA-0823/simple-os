#ifndef _UART_H
#define _UART_H

#include "types.h"

void uart_init(void);
void uart_putc(char c);
char uart_getc(void);
void uart_puts(const char *str);
void uart_put_hex(uint64 n);

#endif
