#include <stdint.h>

#include "limine.h"

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

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end[] = LIMINE_REQUESTS_END_MARKER;

static void halt_forever(void) {
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static void clear_screen(volatile uint16_t *video_memory) {
    for (uint16_t row = 0; row < 25; row++) {
        for (uint16_t column = 0; column < 80; column++) {
            video_memory[(row * 80) + column] = (uint16_t)' ' | 0x0700;
        }
    }
}

static void write_string(volatile uint16_t *video_memory,
                         uint16_t row,
                         uint16_t column,
                         const char *text,
                         uint8_t colour) {
    while (*text != '\0' && column < 80) {
        video_memory[(row * 80) + column] = (uint16_t)(uint8_t)*text | 
                                             ((uint16_t)colour << 8);
        text++;
        column++;
    }
}

void kmain(void) {
    if (limine_hhdm_request.response == 0) {
        halt_forever();
    }

    volatile uint16_t *video_memory =
        (volatile uint16_t *)(limine_hhdm_request.response->offset + 0xb8000);

    clear_screen(video_memory);
    write_string(video_memory, 8, 27, "Welcome to NovaOS", 0x0f);
    write_string(video_memory, 10, 21, "Limine x86-64 boot milestone 1", 0x07);
    halt_forever();
}