#include "uart.h"

void main() {
    uart_init();
    uart_puts("Hello, World!\n");

    uart_puts("Type something:\n");
    while (1) {
        char c = uart_getc();
        uart_putc(c);
    }
}
