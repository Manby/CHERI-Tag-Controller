#pragma once

#include "trace.h"
#include "Decoder.h"
#include "Controller.h"
#include <unordered_set>

class Simulator {
public:
    size_t processTrace(Controller &controller, vector<memAccess> &buffer, size_t n, unordered_set<size_t> log_points) {
        size_t i = 0;
        while (i < n) {
            if (buffer[i].type == ACCESS_TYPE_READ) controller.handleRead(buffer[i]);  // calls are automatically inlined
            else controller.handleWrite(buffer[i]);

            if (log_points.find(i) != log_points.end()) {
                cout << "LOG @ " << i << endl;
                controller.dump_cache();
            }

            ++i;
        }

        return i;
    }
};
