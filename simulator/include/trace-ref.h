#ifndef SIMULATOR_TRACE_H
#define SIMULATOR_TRACE_H

#include <cstdint>

enum initial_access_type : uint8_t
{
    INITIAL_ACCESS_TYPE_INSTR,
    INITIAL_ACCESS_TYPE_LOAD,
    INITIAL_ACCESS_TYPE_STORE,
    INITIAL_ACCESS_TYPE_CLOAD,
    INITIAL_ACCESS_TYPE_CSTORE,
};


typedef struct initial_access_2 initial_access_2;
struct initial_access_2
{
    initial_access_type type;
    uint8_t tag; // ignore for LOADs, either 0 or 1
    uint16_t size;
    uint64_t vaddr; // only for reconstructing the minority of missing paddrs
    uint64_t paddr;
} /*__attribute__((packed))*/;


typedef struct initial_access initial_access;
struct initial_access
{
    int8_t type : 7;
    uint8_t tag : 1;

    initial_access(int8_t type, bool tag) : type(type), tag(tag) {}
};

enum llc_miss_type : uint8_t
{
    LLC_MISS_TYPE_READ,
    LLC_MISS_TYPE_WRITE
};

typedef struct llc_miss llc_miss;
struct llc_miss
{
    llc_miss_type type;
    uint16_t size; // just the cache line size
    uint16_t tags; // number of tags depends on cache line size
    uint16_t tags_known; // mask indicating which tags are actually known
    uint64_t addr;
};

#endif //SIMULATOR_TRACE_H
