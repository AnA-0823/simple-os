#define UART0_BASE 0x09000000UL
#define UART0_DR   (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART0_FR   (*(volatile unsigned int *)(UART0_BASE + 0x18))

void uart_putc(char c) {
    /* 等待 FIFO 非满 */
    while (UART0_FR & (1 << 5));
    UART0_DR = c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');  // 换行前先加回车
        }
        uart_putc(*s++);
    }
}

void main() {
    uart_puts("Hello World on ARM64 Bare Metal!\n");

    while (1) {
        /* 持续空循环 */
    }
}
