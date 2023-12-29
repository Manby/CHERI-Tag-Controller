#pragma once

#include <cstdint>
#include <iostream>
#include "trace.h"
#include "controller.h"

size_t read_init_accesses(FILE *file, initial_access *buffer, size_t n);

size_t read_llc_misses(FILE *file, llc_miss *buffer, size_t n);

class Simulator {
public:
    size_t processTrace(Controller &controller, llc_miss *buffer, size_t n);
};
