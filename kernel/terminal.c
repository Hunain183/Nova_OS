#include <stdint.h>

#include "terminal.h"

#define VGA_COLUMNS 80
#define VGA_ROWS 25
#define FRAMEBUFFER_BACKGROUND 0x102030
#define FRAMEBUFFER_CHARACTER_WIDTH 15
#define FRAMEBUFFER_CHARACTER_HEIGHT 21
#define FRAMEBUFFER_CHARACTER_ADVANCE 18
#define FRAMEBUFFER_LINE_ADVANCE 24
#define FRAMEBUFFER_LEFT_MARGIN 24

static const struct limine_framebuffer *terminal_framebuffer;
static volatile uint16_t *terminal_vga_memory;
static uint64_t terminal_x;
static uint64_t terminal_y;

static const uint8_t *glyph_for(char character) {
    static const uint8_t glyphs[][7] = {
        [0] = {0x0e, 0x11, 0x1f, 0x11, 0x11, 0x00, 0x00},
        [1] = {0x1e, 0x11, 0x1e, 0x11, 0x1e, 0x00, 0x00},
        [2] = {0x0f, 0x10, 0x10, 0x10, 0x0f, 0x00, 0x00},
        [3] = {0x1e, 0x11, 0x11, 0x11, 0x1e, 0x00, 0x00},
        [4] = {0x1f, 0x10, 0x1e, 0x10, 0x1f, 0x00, 0x00},
        [5] = {0x1f, 0x10, 0x1e, 0x10, 0x10, 0x00, 0x00},
        [6] = {0x0f, 0x10, 0x17, 0x11, 0x0f, 0x00, 0x00},
        [7] = {0x11, 0x11, 0x1f, 0x11, 0x11, 0x00, 0x00},
        [8] = {0x1f, 0x04, 0x04, 0x04, 0x1f, 0x00, 0x00},
        [9] = {0x01, 0x01, 0x01, 0x11, 0x0e, 0x00, 0x00},
        [10] = {0x11, 0x12, 0x1c, 0x12, 0x11, 0x00, 0x00},
        [11] = {0x01, 0x01, 0x01, 0x01, 0x1f, 0x00, 0x00},
        [12] = {0x11, 0x1b, 0x15, 0x11, 0x11, 0x00, 0x00},
        [13] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x00, 0x00},
        [14] = {0x0e, 0x11, 0x11, 0x11, 0x0e, 0x00, 0x00},
        [15] = {0x1e, 0x11, 0x1e, 0x01, 0x01, 0x00, 0x00},
        [16] = {0x0e, 0x11, 0x15, 0x12, 0x0d, 0x00, 0x00},
        [17] = {0x1e, 0x11, 0x1e, 0x12, 0x11, 0x00, 0x00},
        [18] = {0x0f, 0x10, 0x0e, 0x01, 0x1e, 0x00, 0x00},
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
        [38] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06},
        [39] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00},
        [40] = {0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00},
    };
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789- .>:";

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
                                  uint64_t x, uint64_t y, uint32_t colour) {
    volatile uint32_t *pixel = (volatile uint32_t *)
        ((uint8_t *)framebuffer->address + (y * framebuffer->pitch) + (x * 4));
    uint32_t red = ((colour >> 16) & 0xff) >>
                   (8 - framebuffer->red_mask_size);
    uint32_t green = ((colour >> 8) & 0xff) >>
                     (8 - framebuffer->green_mask_size);
    uint32_t blue = (colour & 0xff) >> (8 - framebuffer->blue_mask_size);
    *pixel = (red << framebuffer->red_mask_shift) |
             (green << framebuffer->green_mask_shift) |
             (blue << framebuffer->blue_mask_shift);
}

static void framebuffer_fill(const struct limine_framebuffer *framebuffer,
                             uint64_t x, uint64_t y, uint64_t width,
                             uint64_t height, uint32_t colour) {
    for (uint64_t row = y; row < y + height; row++) {
        for (uint64_t column = x; column < x + width; column++) {
            framebuffer_put_pixel(framebuffer, column, row, colour);
        }
    }
}

static void framebuffer_write_glyph(
    const struct limine_framebuffer *framebuffer, char character,
    uint64_t x, uint64_t y) {
    const uint8_t *glyph = glyph_for(character);
    for (uint64_t row = 0; row < 7; row++) {
        for (uint64_t column = 0; column < 5; column++) {
            if ((glyph[row] & (1u << (4 - column))) == 0) {
                continue;
            }
            framebuffer_fill(framebuffer, x + column * 3, y + row * 3, 3, 3,
                             0xffffff);
        }
    }
}

