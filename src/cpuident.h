#pragma once

#if defined __GNUC__ && (defined __i386__ || defined __x86_64__)
#define HAS_X86_CPUID 1
#include <cpuid.h>
static inline void __x86_cpuidex(int reg[], int level, int count)
{ __cpuid_count(level, count, reg[0], reg[1], reg[2], reg[3]); }
#elif defined _MSC_VER && (defined _M_IX86 || defined _M_X64)
#define HAS_X86_CPUID 1
#define __x86_cpuidex __cpuidex
#endif
