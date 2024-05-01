#pragma once

#include "trace.h"
#include <array>

constexpr bool ksw_debug = false;
constexpr bool ksw_testing = false;
#define DBG if (ksw_debug)
#define TST if (ksw_testing)

constexpr int TAG_CACHE_LINE_SIZE = 64;      //size of the tag cache's cachelines in bytes (this should probably be in controller.h)
typedef std::array<uint8_t, TAG_CACHE_LINE_SIZE> Cacheline;

constexpr uint64_t MEMORY_SIZE = ((uint64_t) 1) << 31;
constexpr uint64_t ROOT_TABLE_SIZE = MEMORY_SIZE >> 16;

constexpr uint64_t CSZERO = 1 << 31;

#define QEMU_BASE_ADDRESS 0x80000000 // 2^31

bool assumeTag(bool tag, initialAccessType type) {
    switch (type) {
        case INITIAL_ACCESS_TYPE_INSTR:
        case INITIAL_ACCESS_TYPE_LOAD:
        case INITIAL_ACCESS_TYPE_STORE:
            return false;
        case INITIAL_ACCESS_TYPE_CLOAD:
        case INITIAL_ACCESS_TYPE_CSTORE:
            return tag;
        default:
            //cout << "type was " << (int) type << endl;
            return false; // initial access file claims this address was never accessed (type is -1), so we can return any value
    }
}
