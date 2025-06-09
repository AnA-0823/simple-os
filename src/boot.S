    .section .text
    .global _boot
    .align 7

_boot:
    /* 初始化栈指针 SP */
    ldr x0, =stack_top
    mov sp, x0

    /* 跳转到 C 程序入口 */
    bl main

hang:
    wfe     /* 等待事件，低功耗死循环 */
    b hang

    .section .bss
    .align 7
stack:
    .skip 0x4000         /* 16KB 栈 */
stack_top:
