#pragma once

#include "trace.h"
#include "Decoder.h"
#include "Controller.h"
#include <unordered_set>
#include <algorithm>

constexpr size_t CHUNK_SIZE = 1 << 8;    // the size of the memory access buffer (CHUNK_SIZE * (sizeof(llcMiss) == 16)) is the size of the buffer used in bytes

class Simulator {
public:
    static size_t processTrace(Decoder &decoder, Controller &controller, size_t n, unordered_set<size_t> &log_points) {
        size_t processed = 0;


        vector<memAccess> buffer(CHUNK_SIZE);
        for (size_t at = 0; at < n; at += CHUNK_SIZE) {
            size_t read = decoder.readLLCMisses(buffer, min(CHUNK_SIZE, n-at));

            int i = 0;
            for (size_t remaining = read; remaining > 0; --remaining) {     // CLion hint can be ignored
                if (buffer[i].type == ACCESS_TYPE_READ)
                    controller.handleRead(buffer[i]);  // calls are automatically inlined
                else controller.handleWrite(buffer[i]);

                if (log_points.find(processed) != log_points.end()) {
                    cout << "LOG @ " << processed << endl;
                    controller.dump_table();
                }

                ++i;
                ++processed;
            }
        }

        return processed;
    }
};
