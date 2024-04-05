#pragma once

#include "cache.h"
#include "trace.h"
#include <cstdint>


class Controller {

protected:
    Cache cache;
public:
    explicit Controller(ofstream &output_trace, ofstream &output_log) : cache(output_trace, output_log) {}

    virtual void handleMemoryAccess(access ax) = 0;

    void dump_cache() {
        cache.dump();
    }

    int get_num_accesses() {
        return cache.get_num_accesses();
    }
};