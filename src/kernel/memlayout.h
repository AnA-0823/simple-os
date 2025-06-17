#ifndef _MEMLAYOUT_H
#define _MEMLAYOUT_H
// 物理内存布局定义

// QEMU virt 机器的内存布局
// 0x00000000 -- QEMU 提供的启动 ROM
// 0x09000000 -- UART0
// 0x40000000 -- 内核加载地址

#define EXTMEM    0x40000000L               // 扩展内存起始地址
#define PHYSTOP   (EXTMEM + 128*1024*1024)  // 物理内存顶部

// UART 寄存器物理地址
#define UART0 0x09000000L

// 页大小定义
#define PGSIZE 4096

#endif
