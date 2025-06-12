#include "uart.h"

#define UART0_BASE 0x09000000UL

// PL011 UART 寄存器定义
#define DR    0x00
#define FR    0x18
#define FR_RXFE (1<<4)  // 接收 FIFO 空
#define FR_TXFF (1<<5)  // 发送 FIFO 满
#define IBRD   0x24
#define FBRD   0x28
#define LCRH   0x2c
#define LCRH_FEN  (1<<4)
#define LCRH_WLEN_8BIT  (3<<5)
#define CR     0x30

#define Reg(reg) ((volatile unsigned int *)(UART0_BASE + reg))
#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

void uart_init() {
    // 禁用 UART
    WriteReg(CR, 0);

    // 启用 FIFO，设置 8 位数据位，无校验
    WriteReg(LCRH, LCRH_FEN | LCRH_WLEN_8BIT);

    // 启用 UART，启用发送和接收
    WriteReg(CR, 0x301);

    uart_puts("UART initialized\n");
}

void uart_putc(char c) {
    // 等待发送 FIFO 有空间
    while(ReadReg(FR) & FR_TXFF);
    WriteReg(DR, c);
}

char uart_getc() {
    // 等待接收 FIFO 有数据
    while(ReadReg(FR) & FR_RXFE);
    return ReadReg(DR);
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

