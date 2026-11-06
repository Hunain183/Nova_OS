#ifndef NOVAOS_SCHEDULER_H
#define NOVAOS_SCHEDULER_H

#include <stdint.h>

#define SCHEDULER_MAX_TASKS 8

enum task_state {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_TERMINATED,
};

struct task_info {
    uint64_t id;
    enum task_state state;
    uint64_t switches;
};

typedef void (*task_entry)(void);

int scheduler_initialize(uint64_t hhdm_offset);
int scheduler_run_demo(void);
void scheduler_yield(void);
uint64_t scheduler_task_count(void);
uint64_t scheduler_switch_count(void);
int scheduler_get_task(uint64_t index, struct task_info *info);
int scheduler_kill(uint64_t id);
const char *scheduler_state_name(enum task_state state);

#endif