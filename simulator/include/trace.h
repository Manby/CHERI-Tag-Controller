#pragma once

#include <cstdint>

enum initialAccessType : uint8_t
{
    INITIAL_ACCESS_TYPE_INSTR,
    INITIAL_ACCESS_TYPE_LOAD,
    INITIAL_ACCESS_TYPE_STORE,
    INITIAL_ACCESS_TYPE_CLOAD,
    INITIAL_ACCESS_TYPE_CSTORE
};

struct initialAccess
{
    int8_t type : 7;
    uint8_t tag : 1;

    initialAccess() = default;
    initialAccess(int8_t type, bool tag) : type(type), tag(tag) {}
};

enum llcMissType : uint8_t
{
    LLC_MISS_TYPE_READ,
    LLC_MISS_TYPE_WRITE
};

struct llcMiss
{
    llcMissType type;
    uint16_t size;
    uint16_t tags;
    uint16_t tags_known;
    uint64_t addr;

    llcMiss() = default;
    llcMiss(llcMissType type, uint16_t size, uint16_t tags, uint16_t tags_known, uint64_t addr) : type(type), size(size), tags(tags), tags_known(tags_known), addr(addr) {};
};