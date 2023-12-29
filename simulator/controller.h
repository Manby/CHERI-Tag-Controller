//
// Created by kofi on 28/12/23.
//

#ifndef SIMULATOR_CONTROLLER_H
#define SIMULATOR_CONTROLLER_H

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
    Controller();
    virtual void handleMemoryAccess(access ax) = 0;
};


#endif //SIMULATOR_CONTROLLER_H
