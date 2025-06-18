#include "types.h"
#ifndef _MM_H
#define _MM_H

#define NULL ((void *)0)

void memset(void *dest, char c, uint64 len);
void init_mm(void);
void* alloc_pages(uint32 number_of_pages);
void free_pages(void *addr, uint32 number_of_pages);

#endif
