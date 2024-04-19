/*
 * This class implements the controller used by Arm's Morello Processors
 */

#pragma once

#include <bitset>
#include <iostream>
#include <iomanip>
#include "FlatTableController.h"
#include "Decoder.h"
#include "schema.h"

class MorelloController : public FlatTableController {
private:
    bool cache_type;

public:
    MorelloController(Decoder &decoder, ofstream &output_trace, ofstream &output_log, bool cache_type) : FlatTableController(output_trace, output_log), cache_type(cache_type) {
        setupCache(decoder);
    }

    void handleMemoryAccess(memAccess ax) override {
        uint64_t leaf_base_addr;
        uint16_t leaf_cacheline_index;
        Cacheline leaf_line;
        bitset<8> leaf_byte;
        uint8_t leaf_tags;

        switch (ax.type) {
            case ACCESS_TYPE_READ:
                DBG cout << "\nREAD @ " << ax.addr << endl;

                leaf_base_addr = translateToTagAddr(ax.addr).first;   // base address of the cacheline
                leaf_line = cache.peek(leaf_base_addr);

                leaf_tags = false;
                for (uint8_t byte : leaf_line) {
                    if (byte != 0) {
                        leaf_tags = true;
                        break;
                    }
                }

                if (((leaf_tags == 0) && !cache_type) ||
                        ((leaf_tags != 0) && cache_type)) {// if we are responsible for handling this one, then log it
                    cache.doRead(leaf_base_addr);
                }

                return; // in implementation, we would return whatever the tags are

            case ACCESS_TYPE_WRITE:
                DBG cout << "\nWRITE @ " << ax.addr << endl;

                auto result = translateToTagAddr(ax.addr);
                leaf_base_addr = result.first;
                leaf_cacheline_index = result.second;

                leaf_line = cache.peek(leaf_base_addr);

                // find out which cache should log the READ
                leaf_tags = false;
                for (uint8_t byte : leaf_line) {
                    if (byte != 0) {
                        leaf_tags = true;
                        break;
                    }
                }

                if (((leaf_tags == 0) && !cache_type) ||
                    ((leaf_tags != 0) && cache_type)) {// if we are responsible for handling the READ, then log it
                    cache.doRead(leaf_base_addr);
                }

                leaf_byte = leaf_line[leaf_cacheline_index / 8];

                leaf_byte[leaf_cacheline_index % 8] = (ax.tags & 8) ? 1 : 0;
                leaf_byte[(leaf_cacheline_index + 1) % 8] = (ax.tags & 4) ? 1 : 0;
                leaf_byte[(leaf_cacheline_index + 2) % 8] = (ax.tags & 2) ? 1 : 0;
                leaf_byte[(leaf_cacheline_index + 3) % 8] = (ax.tags & 1) ? 1 : 0;
                // TODO: check above not backwards
                leaf_line[leaf_cacheline_index / 8] = (uint8_t) leaf_byte.to_ulong();

                // find out which cache should log the WRITE
                leaf_tags = false;
                for (uint8_t byte : leaf_line) {
                    if (byte != 0) {
                        leaf_tags = true;
                        break;
                    }
                }

                if (((leaf_tags == 0) && !cache_type) ||
                    ((leaf_tags != 0) && cache_type)) {// if we are responsible for handling the WRITE, then log it
                    cache.doWrite(leaf_base_addr, leaf_line);
                } else {
                    // otherwise, still update the state so the simulation remains accurate
                    cache.set(leaf_base_addr, leaf_line);
                }

                return;
        }
    }
};