#include <stdint.h>

#include "interrupts.h"
#include "pmm.h"
#include "scheduler.h"
#include "shell.h"
#include "terminal.h"
#include "timer.h"

#define SHELL_INPUT_CAPACITY 128
#define NOVAOS_VERSION "0.2.0"

static char input[SHELL_INPUT_CAPACITY];
static uint8_t input_length;
static uint64_t allocation_test_page;

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

static void write_decimal(uint64_t value) {
    char digits[20];
    uint8_t length = 0;
    if (value == 0) {
        terminal_putchar('0');
        return;
    }
    while (value != 0) {
        digits[length++] = (char)('0' + value % 10);
        value /= 10;
    }
    while (length != 0) {
        terminal_putchar(digits[--length]);
    }
}

static int parse_decimal(const char *text, uint64_t *value) {
    uint64_t result = 0;
    if (*text == '\0') {
        return 0;
    }
    while (*text != '\0') {
        if (*text < '0' || *text > '9' ||
            result > (UINT64_MAX - (uint64_t)(*text - '0')) / 10) {
            return 0;
        }
        result = result * 10 + (uint64_t)(*text - '0');
        text++;
    }
    *value = result;
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
        terminal_write("help clear about version echo reboot shutdown ticks uptime meminfo memmap alloc free ps tasktest scheduler kill exceptions inttest exception-test\n");
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
    } else if (text_equals(input, "ticks")) {
        terminal_write("Timer ticks: ");
        write_decimal(timer_ticks());
        terminal_write("\n");
        prompt();
    } else if (text_equals(input, "uptime")) {
        terminal_write("Uptime seconds: ");
        write_decimal(timer_uptime_seconds());
        terminal_write("\n");
        prompt();
    } else if (text_equals(input, "meminfo")) {
        terminal_write("Total memory: ");
        write_decimal(pmm_get_total_memory());
        terminal_write(" bytes\nUsed memory: ");
        write_decimal(pmm_get_used_memory());
        terminal_write(" bytes\nFree memory: ");
        write_decimal(pmm_get_free_memory());
        terminal_write(" bytes\n");
        prompt();
    } else if (text_equals(input, "memmap")) {
        const struct limine_memmap_response *memory_map =
            pmm_get_memory_map();
        terminal_write("Physical memory map:\n");
        for (uint64_t index = 0; index < memory_map->entry_count; index++) {
            const struct limine_memmap_entry *entry =
                memory_map->entries[index];
            if (entry == 0) {
                continue;
            }
            terminal_write("Base ");
            write_decimal(entry->base);
            terminal_write(" Length ");
            write_decimal(entry->length);
            terminal_write(" Type ");
            write_decimal(entry->type);
            terminal_write("\n");
        }
        prompt();
    } else if (text_equals(input, "alloc")) {
        if (allocation_test_page != 0) {
            terminal_write("Allocation test page is already allocated.\n");
        } else {
            allocation_test_page = pmm_alloc_page();
            if (allocation_test_page == 0) {
                terminal_write("Allocation failed: out of physical memory.\n");
            } else {
                terminal_write("Allocated physical page: ");
                write_decimal(allocation_test_page);
                terminal_write("\n");
            }
        }
        prompt();
    } else if (text_equals(input, "free")) {
        if (allocation_test_page != 0 &&
            pmm_free_page(allocation_test_page)) {
            allocation_test_page = 0;
            terminal_write("Allocation test page released.\n");
        } else {
            terminal_write("No allocation test page is active.\n");
        }
        terminal_write("Free memory: ");
        write_decimal(pmm_get_free_memory());
        terminal_write(" bytes\n");
        prompt();
    } else if (text_equals(input, "ps")) {
        struct task_info info;
        terminal_write("Tasks:\n");
        for (uint64_t index = 0; index < SCHEDULER_MAX_TASKS; index++) {
            if (scheduler_get_task(index, &info)) {
                terminal_write("ID ");
                write_decimal(info.id);
                terminal_write(" ");
                terminal_write(scheduler_state_name(info.state));
                terminal_write(" switches ");
                write_decimal(info.switches);
                terminal_write("\n");
            }
        }
        prompt();
    } else if (text_equals(input, "tasktest")) {
        if (scheduler_run_demo()) {
            terminal_write("Task demonstration completed.\n");
        } else {
            terminal_write("Task demonstration could not start.\n");
        }
        prompt();
    } else if (text_equals(input, "scheduler")) {
        terminal_write("Cooperative scheduler\nTasks: ");
        write_decimal(scheduler_task_count());
        terminal_write(" Context switches: ");
        write_decimal(scheduler_switch_count());
        terminal_write("\n");
        prompt();
    } else if (starts_with(input, "kill ")) {
        uint64_t id;
        if (parse_decimal(&input[5], &id) && scheduler_kill(id)) {
            terminal_write("Task terminated.\n");
        } else {
            terminal_write("Unable to terminate that task.\n");
        }
        prompt();
    } else if (text_equals(input, "exceptions")) {
        terminal_write("Supported CPU exceptions:\n");
        for (uint64_t number = 0; number <= 20; number++) {
            write_decimal(number);
            terminal_write(" ");
            terminal_write(interrupt_exception_name(number));
            terminal_write("\n");
        }
        prompt();
    } else if (text_equals(input, "inttest")) {
        terminal_write("Interrupt system operational. Timer ticks: ");
        write_decimal(timer_ticks());
        terminal_write("\n");
        prompt();
    } else if (text_equals(input, "exception-test")) {
        terminal_write("DESTRUCTIVE TEST: triggering divide error; system will halt.\n");
        interrupt_test_divide_error();
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
