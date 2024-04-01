#pragma once

#include "trace.h"
#include "decoder.h"
#include "controller.h"
#include <unordered_set>

class Simulator {
public:
    size_t processTrace(Decoder &decoder, Controller &controller, vector<access> &buffer, size_t n, unordered_set<size_t> log_points) {
        size_t i = 0;
        while (i < n) {
            controller.handleMemoryAccess(buffer[i]);  // calls are automatically inlined

            if (log_points.find(i) != log_points.end()) {
                cout << "LOG @ " << i << endl;
                controller.dump_cache();
            }

            ++i;
        }

        return i;
    }
};
