#include <stdint.h>

#include "limine.h"

extern void keyboard_interrupt_stub(void);

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request limine_hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request limine_framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
    .response = 0,
};

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end[] = LIMINE_REQUESTS_END_MARKER;

static void halt_forever(void) {
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void keyboard_init(void) {
    uint8_t status;
    do {
        status = inb(0x64);
    } while ((status & 0x02) != 0);
    outb(0x64, 0x20);
    do {
        status = inb(0x64);
    } while ((status & 0x01) == 0);
    uint8_t configuration = inb(0x60);
    configuration |= 0x01;
    configuration &= (uint8_t)~0x10;
    do {
        status = inb(0x64);
    } while ((status & 0x02) != 0);
    outb(0x64, 0x60);
    do {
        status = inb(0x64);
    } while ((status & 0x02) != 0);
    outb(0x60, configuration);
    do {
        status = inb(0x64);
    } while ((status & 0x02) != 0);
    outb(0x64, 0xae);
}

static void serial_write(const char *text) {
    while (*text != '\0') {
        uint32_t timeout = 100000;
        while ((inb(0x3fd) & 0x20) == 0 && timeout != 0) {
            timeout--;
        }
        if (timeout == 0) {
            return;
        }
        outb(0x3f8, (uint8_t)*text++);
    }
}

static void serial_init(void) {
    outb(0x3f8 + 1, 0x00);
    outb(0x3f8 + 3, 0x80);
    outb(0x3f8 + 0, 0x03);
    outb(0x3f8 + 1, 0x00);
    outb(0x3f8 + 3, 0x03);
    outb(0x3f8 + 2, 0xc7);
    outb(0x3f8 + 4, 0x0b);
    serial_write("NovaOS: serial online\r\n");
}

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];

static void idt_set_gate(uint8_t vector, uint16_t selector,
                         void (*handler)(void)) {
    uint64_t address = (uint64_t)handler;
    idt[vector].offset_low = (uint16_t)address;
    idt[vector].selector = selector;
    idt[vector].ist = 0;
    idt[vector].attributes = 0x8e;
    idt[vector].offset_mid = (uint16_t)(address >> 16);
    idt[vector].offset_high = (uint32_t)(address >> 32);
    idt[vector].reserved = 0;
}

static void interrupts_init(void) {
    for (uint16_t index = 0; index < 256; index++) {
        idt[index] = (struct idt_entry){0};
    }
    uint16_t code_segment;
    __asm__ volatile ("mov %%cs, %0" : "=r"(code_segment));
    idt_set_gate(33, code_segment, keyboard_interrupt_stub);
    struct idt_pointer pointer = {
        .limit = sizeof(idt) - 1,
        .base = (uint64_t)idt,
    };
    __asm__ volatile ("lidt %0" : : "m"(pointer));

    outb(0x20, 0x11);
    outb(0xa0, 0x11);
    outb(0x21, 0x20);
    outb(0xa1, 0x28);
    outb(0x21, 0x04);
    outb(0xa1, 0x02);
    outb(0x21, 0x01);
    outb(0xa1, 0x01);
    outb(0x21, 0xfd);
    outb(0xa1, 0xff);
    outb(0x21, inb(0x21) & (uint8_t)~0x02);
    keyboard_init();
    __asm__ volatile ("sti");
    serial_write("NovaOS: interrupts online\r\n");
}

static void clear_screen(volatile uint16_t *video_memory) {
    for (uint16_t row = 0; row < 25; row++) {
        for (uint16_t column = 0; column < 80; column++) {
            video_memory[(row * 80) + column] = (uint16_t)' ' | 0x0700;
        }
    }
}

