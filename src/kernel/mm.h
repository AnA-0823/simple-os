#include "types.h"
#ifndef _MM_H
#define _MM_H

#define NULL ((void *)0)

void memset(void *dest, char c, uint64 len);
void *memcpy(void *dest, const void *src, uint32 n);
int memcmp(const void *s1, const void *s2, uint32 n);
void init_mm(void);
void* alloc_pages(uint32 number_of_pages);
void free_pages(void *addr, uint32 number_of_pages);

#endif
