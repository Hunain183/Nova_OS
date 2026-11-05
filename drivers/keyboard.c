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
    static const char normal[128] = {
        [0x01] = 0x1b, [0x02] = '1', [0x03] = '2', [0x04] = '3',
        [0x05] = '4', [0x06] = '5', [0x07] = '6', [0x08] = '7',
        [0x09] = '8', [0x0a] = '9', [0x0b] = '0', [0x0c] = '-',
        [0x0d] = '=', [0x0e] = '\b', [0x0f] = '\t',
        [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
        [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
        [0x18] = 'o', [0x19] = 'p', [0x1a] = '[', [0x1b] = ']',
        [0x1c] = '\n', [0x1e] = 'a', [0x1f] = 's', [0x20] = 'd',
        [0x21] = 'f', [0x22] = 'g', [0x23] = 'h', [0x24] = 'j',
        [0x25] = 'k', [0x26] = 'l', [0x27] = ';', [0x28] = '\'',
        [0x29] = '`', [0x2b] = '\\', [0x2c] = 'z', [0x2d] = 'x',
        [0x2e] = 'c', [0x2f] = 'v', [0x30] = 'b', [0x31] = 'n',
        [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/',
        [0x39] = ' ',
    };
    static const char shifted[128] = {
        [0x01] = 0x1b, [0x02] = '!', [0x03] = '@', [0x04] = '#',
        [0x05] = '$', [0x06] = '%', [0x07] = '^', [0x08] = '&',
        [0x09] = '*', [0x0a] = '(', [0x0b] = ')', [0x0c] = '_',
        [0x0d] = '+', [0x0e] = '\b', [0x0f] = '\t',
        [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R',
        [0x14] = 'T', [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I',
        [0x18] = 'O', [0x19] = 'P', [0x1a] = '{', [0x1b] = '}',
        [0x1c] = '\n', [0x1e] = 'A', [0x1f] = 'S', [0x20] = 'D',
        [0x21] = 'F', [0x22] = 'G', [0x23] = 'H', [0x24] = 'J',
        [0x25] = 'K', [0x26] = 'L', [0x27] = ':', [0x28] = '"',
        [0x29] = '~', [0x2b] = '|', [0x2c] = 'Z', [0x2d] = 'X',
        [0x2e] = 'C', [0x2f] = 'V', [0x30] = 'B', [0x31] = 'N',
        [0x32] = 'M', [0x33] = '<', [0x34] = '>', [0x35] = '?',
        [0x39] = ' ',
    };
    if (scancode >= 128) {
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
