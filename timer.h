#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

uint32_t get_timer_raw(void);
uint32_t get_timer_raw_freq(void);

#define timer_conv_raw_to_usec(v) (v)

void delay_usec(uint32_t usec);
void delay_raw(uint32_t clk);

#endif