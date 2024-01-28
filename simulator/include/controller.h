#pragma once

#include "cache.h"
#include "trace.h"
#include <cstdint>

enum access_type : uint8_t
{
    ACCESS_TYPE_READ,
    ACCESS_TYPE_WRITE
};

struct access
{
    access_type type;
    uint16_t size;
    uint16_t tags;
    uint64_t addr;

    access() = default;
    access(access_type type, uint16_t size, uint16_t tags, uint64_t addr) : type(type), size(size), tags(tags), addr(addr) {};
};

class Controller {

protected:
    Cache cache;
public:
    explicit Controller(ofstream &output_trace) : cache(output_trace) {}

    virtual void handleMemoryAccess(access ax) = 0;
};