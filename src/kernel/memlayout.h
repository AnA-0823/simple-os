#ifndef _MEMLAYOUT_H
#define _MEMLAYOUT_H
// 物理内存布局定义

// QEMU virt 机器的内存布局
// 0x00000000 -- QEMU 提供的启动 ROM
// 0x09000000 -- UART0
// 0x0a000000 -- VIRTIO0 (virtio 设备)
// 0x40000000 -- 内核加载地址

#define MEM_START    0x40000000L               // 扩展内存起始地址
#define MEM_END   (MEM_START + 128*1024*1024)  // 扩展内存结束地址
#define TOTAL_MEM   (MEM_END - MEM_START)      // 总内存大小

// UART 寄存器物理地址
#define UART0 0x09000000L

// VIRTIO 设备物理地址
#define VIRTIO0 0x0a000000L

#endif
