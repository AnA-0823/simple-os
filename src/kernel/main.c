#include "uart.h"
#include "proc.h"

// 测试进程的栈大小
#define STACK_SIZE 4096

// 测试进程的栈空间
static uint64 proc1_stack[STACK_SIZE] __attribute__((aligned(16)));
static uint64 proc2_stack[STACK_SIZE] __attribute__((aligned(16)));
static uint64 proc3_stack[STACK_SIZE] __attribute__((aligned(16)));

// 测试进程函数
void proc1_func(void) {
    while(1) {
        uart_puts("Process 1 running...\n");
        uart_puts("Process 1 need input:\n");
        char c;
        do {
          c = uart_getc();
          uart_putc(c);
        } while (c != '\r');
        uart_puts("\n");
        yield();
    }
}

void proc2_func(void) {
    while(1) {
        uart_puts("Process 2 running...\n");
        uart_puts("Process 2 need input:\n");
        char c;
        do {
          c = uart_getc();
          uart_putc(c);
        } while (c != '\r');
        uart_puts("\n");
        yield();
    }
}

void proc3_func(void) {
    while(1) {
        uart_puts("Process 3 running...\n");
        uart_puts("Process 3 need input:\n");
        char c;
        do {
          c = uart_getc();
          uart_putc(c);
        } while (c != '\r');
        uart_puts("\n");
        yield();
    }
}

// 打印进程上下文信息
static void print_context_info(struct context *ctx, const char *name) {
    uart_puts("\nContext info for ");
    uart_puts(name);
    uart_puts(":\n");
    uart_puts("x30 (return address) = 0x");
    uart_put_hex(ctx->x30);
    uart_puts("\n");
    uart_puts("sp = 0x");
    uart_put_hex(ctx->sp);
    uart_puts("\n");
}

// 初始化进程栈和上下文
void init_proc_stack(struct proc *p, void (*func)(void), uint64 *stack) {
    // 设置栈指针（栈是向下增长的，所以栈顶在数组末尾）
    uint64 *stack_top = stack + STACK_SIZE;
    p->kstack = (uint64 *)stack_top;
    
    // 设置上下文
    p->context.sp = (uint64)stack_top;  // 栈指针指向栈顶
    p->context.x30 = (uint64)func;      // 链接寄存器指向进程函数
    
    // 初始化其他寄存器
    p->context.x18 = 0;  // 平台寄存器
    p->context.x19 = 0;
    p->context.x20 = 0;
    p->context.x21 = 0;
    p->context.x22 = 0;
    p->context.x23 = 0;
    p->context.x24 = 0;
    p->context.x25 = 0;
    p->context.x26 = 0;
    p->context.x27 = 0;
    p->context.x28 = 0;
    p->context.x29 = 0;  // 帧指针

    // 打印初始化信息
    uart_puts("\nInitializing process ");
    uart_putc('0' + p->pid);
    uart_puts("\n");
    uart_puts("Stack base = 0x");
    uart_put_hex((uint64)stack);
    uart_puts("\n");
    uart_puts("Stack top = 0x");
    uart_put_hex((uint64)stack_top);
    uart_puts("\n");
    uart_puts("Function address = 0x");
    uart_put_hex((uint64)func);
    uart_puts("\n");
    print_context_info(&p->context, "initial context");
}

void main(void) {
    // 初始化串口
    uart_init();
    // 初始化进程管理
    proc_init();
    
    // 创建三个测试进程
    struct proc *p1 = proc_alloc();
    struct proc *p2 = proc_alloc();
    struct proc *p3 = proc_alloc();
    
    if(!p1 || !p2 || !p3) {
        uart_puts("Failed to allocate processes!\n");
        return;
    }
    
    // 初始化进程栈和上下文
    init_proc_stack(p1, proc1_func, proc1_stack);
    init_proc_stack(p2, proc2_func, proc2_stack);
    init_proc_stack(p3, proc3_func, proc3_stack);
    
    // 将进程加入就绪队列
    p1->state = RUNNABLE;
    p2->state = RUNNABLE;
    p3->state = RUNNABLE;
    

    // 启动调度器
    scheduler();
    
    // 如果调度器返回（不应该发生），则停止系统
    uart_puts("Main function returned. System halted.\n");
    while(1);
}
