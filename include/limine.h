#ifndef NOVAOS_LIMINE_H
#define NOVAOS_LIMINE_H

#include <stdint.h>

#define LIMINE_BASE_REVISION(N) \
    { 0xf9562b2d5c95a6c8, 0x6a7b384944536bdc, (N) }

#define LIMINE_REQUESTS_START_MARKER \
    { 0xf6b8f4b39de7d1ae, 0xfab91a6940fcb9cf, \
      0x785c6ed015d3e316, 0x181e920a7852b9d9 }

#define LIMINE_REQUESTS_END_MARKER \
    { 0xadc0e0531bb10d03, 0x9572709f31764c62 }

#define LIMINE_COMMON_MAGIC 0xc7b1dd30df4c8b88, 0x0a82e883a194f07b
#define LIMINE_HHDM_REQUEST_ID \
    { LIMINE_COMMON_MAGIC, 0x48dcf1cb8ad2b852, 0x63984e959a98244b }

struct limine_hhdm_response {
    uint64_t revision;
    uint64_t offset;
};

struct limine_hhdm_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_hhdm_response *response;
};

#endif