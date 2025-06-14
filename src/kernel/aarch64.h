#include "types.h"
#ifndef __AARCH64_H
#define __AARCH64_H

// 获取所在核心
static inline uint64 cpuid()
{
  uint64 x;
  asm volatile("mrs %0, mpidr_el1" : "=r" (x) );
  return x & 0xff;
}

#endif
