#include <stdint.h>

#include "keyboard.h"
#include "shell.h"

static uint8_t keyboard_shift;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static int keyboard_wait_input_clear(void) {
    for (uint32_t timeout = 100000; timeout != 0; timeout--) {
        if ((inb(0x64) & 0x02) == 0) {
            return 1;
        }
    }
    return 0;
}

static int keyboard_wait_output_ready(void) {
    for (uint32_t timeout = 100000; timeout != 0; timeout--) {
        if ((inb(0x64) & 0x01) != 0) {
            return 1;
        }
    }
    return 0;
}

int keyboard_initialize(void) {
    if (!keyboard_wait_input_clear()) {
        return 0;
    }
    outb(0x64, 0x20);
    if (!keyboard_wait_output_ready()) {
        return 0;
    }
    uint8_t configuration = inb(0x60);
    configuration |= 0x01;
    configuration &= (uint8_t)~0x10;
    if (!keyboard_wait_input_clear()) {
        return 0;
    }
    outb(0x64, 0x60);
    if (!keyboard_wait_input_clear()) {
        return 0;
    }
    outb(0x60, configuration);
    if (!keyboard_wait_input_clear()) {
        return 0;
    }
    outb(0x64, 0xae);
    return 1;
}

static char keyboard_character(uint8_t scancode) {
    static const char normal[] =
        "??1234567890-=\b\tqwertyuiop[]\n?asdfghjkl;'`?\\zxcvbnm,./?? ";
    static const char shifted[] =
        "\077\077!@#$%^&*()_+\b\tQWERTYUIOP{}\n"
        "\077ASDFGHJKL:\"~?|ZXCVBNM<>\077\077\077 ";
    if (scancode >= sizeof(normal) - 1) {
        return '\0';
    }
    return keyboard_shift ? shifted[scancode] : normal[scancode];
}

void keyboard_interrupt_handler(void) {
    uint8_t scancode = inb(0x60);
    outb(0x20, 0x20);
    if (scancode == 0x2a || scancode == 0x36) {
        keyboard_shift = 1;
        return;
    }
    if (scancode == 0xaa || scancode == 0xb6) {
        keyboard_shift = 0;
        return;
    }
    if ((scancode & 0x80) != 0) {
        return;
    }
    char character = keyboard_character(scancode);
    if (character != '\0') {
        shell_input_character(character);
    }
}
