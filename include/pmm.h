#ifndef NOVAOS_PMM_H
#define NOVAOS_PMM_H

#include <stdint.h>

#include "limine.h"

#define PMM_PAGE_SIZE 4096

int pmm_init(const struct limine_memmap_response *memory_map,
            uint64_t hhdm_offset);
uint64_t pmm_alloc_page(void);
int pmm_free_page(uint64_t physical_address);
uint64_t pmm_get_total_memory(void);
uint64_t pmm_get_used_memory(void);
uint64_t pmm_get_free_memory(void);
uint64_t pmm_get_page_count(void);
const struct limine_memmap_response *pmm_get_memory_map(void);

#endif