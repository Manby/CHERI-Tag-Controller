/*
 * This class implements the controller used by Arm's Morello Processors
 */

#pragma once

#include <iostream>
#include <iomanip>
#include "FlatTableController.h"
#include "Decoder.h"
#include "schema.h"
#include "utils.h"

class MorelloController : public FlatTableController {
private:
    bool cache_type;

public:
    MorelloController(gzFile output_trace, ofstream &output_log, bool cache_type) : FlatTableController(output_trace, output_log), cache_type(cache_type) {}

    uint16_t handleRead(memAccess ax) override {
        pair<uint64_t, uint16_t> result;
        uint64_t leaf_base_addr;
        uint16_t leaf_cacheline_index;
        Cacheline leaf_line;
        bool no_leaf_tags;

        switch (ax.type) {
            case ACCESS_TYPE_READ:
                DBG cout << "\nREAD @ " << ax.addr << endl;
                result = translateToTagAddr(ax.addr);
                leaf_base_addr = result.first;   // base address of the cacheline
                leaf_cacheline_index = result.second;
                leaf_line = cache.peek(leaf_base_addr);

                // find out which cache should log the READ
                no_leaf_tags = isClear(leaf_line);

                if ((no_leaf_tags && !cache_type) || (!no_leaf_tags && cache_type)) {// if we are responsible for handling this one, then log it
                    cache.doRead(leaf_base_addr);
                }

                return getTags(leaf_line, leaf_cacheline_index); // in implementation, we would return whatever the tags are

            case ACCESS_TYPE_WRITE:
                assert(false);  // wrong call was made!
        }

        assert(false);
        return 0xffff;  // should never get here!
    }

    void handleWrite(memAccess ax) override {
        uint64_t leaf_base_addr;
        uint16_t leaf_cacheline_index;
        Cacheline leaf_line;
        bool no_leaf_tags;

        switch (ax.type) {
            case ACCESS_TYPE_READ:
                assert(false);  // wrong call was made!

            case ACCESS_TYPE_WRITE:
                DBG cout << "\nWRITE @ " << ax.addr << endl;

                auto result = translateToTagAddr(ax.addr);
                leaf_base_addr = result.first;
                leaf_cacheline_index = result.second;

                leaf_line = cache.peek(leaf_base_addr);

                // find out which cache should log the READ
                no_leaf_tags = isClear(leaf_line);

                if ((no_leaf_tags && !cache_type) || (!no_leaf_tags && cache_type)) {// if we are responsible for handling this one, then log it
                    cache.doRead(leaf_base_addr);
                }

                modifyTags(leaf_line, leaf_cacheline_index, ax.tags);

                // find out which cache should log the WRITE
                no_leaf_tags = isClear(leaf_line);

                if ((no_leaf_tags && !cache_type) || (!no_leaf_tags && cache_type)) {// if we are responsible for handling this one, then log it
                    cache.doWrite(leaf_base_addr, leaf_line);
                } else {
                    // otherwise, still update the state so the simulation remains accurate
                    cache.set(leaf_base_addr, leaf_line);
                }

                return;
        }
    }
};