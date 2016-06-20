#ifndef ARM_H
#define ARM_H

#include <stdint.h>

void enable_icache(void);
void disable_icache(void);

void enable_dcache(void);
void disable_dcache(void);

#define SYSREG_ACCESS(name)                     \
    static inline uint64_t                      \
    get_##name(void)                             \
    {                                            \
        uint64_t reg;                            \
        asm volatile ("mrs %0, " #name "\n"      \
                      :"=r"(reg));               \
        return reg;                              \
    }                                            \
    static inline void                               \
    set_##name(uint64_t reg)                         \
    {                                                \
        asm volatile ("mrs "#name", %0\n"        \
                      ::"r"(reg));               \
    }

SYSREG_ACCESS(currentel)
SYSREG_ACCESS(cntfrq_el0)
SYSREG_ACCESS(cntpct_el0)

static inline uint32_t read32(unsigned char *p) {
    return *(volatile uint32_t*)p;
}

static inline void write32(unsigned char *p, uint32_t val) {
    *(volatile uint32_t*)p = val;
}

#endif