#pragma once

#include <cstdint>
#include <iostream>
#include "trace.h"
#include "controller.h"

class Decoder {
private:
    FILE *initial_accesses;
    FILE *trace;
    bool getInitialTag(uint64_t addr);
    access furnish(llcMiss miss);
public:
    Decoder(FILE *initial_accesses, FILE *trace);
    // size_t read_init_accesses(initialAccess *buffer, size_t n);
    size_t read_llc_misses(access *buffer, size_t n);
};

class Simulator {
public:
    size_t processTrace(Decoder &decoder, Controller &controller, access *buffer, size_t n);
};
