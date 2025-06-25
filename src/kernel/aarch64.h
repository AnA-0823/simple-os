#ifndef _AARCH64_H
#define _AARCH64_H

#include "types.h"

// 获取所在核心
static inline uint64 cpuid()
{
  uint64 x;
  asm volatile("mrs %0, mpidr_el1" : "=r" (x) );
  return x & 0xff;
}

#endif
