#include <stdint.h>

#include "pmm.h"

#define PMM_INVALID_ADDRESS UINT64_MAX

static uint8_t *page_bitmap;
static uint64_t hhdm_offset;
static uint64_t page_count;
static uint64_t used_pages;
static uint64_t allocation_hint;
static const struct limine_memmap_response *memory_map;

static uint8_t *bitmap_address(uint64_t physical_address) {
    return (uint8_t *)(hhdm_offset + physical_address);
}

static void set_page_state(uint64_t page, int allocated) {
    uint8_t mask = (uint8_t)(1u << (page & 7));
    if (allocated != 0) {
        page_bitmap[page >> 3] |= mask;
    } else {
        page_bitmap[page >> 3] &= (uint8_t)~mask;
    }
}

static int page_is_allocated(uint64_t page) {
    return (page_bitmap[page >> 3] & (1u << (page & 7))) != 0;
}

static uint64_t align_up(uint64_t value, uint64_t alignment) {
    uint64_t remainder = value % alignment;
    if (remainder == 0) {
        return value;
    }
    uint64_t adjustment = alignment - remainder;
    if (value > UINT64_MAX - adjustment) {
        return UINT64_MAX;
    }
    return value + adjustment;
}

static int range_end(uint64_t base, uint64_t length, uint64_t *end) {
    if (length == 0 || base > UINT64_MAX - length) {
        return 0;
    }
    *end = base + length;
    return 1;
}

static int page_range_valid(uint64_t base, uint64_t length,
                            uint64_t *first_page, uint64_t *last_page) {
    uint64_t end;
    if (!range_end(base, length, &end)) {
        return 0;
    }
    uint64_t first_address = align_up(base, PMM_PAGE_SIZE);
    uint64_t first = first_address / PMM_PAGE_SIZE;
    uint64_t last = end / PMM_PAGE_SIZE;
    if (first_address == UINT64_MAX || last == 0 || first >= last) {
        return 0;
    }
    last--;
    if (first >= page_count) {
        return 0;
    }
    if (last >= page_count) {
        last = page_count - 1;
    }
    *first_page = first;
    *last_page = last;
    return 1;
}

static void mark_range(uint64_t base, uint64_t length, int allocated) {
    uint64_t first_page;
    uint64_t last_page;
    if (!page_range_valid(base, length, &first_page, &last_page)) {
        return;
    }
    for (uint64_t page = first_page; page <= last_page; page++) {
        int was_allocated = page_is_allocated(page);
        if (was_allocated != allocated) {
            set_page_state(page, allocated);
            used_pages += allocated != 0 ? 1 : (uint64_t)-1;
        }
        if (page == UINT64_MAX) {
            break;
        }
    }
}

