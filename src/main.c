#include "uart.h"

void main() {
    uart_puts("Hello World on ARM64 Bare Metal!\n");
    while (1) {
        char c = uart_getc();
        uart_putc(c);
        if (c == '\r') {
          uart_putc('\n');
        }
    }
}
