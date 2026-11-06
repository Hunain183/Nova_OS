#include <stdint.h>

#include "pmm.h"
#include "scheduler.h"
#include "terminal.h"

#define SCHEDULER_STACK_PAGES 1
#define SCHEDULER_STACK_SIZE (SCHEDULER_STACK_PAGES * PMM_PAGE_SIZE)

struct task_context {
    uint64_t rsp;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;
    uint64_t rbp;
};

struct task {
    uint64_t id;
    enum task_state state;
    task_entry entry;
    struct task_context context;
    uint64_t stack_physical[SCHEDULER_STACK_PAGES];
    uint64_t switches;
};

extern void context_switch(struct task_context *from,
                            const struct task_context *to);

static struct task tasks[SCHEDULER_MAX_TASKS];
static struct task_context scheduler_context;
static uint64_t hhdm_offset;
static uint64_t next_task_id = 1;
static uint64_t task_count;
static uint64_t switch_count;
static int current_task = -1;
static int scheduler_active;

static void *physical_to_virtual(uint64_t physical_address) {
    return (void *)(hhdm_offset + physical_address);
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

static int find_next_ready(void) {
    if (task_count == 0) {
        return -1;
    }
    for (uint64_t offset = 1; offset <= SCHEDULER_MAX_TASKS; offset++) {
        uint64_t index = (uint64_t)(current_task + offset) %
                          SCHEDULER_MAX_TASKS;
        if (tasks[index].state == TASK_READY) {
            return (int)index;
        }
    }
    return -1;
}

static void release_stack(struct task *task) {
    for (uint64_t page = 0; page < SCHEDULER_STACK_PAGES; page++) {
        if (task->stack_physical[page] != 0) {
            pmm_free_page(task->stack_physical[page]);
            task->stack_physical[page] = 0;
        }
    }
}

static void task_bootstrap(void) {
    struct task *task = &tasks[current_task];
    task->state = TASK_RUNNING;
    task->entry();
    task->state = TASK_TERMINATED;
    scheduler_yield();
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static int create_task(task_entry entry) {
    int slot = -1;
    for (uint64_t index = 0; index < SCHEDULER_MAX_TASKS; index++) {
        if (tasks[index].state == TASK_TERMINATED || tasks[index].id == 0) {
            slot = (int)index;
            break;
        }
    }
    if (slot < 0) {
        return 0;
    }
    struct task *task = &tasks[slot];
    *task = (struct task){0};
    for (uint64_t page = 0; page < SCHEDULER_STACK_PAGES; page++) {
        task->stack_physical[page] = pmm_alloc_page();
        if (task->stack_physical[page] == 0) {
            release_stack(task);
            return 0;
        }
    }
    uint64_t stack_top = (uint64_t)physical_to_virtual(
        task->stack_physical[SCHEDULER_STACK_PAGES - 1]) + PMM_PAGE_SIZE;
    stack_top &= ~0xfULL;
    stack_top -= 8 * sizeof(uint64_t);
    volatile uint64_t *stack = (volatile uint64_t *)stack_top;
    for (uint64_t index = 0; index < 6; index++) {
        stack[index] = 0;
    }
    stack[6] = (uint64_t)task_bootstrap;
    task->context.rsp = stack_top;
    task->id = next_task_id++;
    task->state = TASK_READY;
    task->entry = entry;
    task_count++;
    return 1;
}

static void demo_task_one(void) {
    for (uint64_t count = 1; count <= 3; count++) {
        terminal_write("Task one count ");
        write_decimal(count);
        terminal_write("\n");
        scheduler_yield();
    }
}

static void demo_task_two(void) {
    for (uint64_t count = 1; count <= 3; count++) {
        terminal_write("Task two count ");
        write_decimal(count);
        terminal_write("\n");
        scheduler_yield();
    }
}

static void demo_task_three(void) {
    uint64_t total = 0;
    for (uint64_t count = 1; count <= 3; count++) {
        total += count;
        terminal_write("Task three total ");
        write_decimal(total);
        terminal_write("\n");
        scheduler_yield();
    }
}

int scheduler_initialize(uint64_t new_hhdm_offset) {
    if (new_hhdm_offset == 0) {
        return 0;
    }
    hhdm_offset = new_hhdm_offset;
    tasks[0] = (struct task){0};
    next_task_id = 1;
    task_count = 0;
    switch_count = 0;
    current_task = -1;
    scheduler_active = 0;
    return 1;
}

void scheduler_yield(void) {
    if (scheduler_active == 0 || current_task < 0) {
        return;
    }
    int previous = current_task;
    int next = find_next_ready();
    if (tasks[previous].state == TASK_RUNNING) {
        tasks[previous].state = TASK_READY;
    }
    if (next < 0) {
        scheduler_active = 0;
        current_task = -1;
        context_switch(&tasks[previous].context, &scheduler_context);
        return;
    }
    current_task = next;
    tasks[next].state = TASK_RUNNING;
    tasks[next].switches++;
    switch_count++;
    context_switch(&tasks[previous].context, &tasks[next].context);
}

int scheduler_run_demo(void) {
    if (scheduler_active != 0 || task_count != 0) {
        return 0;
    }
    if (!create_task(demo_task_one) || !create_task(demo_task_two) ||
        !create_task(demo_task_three)) {
        for (uint64_t index = 0; index < SCHEDULER_MAX_TASKS; index++) {
            release_stack(&tasks[index]);
            tasks[index].state = TASK_TERMINATED;
        }
        task_count = 0;
        return 0;
    }
    scheduler_active = 1;
    current_task = find_next_ready();
    tasks[current_task].state = TASK_RUNNING;
    tasks[current_task].switches++;
    switch_count++;
    context_switch(&scheduler_context, &tasks[current_task].context);
    for (uint64_t index = 0; index < SCHEDULER_MAX_TASKS; index++) {
        release_stack(&tasks[index]);
        tasks[index].state = TASK_TERMINATED;
    }
    task_count = 0;
    current_task = -1;
    return 1;
}

uint64_t scheduler_task_count(void) {
    return task_count;
}

uint64_t scheduler_switch_count(void) {
    return switch_count;
}

int scheduler_get_task(uint64_t index, struct task_info *info) {
    if (info == 0 || index >= SCHEDULER_MAX_TASKS || tasks[index].id == 0) {
        return 0;
    }
    info->id = tasks[index].id;
    info->state = tasks[index].state;
    info->switches = tasks[index].switches;
    return 1;
}

int scheduler_kill(uint64_t id) {
    for (uint64_t index = 0; index < SCHEDULER_MAX_TASKS; index++) {
        if (tasks[index].id == id && tasks[index].state == TASK_READY) {
            tasks[index].state = TASK_TERMINATED;
            release_stack(&tasks[index]);
            if (task_count != 0) {
                task_count--;
            }
            return 1;
        }
    }
    return 0;
}

const char *scheduler_state_name(enum task_state state) {
    switch (state) {
    case TASK_READY:
        return "READY";
    case TASK_RUNNING:
        return "RUNNING";
    case TASK_BLOCKED:
        return "BLOCKED";
    case TASK_TERMINATED:
        return "TERMINATED";
    }
    return "UNKNOWN";
}