int pmm_init(const struct limine_memmap_response *new_memory_map,
            uint64_t new_hhdm_offset) {
    if (new_memory_map == 0 || new_memory_map->entries == 0 ||
        new_memory_map->entry_count == 0 || new_hhdm_offset == 0) {
        return 0;
    }

    uint64_t highest_address = 0;
    for (uint64_t index = 0; index < new_memory_map->entry_count; index++) {
        const struct limine_memmap_entry *entry =
            new_memory_map->entries[index];
        uint64_t end;
        if (entry == 0 || !range_end(entry->base, entry->length, &end)) {
            continue;
        }
        if (end > highest_address) {
            highest_address = end;
        }
    }
    if (highest_address == 0) {
        return 0;
    }
    if (highest_address > UINT64_MAX - (PMM_PAGE_SIZE - 1)) {
        return 0;
    }
    uint64_t new_page_count = (highest_address + PMM_PAGE_SIZE - 1) /
                              PMM_PAGE_SIZE;
    if (new_page_count == 0 || new_page_count > UINT64_MAX - 7) {
        return 0;
    }
    uint64_t bitmap_bytes = (new_page_count + 7) / 8;

    uint64_t bitmap_physical = PMM_INVALID_ADDRESS;
    uint64_t bitmap_pages = (bitmap_bytes + PMM_PAGE_SIZE - 1) /
                            PMM_PAGE_SIZE;
    for (uint64_t index = 0; index < new_memory_map->entry_count; index++) {
        const struct limine_memmap_entry *entry =
            new_memory_map->entries[index];
        if (entry == 0 || entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }
        uint64_t end;
        if (!range_end(entry->base, entry->length, &end)) {
            continue;
        }
        uint64_t candidate = align_up(entry->base, PMM_PAGE_SIZE);
        if (candidate != UINT64_MAX && bitmap_pages <= (end - candidate) /
                                            PMM_PAGE_SIZE) {
            bitmap_physical = candidate;
            break;
        }
    }
    if (bitmap_physical == PMM_INVALID_ADDRESS) {
        return 0;
    }
    if (new_hhdm_offset > UINT64_MAX - highest_address) {
        return 0;
    }

    page_count = new_page_count;
    used_pages = page_count;
    hhdm_offset = new_hhdm_offset;
    memory_map = new_memory_map;
    page_bitmap = bitmap_address(bitmap_physical);
    for (uint64_t index = 0; index < bitmap_bytes; index++) {
        page_bitmap[index] = 0xff;
    }
    for (uint64_t index = bitmap_bytes;
         index < bitmap_pages * PMM_PAGE_SIZE; index++) {
        page_bitmap[index] = 0xff;
    }

    for (uint64_t index = 0; index < new_memory_map->entry_count; index++) {
        const struct limine_memmap_entry *entry =
            new_memory_map->entries[index];
        if (entry != 0 && entry->type == LIMINE_MEMMAP_USABLE) {
            mark_range(entry->base, entry->length, 0);
        }
    }
    mark_range(bitmap_physical, bitmap_pages * PMM_PAGE_SIZE, 1);
    allocation_hint = 0;
    return 1;
}

uint64_t pmm_alloc_page(void) {
    if (page_bitmap == 0 || page_count == 0) {
        return 0;
    }
    for (uint64_t offset = 0; offset < page_count; offset++) {
        uint64_t page = allocation_hint + offset;
        if (page >= page_count) {
            page -= page_count;
        }
        if (!page_is_allocated(page)) {
            set_page_state(page, 1);
            used_pages++;
            allocation_hint = page + 1;
            if (allocation_hint == page_count) {
                allocation_hint = 0;
            }
            return page * PMM_PAGE_SIZE;
        }
    }
    return 0;
}

int pmm_free_page(uint64_t physical_address) {
    if (page_bitmap == 0 || physical_address % PMM_PAGE_SIZE != 0) {
        return 0;
    }
    uint64_t page = physical_address / PMM_PAGE_SIZE;
    if (page >= page_count || !page_is_allocated(page)) {
        return 0;
    }
    int usable = 0;
    for (uint64_t index = 0; index < memory_map->entry_count; index++) {
        const struct limine_memmap_entry *entry = memory_map->entries[index];
        uint64_t end;
        if (entry != 0 && entry->type == LIMINE_MEMMAP_USABLE &&
            range_end(entry->base, entry->length, &end) &&
            physical_address >= entry->base &&
            physical_address <= end - PMM_PAGE_SIZE) {
            usable = 1;
            break;
        }
    }
    if (!usable) {
        return 0;
    }
    set_page_state(page, 0);
    used_pages--;
    if (page < allocation_hint) {
        allocation_hint = page;
    }
    return 1;
}

uint64_t pmm_get_total_memory(void) {
    return page_count * PMM_PAGE_SIZE;
}

uint64_t pmm_get_used_memory(void) {
    return used_pages * PMM_PAGE_SIZE;
}

uint64_t pmm_get_free_memory(void) {
    return (page_count - used_pages) * PMM_PAGE_SIZE;
}

uint64_t pmm_get_page_count(void) {
    return page_count;
}

const struct limine_memmap_response *pmm_get_memory_map(void) {
    return memory_map;
}