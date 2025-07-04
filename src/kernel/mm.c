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

void *memcpy(void *dest, const void *src, uint32 n) {
    uint8 *d = (uint8*)dest;
    const uint8 *s = (const uint8*)src;
    for (uint32 i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

int memcmp(const void *s1, const void *s2, uint32 n) {
    const uint8 *a = (const uint8*)s1;
    const uint8 *b = (const uint8*)s2;
    for (uint32 i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
    }
    return 0;
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

// 申请连续页
void* alloc_pages(uint32 number_of_pages) {
    if (number_of_pages == 0 || number_of_pages > TOTAL_PAGES) return NULL;

    for (uint32 i = 0; i <= TOTAL_PAGES - number_of_pages; i++) {
        uint32 found = 1;
        for (uint32 j = 0; j < number_of_pages; j++) {
            if (bitmap_test(i + j)) {
                found = 0;
                i += j;
                break;
            }
        }
        if (found) {
            for (uint32 j = 0; j < number_of_pages; j++) {
                bitmap_set(i + j);
            }
            return (void *)(MEM_START + i * PAGE_SIZE);
        }
    }
    return NULL; // 分配失败
}

// 释放连续页
void free_pages(void *addr, uint32 number_of_pages) {
    uint64 page_addr = (uint64)addr;
    if (page_addr < MEM_START || page_addr >= MEM_END || (page_addr - MEM_START) % PAGE_SIZE != 0) {
        return; // 非法地址
    }
    uint32 index = (page_addr - MEM_START) / PAGE_SIZE;
    if (index + number_of_pages > TOTAL_PAGES) {
        return; // 越界
    }
    for (uint32 i = 0; i < number_of_pages; i++) {
        bitmap_clear(index + i);
    }
}