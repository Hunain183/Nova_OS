#include <stdint.h>

#include "timer.h"

#define PIT_CHANNEL_0 0x40
#define PIT_COMMAND 0x43
#define PIT_BASE_FREQUENCY 1193182u

static volatile uint64_t system_ticks;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void timer_initialize(void) {
    uint16_t divisor = (uint16_t)(PIT_BASE_FREQUENCY / TIMER_FREQUENCY_HZ);
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL_0, (uint8_t)(divisor & 0xff));
    outb(PIT_CHANNEL_0, (uint8_t)(divisor >> 8));
}

void timer_interrupt(void) {
    system_ticks++;
}

uint64_t timer_ticks(void) {
    return system_ticks;
}

uint64_t timer_uptime_seconds(void) {
    return system_ticks / TIMER_FREQUENCY_HZ;
}