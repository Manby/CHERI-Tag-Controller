#pragma once

#include <cstdint>

enum initial_access_type : uint8_t
{
    INITIAL_ACCESS_TYPE_INSTR,
    INITIAL_ACCESS_TYPE_LOAD,
    INITIAL_ACCESS_TYPE_STORE,
    INITIAL_ACCESS_TYPE_CLOAD,
    INITIAL_ACCESS_TYPE_CSTORE,
};

struct initial_access
{
    int8_t type : 7;
    uint8_t tag : 1;

    initial_access() = default;
    initial_access(int8_t type, bool tag) : type(type), tag(tag) {}
};

enum llc_miss_type : uint8_t
{
    LLC_MISS_TYPE_READ,
    LLC_MISS_TYPE_WRITE
};

struct llc_miss
{
    llc_miss_type type;
    uint16_t size;
    uint16_t tags;
    uint16_t tags_known;
    uint64_t addr;

    llc_miss() = default;
    llc_miss(llc_miss_type type, uint16_t size, uint16_t tags, uint16_t tags_known, uint64_t addr) : type(type), size(size), tags(tags), tags_known(tags_known), addr(addr) {};
};