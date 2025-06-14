.section .text.boot
.globl _start

_start:
    // 禁用 MMU 和缓存
    mrs x0, sctlr_el1
    bic x0, x0, #(1 << 0)  // 禁用 MMU
    bic x0, x0, #(1 << 2)  // 禁用数据缓存
    bic x0, x0, #(1 << 12) // 禁用指令缓存
    msr sctlr_el1, x0
    isb

    // 设置栈指针（在空闲高地址）
    ldr x0, =_stack_top
    mov sp, x0

    // 跳转到 C 语言主函数
    bl main

1:
    wfe
    b 1b

.section .bss
.align 12
.space 4096
_stack_top:
