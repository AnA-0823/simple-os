#include "aarch64.h"
#include "proc.h"
#include "uart.h"

// 进程表
static struct proc proc[NPROC];
// CPU 表
static struct cpu cpus[NCPU];

int nextpid = 1;

// 获取当前 CPU
struct cpu* mycpu(void) {
    int id = cpuid();
    return &cpus[id];
}

// 获取当前 PCB
struct proc* myproc(void) {
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  return p;
}

// 申请pid
int pid_alloc(){
  return nextpid++;
}

// 打印进程信息
static void print_proc_info(struct proc *p) {
    if(!p) return;
    uart_puts("Process ");
    uart_putc('0' + p->pid);
    uart_puts(": state=");
    switch(p->state) {
        case UNUSED: uart_puts("UNUSED"); break;
        case USED: uart_puts("USED"); break;
        case RUNNABLE: uart_puts("RUNNABLE"); break;
        case RUNNING: uart_puts("RUNNING"); break;
        case BLOCKED : uart_puts("BLOCKED"); break;
        case ZOMBIE: uart_puts("ZOMBIE"); break;
        default: uart_puts("UNKNOWN"); break;
    }
    uart_puts("\n");
}

// 初始化进程管理
void proc_init(void) {
    for(int i = 0; i < NPROC; i++) {
        proc[i].state = UNUSED;
    }
    // 初始化当前 CPU 表
    struct cpu *c = mycpu();
    c->proc = 0;
//    c->noff = 0;
//    c->intena = 0;
    // c->context 会在第一次 switch 时被保存
}

// 分配一个新的进程控制块
struct proc* proc_alloc(void) {
  	struct proc *p;
    for(int i = 0; i < NPROC; i++) {
      	p = &proc[i];
        if(p->state == UNUSED) {
            p->state = USED;
            p->pid = pid_alloc();
            return p;
        }
    }
    return 0;
}

// 释放进程控制块
void proc_free(struct proc *p) {
    p->pid = 0;
    p->state = UNUSED;
}

// 进程调度器
void scheduler(void) {
    struct cpu *c = mycpu();
    c->proc = 0; // 初始化当前 CPU 的进程为空

    struct proc *p;
    for(;;) {
        // 避免死锁，确保设备可以中断 (如果实现中断，取消注释)
        // intr_on();

        for(int i = 0; i < NPROC; i++) {
         	p = &proc[i];
        	if(p->state == RUNNABLE) {
            	p->state = RUNNING;
            	c->proc = p;
            	// 切换到下一进程，当该进程 yield 后，cpu会回到这里
            	switch_context(&c->context, &p->context);
            	// 进程返回时，c->proc 仍然指向刚刚运行的进程。
            	// 调度器将继续循环，从 ready_queue 中获取下一个进程。
            	// 此时将 c->proc 重置为 0，以便在下一次循环中正确设置。
            	c->proc = 0;
          	}
        }
    }
}

// 主动让出CPU
void yield(void) {
    struct proc *p = myproc();
    p->state = RUNNABLE;

    // 将当前进程的上下文保存到其 proc 结构中，
    // 然后切换到 CPU 的调度器上下文。
    // 这将使执行流返回到 scheduler() 函数中的 switch_context 调用点。
    switch_context(&p->context, &mycpu()->context);
}

// 汇编实现的上下文切换
extern void switch_context(struct context *old, struct context *new);