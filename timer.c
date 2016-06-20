#include "timer.h"
#include "arm.h"

#define TIMER_BASE ((unsigned char*)0x3f003000)

uint32_t
get_timer_raw(void)
{
    return read32(TIMER_BASE + 0x4);
}

uint32_t
get_timer_raw_freq(void)
{
    return 1000000;             /* 1MHz? */
}



