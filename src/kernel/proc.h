#include "types.h"
#ifndef _PROC_H
#define _PROC_H

// CPU数
#define NCPU 1

// 最大进程数
#define NPROC 16

// 进程上下文结构
struct context {
    uint64 sp;     // 栈指针
    uint64 x18;    // 平台寄存器
    uint64 x19;    // 被调用者保存寄存器
    uint64 x20;
    uint64 x21;
    uint64 x22;
    uint64 x23;
    uint64 x24;
    uint64 x25;
    uint64 x26;
    uint64 x27;
    uint64 x28;
    uint64 x29;    // 帧指针
    uint64 x30;    // 链接寄存器
};

// 进程状态枚举
enum procstate { UNUSED, USED, BLOCKED, RUNNABLE, RUNNING, ZOMBIE };

// 进程控制块结构
struct proc {
    uint64 state;        // 进程状态
    uint64 pid;          // 进程ID
    uint64 kstack;      // 内核栈指针
    struct context context; // 进程上下文
};

// CPU 结构体
struct cpu {
    struct proc *proc;          // 当前运行的进程
    struct context context;     // CPU 的上下文
//    int noff;                   // 中断嵌套深度
//    int intena;                 // 中断使能状态
};

// 函数声明
struct cpu* mycpu(void);
struct proc* myproc(void);
void proc_init(void);
struct proc* proc_alloc(void);
void proc_free(struct proc *p);
void scheduler(void);
void yield(void);

// 汇编实现的上下文切换
extern void switch_context(struct context *old, struct context *new);

#endif 