static void framebuffer_scroll(void) {
    uint64_t line_height = FRAMEBUFFER_LINE_ADVANCE;
    if (terminal_framebuffer->height <= line_height) {
        terminal_y = 0;
        return;
    }
    for (uint64_t row = 0; row + line_height < terminal_framebuffer->height;
         row++) {
        volatile uint8_t *destination =
            (volatile uint8_t *)terminal_framebuffer->address +
            row * terminal_framebuffer->pitch;
        volatile uint8_t *source =
            destination + line_height * terminal_framebuffer->pitch;
        for (uint64_t byte = 0; byte < terminal_framebuffer->pitch; byte++) {
            destination[byte] = source[byte];
        }
    }
    framebuffer_fill(terminal_framebuffer, 0,
                     terminal_framebuffer->height - line_height,
                     terminal_framebuffer->width, line_height,
                     FRAMEBUFFER_BACKGROUND);
    terminal_y = terminal_framebuffer->height - line_height;
}

static void vga_scroll(void) {
    for (uint16_t row = 1; row < VGA_ROWS; row++) {
        for (uint16_t column = 0; column < VGA_COLUMNS; column++) {
            terminal_vga_memory[(row - 1) * VGA_COLUMNS + column] =
                terminal_vga_memory[row * VGA_COLUMNS + column];
        }
    }
    for (uint16_t column = 0; column < VGA_COLUMNS; column++) {
        terminal_vga_memory[(VGA_ROWS - 1) * VGA_COLUMNS + column] =
            (uint16_t)' ' | 0x0700;
    }
    terminal_y = VGA_ROWS - 1;
}

static void terminal_newline(void) {
    terminal_x = terminal_framebuffer != 0 ? FRAMEBUFFER_LEFT_MARGIN : 0;
    terminal_y += terminal_framebuffer != 0 ? FRAMEBUFFER_LINE_ADVANCE : 1;
    if (terminal_framebuffer != 0) {
        if (terminal_y + FRAMEBUFFER_CHARACTER_HEIGHT >
            terminal_framebuffer->height) {
            framebuffer_scroll();
        }
    } else if (terminal_y >= VGA_ROWS) {
        vga_scroll();
    }
}

void terminal_init(struct limine_framebuffer *framebuffer,
                   volatile uint16_t *vga_memory) {
    terminal_framebuffer = framebuffer;
    terminal_vga_memory = vga_memory;
    terminal_x = framebuffer != 0 ? FRAMEBUFFER_LEFT_MARGIN : 0;
    terminal_y = 0;
    terminal_clear();
}

void terminal_clear(void) {
    if (terminal_framebuffer != 0) {
        framebuffer_fill(terminal_framebuffer, 0, 0,
                         terminal_framebuffer->width,
                         terminal_framebuffer->height,
                         FRAMEBUFFER_BACKGROUND);
        terminal_x = FRAMEBUFFER_LEFT_MARGIN;
        terminal_y = 0;
        return;
    }
    for (uint16_t row = 0; row < VGA_ROWS; row++) {
        for (uint16_t column = 0; column < VGA_COLUMNS; column++) {
            terminal_vga_memory[row * VGA_COLUMNS + column] =
                (uint16_t)' ' | 0x0700;
        }
    }
    terminal_x = 0;
    terminal_y = 0;
}

void terminal_putchar(char character) {
    if (character == '\r') {
        return;
    }
    if (character == '\n') {
        terminal_newline();
        return;
    }
    if (character == '\b') {
        if (terminal_framebuffer != 0) {
            if (terminal_x > FRAMEBUFFER_LEFT_MARGIN) {
                terminal_x -= FRAMEBUFFER_CHARACTER_ADVANCE;
                framebuffer_fill(terminal_framebuffer, terminal_x, terminal_y,
                                 FRAMEBUFFER_CHARACTER_ADVANCE,
                                 FRAMEBUFFER_CHARACTER_HEIGHT,
                                 FRAMEBUFFER_BACKGROUND);
            }
        } else if (terminal_x > 0) {
            terminal_x--;
            terminal_vga_memory[terminal_y * VGA_COLUMNS + terminal_x] =
                (uint16_t)' ' | 0x0700;
        }
        return;
    }
    if (terminal_framebuffer != 0) {
        if (terminal_x + FRAMEBUFFER_CHARACTER_WIDTH >=
            terminal_framebuffer->width) {
            terminal_newline();
        }
        framebuffer_write_glyph(terminal_framebuffer, character, terminal_x,
                                terminal_y);
        terminal_x += FRAMEBUFFER_CHARACTER_ADVANCE;
    } else if (terminal_x < VGA_COLUMNS) {
        terminal_vga_memory[terminal_y * VGA_COLUMNS + terminal_x] =
            (uint16_t)(uint8_t)character | 0x0700;
        terminal_x++;
    }
}

void terminal_write(const char *text) {
    while (*text != '\0') {
        terminal_putchar(*text++);
    }
}