static const uint8_t *glyph_for(char character) {
    static const uint8_t glyphs[][7] = {
        [0] = {0x1e, 0x05, 0x1f, 0x14, 0x1f, 0x00, 0x00},
        [1] = {0x1e, 0x05, 0x1e, 0x05, 0x1e, 0x00, 0x00},
        [2] = {0x1e, 0x01, 0x01, 0x01, 0x1e, 0x00, 0x00},
        [3] = {0x1e, 0x05, 0x05, 0x05, 0x1e, 0x00, 0x00},
        [4] = {0x1f, 0x01, 0x0f, 0x01, 0x1f, 0x00, 0x00},
        [5] = {0x1f, 0x01, 0x0f, 0x01, 0x01, 0x00, 0x00},
        [6] = {0x1e, 0x01, 0x1d, 0x11, 0x1f, 0x00, 0x00},
        [7] = {0x11, 0x11, 0x1f, 0x11, 0x11, 0x00, 0x00},
        [8] = {0x1f, 0x04, 0x04, 0x04, 0x1f, 0x00, 0x00},
        [9] = {0x1f, 0x02, 0x02, 0x12, 0x0c, 0x00, 0x00},
        [10] = {0x11, 0x0b, 0x05, 0x0b, 0x11, 0x00, 0x00},
        [11] = {0x01, 0x01, 0x01, 0x01, 0x1f, 0x00, 0x00},
        [12] = {0x11, 0x1b, 0x15, 0x11, 0x11, 0x00, 0x00},
        [13] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x00, 0x00},
        [14] = {0x0e, 0x11, 0x11, 0x11, 0x0e, 0x00, 0x00},
        [15] = {0x1e, 0x11, 0x1e, 0x01, 0x01, 0x00, 0x00},
        [16] = {0x0e, 0x11, 0x15, 0x09, 0x16, 0x00, 0x00},
        [17] = {0x1e, 0x11, 0x1e, 0x05, 0x11, 0x00, 0x00},
        [18] = {0x1f, 0x01, 0x1f, 0x10, 0x1f, 0x00, 0x00},
        [19] = {0x1f, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00},
        [20] = {0x11, 0x11, 0x11, 0x11, 0x0e, 0x00, 0x00},
        [21] = {0x11, 0x11, 0x11, 0x0a, 0x04, 0x00, 0x00},
        [22] = {0x11, 0x11, 0x15, 0x1b, 0x11, 0x00, 0x00},
        [23] = {0x11, 0x0a, 0x04, 0x0a, 0x11, 0x00, 0x00},
        [24] = {0x11, 0x0a, 0x04, 0x04, 0x04, 0x00, 0x00},
        [25] = {0x1f, 0x08, 0x04, 0x02, 0x1f, 0x00, 0x00},
        [26] = {0x0e, 0x11, 0x13, 0x15, 0x19, 0x0e, 0x00},
        [27] = {0x0c, 0x12, 0x02, 0x04, 0x1f, 0x00, 0x00},
        [28] = {0x1e, 0x01, 0x06, 0x08, 0x1f, 0x00, 0x00},
        [29] = {0x1e, 0x01, 0x0e, 0x01, 0x1e, 0x00, 0x00},
        [30] = {0x02, 0x06, 0x0a, 0x1f, 0x02, 0x00, 0x00},
        [31] = {0x1f, 0x10, 0x1e, 0x01, 0x1e, 0x00, 0x00},
        [32] = {0x0e, 0x10, 0x1e, 0x11, 0x0e, 0x00, 0x00},
        [33] = {0x1f, 0x01, 0x02, 0x04, 0x04, 0x00, 0x00},
        [34] = {0x0e, 0x11, 0x0e, 0x11, 0x0e, 0x00, 0x00},
        [35] = {0x0e, 0x11, 0x0f, 0x01, 0x0e, 0x00, 0x00},
        [36] = {0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00},
        [37] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    };
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789- .";

    if (character >= 'a' && character <= 'z') {
        character = (char)(character - 'a' + 'A');
    }
    for (uint8_t index = 0; index < sizeof(alphabet) - 1; index++) {
        if (alphabet[index] == character) {
            return glyphs[index];
        }
    }
    return glyphs[37];
}

static void framebuffer_put_pixel(const struct limine_framebuffer *framebuffer,
                                  uint64_t x,
                                  uint64_t y,
                                  uint32_t colour) {
    volatile uint32_t *pixel = (volatile uint32_t *)
        ((uint8_t *)framebuffer->address + (y * framebuffer->pitch) + (x * 4));
    uint32_t red = ((colour >> 16) & 0xff) >> (8 - framebuffer->red_mask_size);
    uint32_t green = ((colour >> 8) & 0xff) >> (8 - framebuffer->green_mask_size);
    uint32_t blue = (colour & 0xff) >> (8 - framebuffer->blue_mask_size);
    *pixel = (red << framebuffer->red_mask_shift) |
             (green << framebuffer->green_mask_shift) |
             (blue << framebuffer->blue_mask_shift);
}

static void framebuffer_write(const struct limine_framebuffer *framebuffer,
                              const char *text,
                              uint64_t x,
                              uint64_t y) {
    for (; *text != '\0'; text++, x += 18) {
        const uint8_t *glyph = glyph_for(*text);
        for (uint64_t row = 0; row < 7; row++) {
            for (uint64_t column = 0; column < 5; column++) {
                if ((glyph[row] & (1u << (4 - column))) == 0) {
                    continue;
                }
                for (uint64_t scale_y = 0; scale_y < 3; scale_y++) {
                    for (uint64_t scale_x = 0; scale_x < 3; scale_x++) {
                        framebuffer_put_pixel(framebuffer,
                                              x + (column * 3) + scale_x,
                                              y + (row * 3) + scale_y,
                                              0xffffff);
                    }
                }
            }
        }
    }
}

static void framebuffer_clear(const struct limine_framebuffer *framebuffer,
                              uint32_t colour) {
    for (uint64_t y = 0; y < framebuffer->height; y++) {
        for (uint64_t x = 0; x < framebuffer->width; x++) {
            framebuffer_put_pixel(framebuffer, x, y, colour);
        }
    }
}

static volatile uint16_t *vga_memory;
static const struct limine_framebuffer *terminal_framebuffer;
static uint64_t terminal_x;
static uint64_t terminal_y;
static char command[80];
static uint8_t command_length;

