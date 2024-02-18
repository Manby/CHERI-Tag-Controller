#pragma once

#include "cache.h"
#include "trace.h"
#include <cstdint>


class Controller {

protected:
    Cache cache;
public:
    explicit Controller(ofstream &output_trace) : cache(output_trace) {}

    virtual void handleMemoryAccess(access ax) = 0;
};