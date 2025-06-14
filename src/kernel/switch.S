# 上下文切换
# void switch_context(struct context *old, struct context *new);
# x0 = old, x1 = new
.global switch_context
switch_context:
    # 保存栈指针
    mov x10, sp

    # 保存旧上下文
    stp x10, x18, [x0, #16 * 0]
    stp x19, x20, [x0, #16 * 1]
    stp x21, x22, [x0, #16 * 2]
    stp x23, x24, [x0, #16 * 3]
    stp x25, x26, [x0, #16 * 4]
    stp x27, x28, [x0, #16 * 5]
    stp x29, x30, [x0, #16 * 6]

    # 加载新上下文
    ldp x10, x18, [x1, #16 * 0]
    ldp x19, x20, [x1, #16 * 1]
    ldp x21, x22, [x1, #16 * 2]
    ldp x23, x24, [x1, #16 * 3]
    ldp x25, x26, [x1, #16 * 4]
    ldp x27, x28, [x1, #16 * 5]
    ldp x29, x30, [x1, #16 * 6]

    # 恢复栈指针
    mov sp, x10

    # 返回到新进程
    ret 