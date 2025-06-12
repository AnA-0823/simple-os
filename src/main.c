#include "uart.h"

void main() {
    uart_init();
    uart_puts("Hello, World!\n");

    while (1) {
        // 回显输入
        char c = uart_getc();
        uart_putc(c);
    }
}
