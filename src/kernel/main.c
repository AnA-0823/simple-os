#include "uart.h"
#include "proc.h"
#include "mm.h"
#include "virtio_blk.h"
#include "fat.h"

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
void init_proc_stack(struct proc *p, void (*func)(void), void *stack, uint32 stack_size) {
    // 设置栈指针（栈是向下增长的，所以栈顶在数组末尾）
    void *stack_top = stack + stack_size;
    p->kstack = (uint64)stack_top;
    
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

// 测试 virtio-blk 驱动
void test_virtio_blk(void) {
    uart_puts("Testing virtio block device...\n");

    // 分配一个缓冲区用于测试
    char test_buf[512];

    // 初始化测试数据
    for(int i = 0; i < 512; i++) {
        test_buf[i] = i & 0xFF;
    }

    uart_puts("Writing test data to sector 0...\n");

    // 尝试写入扇区 0
    if(virtio_blk_rw(test_buf, 0, 1) == 0) {
        uart_puts("Write successful\n");

        // 清空缓冲区
        for(int i = 0; i < 512; i++) {
            test_buf[i] = 0;
        }

        uart_puts("Reading from sector 0...\n");

        // 尝试读取扇区 0
        if(virtio_blk_rw(test_buf, 0, 0) == 0) {
            uart_puts("Read successful\n");

            // 验证数据
            int correct = 1;
            for(int i = 0; i < 512; i++) {
                if(test_buf[i] != (i & 0xFF)) {
                    correct = 0;
                    break;
                }
            }

            if(correct) {
                uart_puts("Data verification successful!\n");
            } else {
                uart_puts("Data verification failed!\n");
            }
        } else {
            uart_puts("Read failed\n");
        }
    } else {
        uart_puts("Write failed\n");
    }
}

void test_fat(void) {
    uart_puts("\nFAT 文件系统测试开始\n");
    // 1. 新建文件并写入内容
    const char *fn1 = "TEST1   TXT";
    const char *fn2 = "TEST2   TXT";
    char buf1[32] = "Hello, this is file 1!";
    char buf2[32] = "File 2, first content.";
    int w1 = fat_write_file(fn1, buf1, 23, 0);
    int w2 = fat_write_file(fn2, buf2, 22, 0);
    uart_puts("新建并写入TEST1.TXT字节数: "); uart_put_hex(w1); uart_puts("\n");
    uart_puts("新建并写入TEST2.TXT字节数: "); uart_put_hex(w2); uart_puts("\n");
    // 2. 覆盖写入同名文件
    char buf1b[32] = "Overwrite file 1!";
    int w1b = fat_write_file(fn1, buf1b, 18, 0);
    uart_puts("覆盖写入TEST1.TXT字节数: "); uart_put_hex(w1b); uart_puts("\n");
    // 3. 读取文件内容
    char rbuf[40];
    int r1 = fat_read_file(fn1, rbuf, 23, 0);
    rbuf[r1 > 0 ? r1 : 0] = 0;
    uart_puts("读取TEST1.TXT内容: "); uart_puts(rbuf); uart_puts("\n");
    int r2 = fat_read_file(fn2, rbuf, 22, 0);
    rbuf[r2 > 0 ? r2 : 0] = 0;
    uart_puts("读取TEST2.TXT内容: "); uart_puts(rbuf); uart_puts("\n");
    // 4. 读取不存在的文件
    int r3 = fat_read_file("NOFILE  TXT", rbuf, 20, 0);
    uart_puts("读取NOFILE.TXT返回: "); uart_put_hex(r3); uart_puts(" (应为-1)\n");
    // 5. 遍历根目录
    uart_puts("根目录文件列表:\n");
    struct fat_dir_entry entries[16];
    int n = fat_list_dir("/", entries, 16);
    for (int i = 0; i < n; i++) {
        if (entries[i].name[0] == 0x00 || entries[i].name[0] == 0xE5) continue; // 跳过空/已删除
        // 打印8.3文件名
        char name[12];
        for (int j = 0; j < 8; j++) name[j] = entries[i].name[j];
        name[8] = '.';
        for (int j = 0; j < 3; j++) name[9 + j] = entries[i].name[8 + j];
        name[11] = 0;
        uart_puts("  ");
        uart_puts(name);
        uart_puts(" size: ");
        uart_put_hex(entries[i].size);
        uart_puts("\n");
    }
    uart_puts("[TEST] FAT 文件系统测试结束\n\n");
}

void test_proc_and_mm(void) {
    // 创建三个测试进程
    struct proc *p1 = proc_alloc();
    struct proc *p2 = proc_alloc();
    struct proc *p3 = proc_alloc();

    if(!p1 || !p2 || !p3) {
        uart_puts("Failed to allocate processes!\n");
        return;
    }

    // 每个进程请求16kb栈
    void *stack1 = alloc_pages(4);
    void *stack2 = alloc_pages(4);
    void *stack3 = alloc_pages(4);

    // 初始化进程栈和上下文
    init_proc_stack(p1, proc1_func, stack1, 16 * 1024);
    init_proc_stack(p2, proc2_func, stack2, 16 * 1024);
    init_proc_stack(p3, proc3_func, stack3, 16 * 1024);

    // 将进程加入就绪队列
    p1->state = RUNNABLE;
    p2->state = RUNNABLE;
    p3->state = RUNNABLE;


    // 启动调度器
    scheduler();
}

void main(void) {
    // 初始化串口
    uart_init();
    // 初始化进程管理
    proc_init();
    // 初始化内存管理
    init_mm();
    // 初始化 virtio 块设备
    virtio_blk_init();
    // 初始化 FAT 文件系统
    fat_init();

    // FAT 文件系统测试
    test_fat();
    // 测试进程管理和内存管理
    test_proc_and_mm();
    
    // 如果调度器返回（不应该发生），则停止系统
    uart_puts("Main function returned. System halted.\n");
    while(1);
}
