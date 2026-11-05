#include <stdint.h>

#include "shell.h"
#include "terminal.h"

#define SHELL_INPUT_CAPACITY 128
#define NOVAOS_VERSION "0.2.0"

static char input[SHELL_INPUT_CAPACITY];
static uint8_t input_length;

static int text_equals(const char *left, const char *right) {
    while (*left != '\0' && *right != '\0' && *left == *right) {
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

static int starts_with(const char *text, const char *prefix) {
    while (*prefix != '\0') {
        if (*text++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static void prompt(void) {
    terminal_write("NovaOS> ");
    input_length = 0;
}

static void reboot(void) {
    terminal_write("Rebooting...\n");
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xfe),
                      "Nd"((uint16_t)0x64));
    for (uint32_t timeout = 1000000; timeout != 0; timeout--) {
        __asm__ volatile ("pause");
    }
    terminal_write("Reboot request was not accepted.\n");
    prompt();
}

static void shutdown(void) {
    __asm__ volatile ("outw %0, %1" : : "a"((uint16_t)0x2000),
                      "Nd"((uint16_t)0x604));
    __asm__ volatile ("outw %0, %1" : : "a"((uint16_t)0x2000),
                      "Nd"((uint16_t)0xb004));
    terminal_write("Shutdown is unavailable on this machine.\n");
    prompt();
}

static void execute_command(void) {
    input[input_length] = '\0';
    terminal_write("\n");
    if (input_length == 0) {
        prompt();
    } else if (text_equals(input, "help")) {
        terminal_write("help clear about version echo reboot shutdown\n");
        prompt();
    } else if (text_equals(input, "clear")) {
        terminal_clear();
        prompt();
    } else if (text_equals(input, "about")) {
        terminal_write("NovaOS is an educational x86-64 operating system.\n");
        prompt();
    } else if (text_equals(input, "version")) {
        terminal_write("NovaOS version ");
        terminal_write(NOVAOS_VERSION);
        terminal_write("\n");
        prompt();
    } else if (starts_with(input, "echo") &&
               (input[4] == '\0' || input[4] == ' ')) {
        if (input[4] == ' ' && input[5] != '\0') {
            terminal_write(&input[5]);
        }
        terminal_write("\n");
        prompt();
    } else if (text_equals(input, "reboot")) {
        reboot();
    } else if (text_equals(input, "shutdown")) {
        shutdown();
    } else {
        terminal_write("Unknown command. Type help for available commands.\n");
        prompt();
    }
}

void shell_init(void) {
    terminal_write("WELCOME TO NOVAOS\n");
    terminal_write("LIMINE X86-64 BOOT\n");
    terminal_write("Type help for available commands.\n");
    prompt();
}

void shell_input_character(char character) {
    if (character == '\n') {
        execute_command();
    } else if (character == '\b') {
        if (input_length > 0) {
            input_length--;
            terminal_putchar('\b');
        }
    } else if (input_length < SHELL_INPUT_CAPACITY - 1) {
        input[input_length++] = character;
        terminal_putchar(character);
    }
}
