#pragma once

#include "Memory.h"
#include "trace.h"
#include "Decoder.h"
#include <cstdint>


class Controller {

protected:
    Memory memory;

public:
    explicit Controller(gzFile output_trace, ofstream &output_log) : memory(output_trace, output_log) {}

    virtual void setup(Decoder &decoder) = 0;

    virtual uint16_t handleRead(memAccess ax) = 0;

    virtual void handleWrite(memAccess ax) = 0;

    void dump_table() {
        memory.dump_table();
    }

    int getNumAccesses() {
        return memory.getNumAccesses();
    }

    vector<champsimInstr> getPrevLogged() {
        return memory.getPrevLogged();
    }

    virtual void reportStats() {}
};