#pragma once

#include "Cache.h"
#include "trace.h"
#include "Decoder.h"
#include <cstdint>


class Controller {

protected:
    Cache cache;

public:
    explicit Controller(ofstream &output_trace, ofstream &output_log) : cache(output_trace, output_log) {}

    virtual void setup(Decoder &decoder) = 0;

    virtual uint16_t handleRead(memAccess ax) = 0;

    virtual void handleWrite(memAccess ax) = 0;

    void dump_cache() {
        cache.dump();
    }

    int getNumAccesses() {
        return cache.getNumAccesses();
    }

    vector<champsimInstr> getPrevLogged() {
        return cache.getPrevLogged();
    }

    virtual void reportStats() {}
};