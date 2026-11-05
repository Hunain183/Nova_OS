#ifndef NOVAOS_TIMER_H
#define NOVAOS_TIMER_H

#include <stdint.h>

#define TIMER_FREQUENCY_HZ 100

void timer_initialize(void);
void timer_interrupt(void);
uint64_t timer_ticks(void);
uint64_t timer_uptime_seconds(void);

#endif