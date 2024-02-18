#pragma once

#include "trace.h"
#include "decoder.h"
#include "controller.h"

class Simulator {
public:
    template <size_t l> size_t processTrace(Decoder &decoder, Controller &controller, array<access, l> &buffer, size_t n) {
        size_t i = 0;
        while (i < n) {
            controller.handleMemoryAccess(buffer[i]);  // calls are automatically inlined
            ++i;
        }

        return i;
    }
};
