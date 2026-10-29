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

void kmain(void) {
    if (limine_framebuffer_request.response != 0 &&
        limine_framebuffer_request.response->framebuffer_count != 0) {
        struct limine_framebuffer *framebuffer =
            limine_framebuffer_request.response->framebuffers[0];
        if (framebuffer->memory_model == LIMINE_FRAMEBUFFER_RGB &&
            framebuffer->bpp == 32 && framebuffer->width >= 330 &&
            framebuffer->height >= 60) {
            framebuffer_clear(framebuffer, 0x102030);
            framebuffer_write(framebuffer, "WELCOME TO NOVAOS", 24, 24);
            framebuffer_write(framebuffer, "LIMINE X86-64 BOOT", 24, 54);
            halt_forever();
        }
    }

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