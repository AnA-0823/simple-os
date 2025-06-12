#include "uart.h"

#define UART0_BASE 0x09000000UL
#define UART0_DR   (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART0_FR   (*(volatile unsigned int *)(UART0_BASE + 0x18))

void uart_init() {
    // 简化：PL011 已由 QEMU 初始化
}

void uart_putc(char c) {
    while (UART0_FR & (1 << 5)); // 等待TX FIFO非空
    UART0_DR = c;
}

char uart_getc() {
	while (UART0_FR & (1 << 4)); // 等待RX FIFO非空
	return UART0_DR;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');  // 换行前先加回车
        }
        uart_putc(*s++);
    }
}

