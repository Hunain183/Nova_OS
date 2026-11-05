#include <stdint.h>

#include "keyboard.h"
#include "limine.h"
#include "shell.h"
#include "terminal.h"

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

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void halt_forever(void) {
    for (;;) {
        __asm__ volatile ("hlt");
    }
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
    outb(0x3f9, 0x00);
    outb(0x3fb, 0x80);
    outb(0x3f8, 0x03);
    outb(0x3f9, 0x00);
    outb(0x3fb, 0x03);
    outb(0x3fa, 0xc7);
    outb(0x3fc, 0x0b);
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

extern void keyboard_interrupt_stub(void);

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

static int interrupts_init(void) {
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
    outb(0x21, 0xff);
    outb(0xa1, 0xff);

    if (!keyboard_initialize()) {
        serial_write("NovaOS: keyboard unavailable\r\n");
        return 0;
    }
    outb(0x21, inb(0x21) & (uint8_t)~0x02);
    __asm__ volatile ("sti");
    return 1;
}

void kmain(void) {
    serial_init();

    volatile uint16_t *vga_memory = 0;
    struct limine_framebuffer *framebuffer = 0;
    if (limine_framebuffer_request.response != 0 &&
        limine_framebuffer_request.response->framebuffer_count != 0) {
        struct limine_framebuffer *candidate =
            limine_framebuffer_request.response->framebuffers[0];
        if (candidate->memory_model == LIMINE_FRAMEBUFFER_RGB &&
            candidate->bpp == 32 && candidate->width >= 330 &&
            candidate->height >= 60) {
            framebuffer = candidate;
        }
    }

    if (framebuffer == 0) {
        if (limine_hhdm_request.response == 0) {
            serial_write("NovaOS: no framebuffer or HHDM\r\n");
            halt_forever();
        }
        vga_memory = (volatile uint16_t *)
            (limine_hhdm_request.response->offset + 0xb8000);
    }

    terminal_init(framebuffer, vga_memory);
    shell_init();
    if (interrupts_init()) {
        serial_write("NovaOS: interrupts online\r\n");
    }
    halt_forever();
}
