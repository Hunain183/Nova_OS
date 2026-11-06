#include <stdint.h>

#include "interrupts.h"
#include "keyboard.h"
#include "limine.h"
#include "pmm.h"
#include "shell.h"
#include "scheduler.h"
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

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request limine_memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
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

void kmain(void) {
    serial_init();

    volatile uint16_t *vga_memory = 0;
    struct limine_framebuffer *framebuffer = 0;
    if (limine_hhdm_request.response == 0 ||
        limine_memmap_request.response == 0 ||
        !pmm_init(limine_memmap_request.response,
                  limine_hhdm_request.response->offset)) {
        serial_write("NovaOS: physical memory initialization failed\r\n");
        halt_forever();
    }
    if (!scheduler_initialize(limine_hhdm_request.response->offset)) {
        serial_write("NovaOS: scheduler initialization failed\r\n");
        halt_forever();
    }
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
