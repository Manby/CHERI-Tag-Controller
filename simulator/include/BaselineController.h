/*
 * This class implements a controller for a standard, uncompressed tag table setup.
 */

#pragma once

#include <iostream>
#include "FlatTableController.h"
#include "schema.h"
#include "utils.h"
#include "Decoder.h"

class BaselineController : public FlatTableController {
public:
    BaselineController(ofstream &output_trace, ofstream &output_log) : FlatTableController(output_trace, output_log) {}

    void handleMemoryAccess(memAccess ax) override {
        uint64_t leaf_base_addr;
        uint16_t leaf_cacheline_index;
        Cacheline leaf_line;

        switch (ax.type) {
            case ACCESS_TYPE_READ:
                cache.doRead(translateToTagAddr(ax.addr).first);
                return; // in implementation, we would return whatever the tags are

            case ACCESS_TYPE_WRITE:
                auto result = translateToTagAddr(ax.addr);
                leaf_base_addr = result.first;
                leaf_cacheline_index = result.second;

                leaf_line = cache.doRead(leaf_base_addr);
                modifyTags(leaf_line, leaf_cacheline_index, ax.tags);
                cache.doWrite(leaf_base_addr, leaf_line);
        }
    }
};