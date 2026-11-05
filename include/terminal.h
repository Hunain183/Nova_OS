#ifndef NOVAOS_TERMINAL_H
#define NOVAOS_TERMINAL_H

#include "limine.h"

void terminal_init(struct limine_framebuffer *framebuffer,
                   volatile uint16_t *vga_memory);
void terminal_clear(void);
void terminal_putchar(char character);
void terminal_write(const char *text);

#endif
