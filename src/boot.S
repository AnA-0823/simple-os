.section .text.boot
.globl _start

_start:
    // 设置栈指针（在空闲高地址）
    ldr x0, =_stack_top
    mov sp, x0

    // 跳转到 C 语言主函数
    bl main

1:
    wfe
    b 1b

.section .bss
    .space 4096
_stack_top:
