/*
 * This class implements a controller for a standard, uncompressed tag table setup.
 */

#pragma once

#include <iostream>
#include "Controller.h"
#include "schema.h"
#include "Decoder.h"

class FlatTableController : public Controller {
public:
    explicit FlatTableController(gzFile output_trace, ofstream &output_log) : Controller(output_trace, output_log) {}

protected:
    static std::pair<uint64_t, uint16_t> translateToTagAddr(uint64_t addr_base) {  // converts the base address of a data cacheline to that of the corresponding tag cacheline, and the bit-index into the cacheline where the tags begin
        // TODO: DO I NEED TO SUBTRACT THE SIZE OF THE TAG TABLE ITSELF FROM EACH ADDRESS (BEFORE DOING ANY MATHS WITH IT)?
        // Though this would mean that the few accesses to nullptr would underflow..... perhaps check the source code of the thing that generated the trace files

        uint64_t tag_base = ((addr_base / 128) >> 6) << 6;  // base address of the tag cacheline (byte address)
        uint16_t tag_offset = ((addr_base>>6)*4) % 512;         // index into the tag cacheline (bit-index; multiple of 4 in [0,512))
        return {tag_base, tag_offset};   // 1 bit (the tag) per 16 bytes (the capability-aligned address)
    }

public:
    void setup(Decoder &decoder) override {
        Cacheline leaf_line;

        for (uint64_t rl = 0; rl < ROOT_TABLE_SIZE; rl += TAG_CACHE_LINE_SIZE) {        // do one root cacheline line at a time
            for (int rb = 0; rb < TAG_CACHE_LINE_SIZE; ++rb) {                          // each root cacheline contains TAG_CACHE_LINE_SIZE bytes
                for (int rt = 0; rt < 8; ++rt) {                                        // each byte contains 8 tags

                    // focus on this single root tag
                    decoder.getInitialTags((8*rl+8*rb+rt)*TAG_CACHE_LINE_SIZE*8, TAG_CACHE_LINE_SIZE*8, leaf_line);

                    cache.set(TAG_CACHE_LINE_SIZE*(8*rl+8*rb+rt), leaf_line);     // insert the leaf line into the cache
                }
            }
        }
    }
};
