#include "memlayout.h"
#include "mm.h"
#include "uart.h"

#define PAGE_SIZE 4096 // 页大小定义
#define TOTAL_PAGES (TOTAL_MEM / PAGE_SIZE) // 内存总页数
#define BITMAP_SIZE (TOTAL_PAGES / 8) // 管理页表的位图大小

static uint8 bitmap[BITMAP_SIZE];

static inline void bitmap_set(uint32 index) {
    bitmap[index / 8] |= (1 << (index % 8));
}

static inline void bitmap_clear(uint32 index) {
    bitmap[index / 8] &= ~(1 << (index % 8));
}

static inline uint32 bitmap_test(uint32 index) {
    return (bitmap[index / 8] >> (index % 8)) & 1;
}

// 内核结束位置
extern char end[];

void memset(void *dest, char c, uint64 len) {
    for (uint8 *d = dest; len > 0; d++, len--)
        *d = c;
}

void init_mm(void) {
    memset(bitmap, 0, BITMAP_SIZE);

    // 计算内核占用页数，向上取整
    uint64 kernel_end = (uint64)end;
    uint32 reserved_pages = (kernel_end - MEM_START + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32 i = 0; i < reserved_pages; i++) {
        bitmap_set(i);
    }
}