static void terminal_putchar(char character) {
    if (terminal_framebuffer != 0) {
        if (character == '\n') {
            terminal_x = 24;
            terminal_y += 24;
        } else if (character == '\b') {
            if (terminal_x > 24) {
                terminal_x -= 18;
                framebuffer_write(terminal_framebuffer, " ", terminal_x, terminal_y);
            }
        } else {
            char text[2] = {character, '\0'};
            framebuffer_write(terminal_framebuffer, text, terminal_x, terminal_y);
            terminal_x += 18;
            if (terminal_x + 18 >= terminal_framebuffer->width) {
                terminal_x = 24;
                terminal_y += 24;
            }
        }
        return;
    }
    if (character == '\n') {
        terminal_x = 0;
        terminal_y++;
    } else if (character == '\b') {
        if (terminal_x > 0) {
            terminal_x--;
            vga_memory[(terminal_y * 80) + terminal_x] = (uint16_t)' ' | 0x0700;
        }
    } else if (terminal_x < 80) {
        vga_memory[(terminal_y * 80) + terminal_x] = (uint16_t)(uint8_t)character | 0x0700;
        terminal_x++;
    }
}

static void terminal_write(const char *text) {
    while (*text != '\0') {
        terminal_putchar(*text++);
    }
}

static void terminal_prompt(void) {
    terminal_write("\n> ");
    command_length = 0;
}

static void terminal_command(void) {
    command[command_length] = '\0';
    serial_write("NovaOS command: ");
    serial_write(command);
    serial_write("\r\n");
    if (command_length == 0) {
        terminal_prompt();
    } else if (command[0] == 'h' && command[1] == 'e' && command[2] == 'l' &&
               command[3] == 'p' && command[4] == '\0') {
        terminal_write("\ncommands: help, clear, echo, info, halt");
        terminal_prompt();
    } else if (command[0] == 'c' && command[1] == 'l' && command[2] == 'e' &&
               command[3] == 'a' && command[4] == 'r' && command[5] == '\0') {
        if (terminal_framebuffer != 0) {
            framebuffer_clear(terminal_framebuffer, 0x102030);
        } else {
            clear_screen(vga_memory);
        }
        terminal_x = terminal_framebuffer != 0 ? 24 : 0;
        terminal_y = 0;
        terminal_prompt();
    } else if (command[0] == 'e' && command[1] == 'c' && command[2] == 'h' &&
               command[3] == 'o' && command[4] == ' ' && command_length > 5) {
        terminal_write("\n");
        terminal_write(&command[5]);
        terminal_prompt();
    } else if (command[0] == 'i' && command[1] == 'n' && command[2] == 'f' &&
               command[3] == 'o' && command[4] == '\0') {
        terminal_write("\nNovaOS x86-64 | interrupts: on | keyboard: PS/2");
        terminal_prompt();
    } else if (command[0] == 'h' && command[1] == 'a' && command[2] == 'l' &&
               command[3] == 't' && command[4] == '\0') {
        terminal_write("\nhalted");
        halt_forever();
    } else {
        terminal_write("\nunknown command");
        terminal_prompt();
    }
}

static const char keyboard_map[] =
    "\0\0" "1234567890-=" "\b\t" "qwertyuiop[]" "\n\0" "asdfghjkl;'`"
    "\0\\zxcvbnm,./\0*\0 \0";

void keyboard_interrupt_handler(void) {
    uint8_t scancode = inb(0x60);
    outb(0x20, 0x20);
    if ((scancode & 0x80) != 0 || scancode >= sizeof(keyboard_map) - 1) {
        return;
    }
    char character = keyboard_map[scancode];
    if (character == '\0') {
        return;
    }
    if (character == '\n') {
        terminal_command();
    } else if (character == '\b') {
        if (command_length > 0) {
            command_length--;
            terminal_putchar('\b');
        }
    } else if (command_length < sizeof(command) - 1) {
        command[command_length++] = character;
        terminal_putchar(character);
    }
}

void kmain(void) {
    serial_init();
    if (limine_framebuffer_request.response != 0 &&
        limine_framebuffer_request.response->framebuffer_count != 0) {
        struct limine_framebuffer *framebuffer =
            limine_framebuffer_request.response->framebuffers[0];
        if (framebuffer->memory_model == LIMINE_FRAMEBUFFER_RGB &&
            framebuffer->bpp == 32 && framebuffer->width >= 330 &&
            framebuffer->height >= 60) {
            terminal_framebuffer = framebuffer;
            framebuffer_clear(framebuffer, 0x102030);
            terminal_write("WELCOME TO NOVAOS\n");
            terminal_write("LIMINE X86-64 BOOT");
            terminal_prompt();
            interrupts_init();
            halt_forever();
        }
    }

    if (limine_hhdm_request.response == 0) {
        halt_forever();
    }

    vga_memory =
        (volatile uint16_t *)(limine_hhdm_request.response->offset + 0xb8000);

    clear_screen(vga_memory);
    terminal_write("Welcome to NovaOS\n");
    terminal_write("Limine x86-64 interactive shell");
    terminal_prompt();
    interrupts_init();
    halt_forever();
}