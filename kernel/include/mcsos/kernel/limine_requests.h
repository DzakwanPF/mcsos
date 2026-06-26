#ifndef MCSOS_LIMINE_REQUESTS_H
#define MCSOS_LIMINE_REQUESTS_H

#include <stdint.h>
#include <stddef.h>

/* Limine base revision */
__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[3] = {
    0xf9562b2d5c95a6c8ULL,
    0x6a7b384944536bfcULL,
    3
};

/* Limine requests terminator */
__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[2] = {
    0xf6b8f4b39de7d1aeULL,
    0x5c4986d1b9ef80acULL
};

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[2] = {
    0xadc0e0531bb10d03ULL,
    0x9572709f31764c62ULL
};

#endif
