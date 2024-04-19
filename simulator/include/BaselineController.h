/*
 * This class implements a controller for a standard, uncompressed tag table setup.
 */

#pragma once

#include <bitset>
#include <iostream>
#include "FlatTableController.h"
#include "schema.h"
#include "Decoder.h"

class BaselineController : public FlatTableController {
public:
    BaselineController(Decoder &decoder, ofstream &output_trace, ofstream &output_log) : FlatTableController(output_trace, output_log) {
        setupCache(decoder);
    }

    void handleMemoryAccess(memAccess ax) override {
        uint64_t leaf_base_addr;
        uint16_t leaf_cacheline_index;
        Cacheline leaf_line;
        bitset<8> leaf_byte;

        switch (ax.type) {
            case ACCESS_TYPE_READ:
                cache.doRead(translateToTagAddr(ax.addr).first);
                return; // in implementation, we would return whatever the tags are

            case ACCESS_TYPE_WRITE:
                auto result = translateToTagAddr(ax.addr);
                leaf_base_addr = result.first;
                leaf_cacheline_index = result.second;

                leaf_line = cache.doRead(leaf_base_addr);
                leaf_byte = leaf_line[leaf_cacheline_index / 8];

                leaf_byte[leaf_cacheline_index % 8] = (ax.tags & 8) ? 1 : 0;
                leaf_byte[(leaf_cacheline_index + 1) % 8] = (ax.tags & 4) ? 1 : 0;
                leaf_byte[(leaf_cacheline_index + 2) % 8] = (ax.tags & 2) ? 1 : 0;
                leaf_byte[(leaf_cacheline_index + 3) % 8] = (ax.tags & 1) ? 1 : 0;
                // TODO: check above not backwards
                leaf_line[leaf_cacheline_index / 8] = (uint8_t) leaf_byte.to_ulong();

                cache.doWrite(leaf_base_addr, leaf_line);
        }
    }
};